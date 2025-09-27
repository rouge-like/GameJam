#include "FusionMode.h"

#include <cstdarg>

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"

DEFINE_LOG_CATEGORY_STATIC(LogFusionMode, Log, All);

FColor AFusionMode::GetLogColor(ELogVerbosity::Type Verbosity) const
{
    switch (Verbosity)
    {
    case ELogVerbosity::Error:
        return FColor::Red;
    case ELogVerbosity::Warning:
        return FColor::Yellow;
    case ELogVerbosity::Verbose:
    case ELogVerbosity::VeryVerbose:
        return FColor::Silver;
    default:
        return FColor::Cyan;
    }
}

void AFusionMode::LogOnScreen(ELogVerbosity::Type Verbosity, const TCHAR* Format, ...) const
{
    va_list ArgPtr;
    va_start(ArgPtr, Format);
    TCHAR Buffer[1024];
    FCString::GetVarArgs(Buffer, UE_ARRAY_COUNT(Buffer), Format, ArgPtr);
    va_end(ArgPtr);

    const FString Message(Buffer);

    // UE_LOG(LogFusionMode, Verbosity, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, GetLogColor(Verbosity), Message);
    }
}

AFusionMode::AFusionMode()
{
    GestureStreamUrl = TEXT("ws://127.0.0.1:8765/gesture_stream");
    DescribeEndpoint = TEXT("http://127.0.0.1:8000/descriptions");
    VoiceQueryEndpoint = TEXT("http://127.0.0.1:8000/voice-query");
    GestureKeepAliveInterval = 5.f;
}

void AFusionMode::BeginPlay()
{
    Super::BeginPlay();

    InitializeGestureWebSocket();
    ScheduleGestureKeepAlive();
}

void AFusionMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ShutdownGestureWebSocket();
    GetWorldTimerManager().ClearTimer(GestureKeepAliveHandle);
    GetWorldTimerManager().ClearTimer(GestureReconnectHandle);

    Super::EndPlay(EndPlayReason);
}

void AFusionMode::InitializeGestureWebSocket()
{
    if (GestureStreamUrl.IsEmpty())
    {
        LogOnScreen(ELogVerbosity::Warning, TEXT("GestureStreamUrl is empty; skipping WebSocket initialisation."));
        return;
    }

    if (GestureSocket.IsValid())
    {
        LogOnScreen(ELogVerbosity::Verbose, TEXT("Gesture WebSocket already initialised."));
        return;
    }

    FWebSocketsModule& WebSocketModule = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
    GestureSocket = WebSocketModule.CreateWebSocket(GestureStreamUrl);

    GestureSocket->OnConnected().AddUObject(this, &AFusionMode::HandleWebSocketConnected);
    GestureSocket->OnConnectionError().AddUObject(this, &AFusionMode::HandleWebSocketConnectionError);
    GestureSocket->OnMessage().AddUObject(this, &AFusionMode::HandleWebSocketMessage);
    GestureSocket->OnClosed().AddUObject(this, &AFusionMode::HandleWebSocketClosed);

    LogOnScreen(ELogVerbosity::Log, TEXT("Connecting to gesture WebSocket: %s"), *GestureStreamUrl);
    GestureSocket->Connect();
}

void AFusionMode::ShutdownGestureWebSocket()
{
    if (GestureSocket.IsValid())
    {
        GestureSocket->OnConnected().RemoveAll(this);
        GestureSocket->OnConnectionError().RemoveAll(this);
        GestureSocket->OnMessage().RemoveAll(this);
        GestureSocket->OnClosed().RemoveAll(this);

        if (GestureSocket->IsConnected())
        {
            GestureSocket->Close(1000, TEXT("Shutdown"));
        }

        GestureSocket.Reset();
    }
}

void AFusionMode::HandleWebSocketConnected()
{
    LogOnScreen(ELogVerbosity::Log, TEXT("Gesture WebSocket connected."));
}

void AFusionMode::HandleWebSocketConnectionError(const FString& Error)
{
    LogOnScreen(ELogVerbosity::Error, TEXT("Gesture WebSocket error: %s"), *Error);
}

void AFusionMode::HandleWebSocketMessage(const FString& Message)
{
    LogOnScreen(ELogVerbosity::Verbose, TEXT("Gesture message received: %s"), *Message);
    OnGesturePayloadReceived.Broadcast(Message);

    FFusionGestureFrame ParsedFrame;
    if (TryParseGestureFrame(Message, ParsedFrame))
    {
        OnGestureFrameReceived.Broadcast(ParsedFrame);

        const bool bIsPointGesture = ParsedFrame.Gesture.Equals(TEXT("point"), ESearchCase::IgnoreCase) || ParsedFrame.Gesture.Equals(TEXT("select"), ESearchCase::IgnoreCase);
        if (bIsPointGesture && !ParsedFrame.ObjectHint.IsEmpty())
        {
            RequestObjectDescription(ParsedFrame.ObjectHint);
        }

        const bool bIsBackGesture = ParsedFrame.Gesture.Equals(TEXT("fist"), ESearchCase::IgnoreCase)
            || ParsedFrame.Gesture.Equals(TEXT("back"), ESearchCase::IgnoreCase)
            || ParsedFrame.Hand.Equals(TEXT("fist"), ESearchCase::IgnoreCase);
        if (bIsBackGesture)
        {
            BroadcastBackToUI();
        }
    }
}

void AFusionMode::HandleWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    LogOnScreen(ELogVerbosity::Warning, TEXT("Gesture WebSocket closed (code=%d, clean=%s): %s"), StatusCode, bWasClean ? TEXT("true") : TEXT("false"), *Reason);

    if (UWorld* World = GetWorld())
    {
        FTimerDelegate ReconnectDelegate;
        ReconnectDelegate.BindLambda([this]()
        {
            ShutdownGestureWebSocket();
            InitializeGestureWebSocket();
        });

        const float RetryDelaySeconds = 2.f;
        World->GetTimerManager().SetTimer(GestureReconnectHandle, ReconnectDelegate, RetryDelaySeconds, false);
    }
}

void AFusionMode::ScheduleGestureKeepAlive()
{
    if (GestureKeepAliveInterval <= 0.f)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(GestureKeepAliveHandle, this, &AFusionMode::SendGestureKeepAlive, GestureKeepAliveInterval, true);
}

void AFusionMode::SendGestureKeepAlive()
{
    if (!GestureSocket.IsValid() || !GestureSocket->IsConnected())
    {
        return;
    }

    static const FString PingPayload = TEXT("{\"type\":\"ping\"}");
    GestureSocket->Send(PingPayload);
}

bool AFusionMode::TryParseGestureFrame(const FString& Message, FFusionGestureFrame& OutFrame) const
{
    TSharedPtr<FJsonObject> JsonPayload;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonPayload) || !JsonPayload.IsValid())
    {
        return false;
    }

    OutFrame = FFusionGestureFrame();
    OutFrame.RawJson = Message;

    JsonPayload->TryGetStringField(TEXT("gesture"), OutFrame.Gesture);
    if (OutFrame.Gesture.IsEmpty())
    {
        JsonPayload->TryGetStringField(TEXT("hand_state"), OutFrame.Gesture);
    }

    JsonPayload->TryGetStringField(TEXT("hand"), OutFrame.Hand);
    if (OutFrame.Hand.IsEmpty())
    {
        JsonPayload->TryGetStringField(TEXT("handedness"), OutFrame.Hand);
    }

    FString ObjectHint;
    if (JsonPayload->TryGetStringField(TEXT("object_id"), ObjectHint) || JsonPayload->TryGetStringField(TEXT("object_hint"), ObjectHint))
    {
        OutFrame.ObjectHint = ObjectHint;
    }

    double ConfidenceValue = 0.0;
    if (JsonPayload->TryGetNumberField(TEXT("confidence"), ConfidenceValue))
    {
        OutFrame.Confidence = static_cast<float>(ConfidenceValue);
    }

    const TSharedPtr<FJsonObject>* FingerTipObject = nullptr;
    if (JsonPayload->TryGetObjectField(TEXT("finger_tip"), FingerTipObject) && FingerTipObject && FingerTipObject->IsValid())
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
        if ((*FingerTipObject)->TryGetNumberField(TEXT("x"), X) && (*FingerTipObject)->TryGetNumberField(TEXT("y"), Y) && (*FingerTipObject)->TryGetNumberField(TEXT("z"), Z))
        {
            OutFrame.FingerTipWorld = FVector(X, Y, Z);
            OutFrame.bHasWorldLocation = true;
        }
    }
    else if (JsonPayload->TryGetObjectField(TEXT("pointer_world"), FingerTipObject) && FingerTipObject && FingerTipObject->IsValid())
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
        if ((*FingerTipObject)->TryGetNumberField(TEXT("x"), X) && (*FingerTipObject)->TryGetNumberField(TEXT("y"), Y) && (*FingerTipObject)->TryGetNumberField(TEXT("z"), Z))
        {
            OutFrame.FingerTipWorld = FVector(X, Y, Z);
            OutFrame.bHasWorldLocation = true;
        }
    }

    const TSharedPtr<FJsonObject>* ScreenTipObject = nullptr;
    if (JsonPayload->TryGetObjectField(TEXT("screen_tip"), ScreenTipObject) && ScreenTipObject && ScreenTipObject->IsValid())
    {
        double U = 0.0;
        double V = 0.0;
        if ((*ScreenTipObject)->TryGetNumberField(TEXT("x"), U) && (*ScreenTipObject)->TryGetNumberField(TEXT("y"), V))
        {
            OutFrame.FingerTipViewport = FVector2D(U, V);
            OutFrame.bHasViewportLocation = true;
        }
    }
    else if (JsonPayload->TryGetObjectField(TEXT("pointer_viewport"), ScreenTipObject) && ScreenTipObject && ScreenTipObject->IsValid())
    {
        double U = 0.0;
        double V = 0.0;
        if ((*ScreenTipObject)->TryGetNumberField(TEXT("x"), U) && (*ScreenTipObject)->TryGetNumberField(TEXT("y"), V))
        {
            OutFrame.FingerTipViewport = FVector2D(U, V);
            OutFrame.bHasViewportLocation = true;
        }
    }

    return true;
}

void AFusionMode::RequestObjectDescription(const FString& ObjectId)
{
    if (DescribeEndpoint.IsEmpty())
    {
        LogOnScreen(ELogVerbosity::Warning, TEXT("DescribeEndpoint is empty; cannot request description."));
        return;
    }

    LogOnScreen(ELogVerbosity::Log, TEXT("Requesting description for %s"), *ObjectId);

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(DescribeEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    if (!ApiToken.IsEmpty())
    {
        Request->SetHeader(TEXT("Authorization"), ApiToken);
    }

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("object_id"), ObjectId);

    FString Payload;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(Body, Writer);
    Request->SetContentAsString(Payload);

    Request->OnProcessRequestComplete().BindUObject(this, &AFusionMode::OnDescriptionRequestComplete);
    Request->ProcessRequest();
}

void AFusionMode::SendVoiceQuery(const FString& FilePath)
{
    if (VoiceQueryEndpoint.IsEmpty())
    {
        LogOnScreen(ELogVerbosity::Warning, TEXT("VoiceQueryEndpoint is empty; cannot send voice query."));
        return;
    }

    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        LogOnScreen(ELogVerbosity::Error, TEXT("Failed to load wav file: %s"), *FilePath);
        return;
    }

    LogOnScreen(ELogVerbosity::Log, TEXT("Uploading voice query (%d bytes)"), FileData.Num());

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(VoiceQueryEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("audio/wav"));
    if (!ApiToken.IsEmpty())
    {
        Request->SetHeader(TEXT("Authorization"), ApiToken);
    }

    Request->SetContent(FileData);
    Request->OnProcessRequestComplete().BindUObject(this, &AFusionMode::OnVoiceQueryComplete);
    Request->ProcessRequest();
}

void AFusionMode::OnDescriptionRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        LogOnScreen(ELogVerbosity::Error, TEXT("Description request failed"));
        return;
    }

    const FString ResponseStr = Response->GetContentAsString();
    LogOnScreen(ELogVerbosity::Verbose, TEXT("Description response: %s"), *ResponseStr);

    TSharedPtr<FJsonObject> JsonPayload;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
    if (FJsonSerializer::Deserialize(Reader, JsonPayload) && JsonPayload.IsValid())
    {
        FString ObjectId;
        JsonPayload->TryGetStringField(TEXT("object_id"), ObjectId);

        FString Description;
        JsonPayload->TryGetStringField(TEXT("description"), Description);

        FString TtsUrl;
        JsonPayload->TryGetStringField(TEXT("tts_url"), TtsUrl);

        BroadcastDescriptionToUI(ObjectId, Description, TtsUrl);
    }
}

void AFusionMode::OnVoiceQueryComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        LogOnScreen(ELogVerbosity::Error, TEXT("Voice query request failed"));
        return;
    }

    const FString ResponseStr = Response->GetContentAsString();
    LogOnScreen(ELogVerbosity::Verbose, TEXT("Voice response: %s"), *ResponseStr);

    TSharedPtr<FJsonObject> JsonPayload;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
    if (FJsonSerializer::Deserialize(Reader, JsonPayload) && JsonPayload.IsValid())
    {
        FString Transcript;
        JsonPayload->TryGetStringField(TEXT("transcript"), Transcript);

        FString TtsUrl;
        JsonPayload->TryGetStringField(TEXT("tts_url"), TtsUrl);

        BroadcastVoiceAnswerToUI(Transcript, TtsUrl);
    }
}

void AFusionMode::BroadcastDescriptionToUI(const FString& ObjectId, const FString& Description, const FString& TtsUrl)
{
    LogOnScreen(ELogVerbosity::Log, TEXT("Broadcasting description for %s"), *ObjectId);
    OnObjectDescriptionReceived.Broadcast(ObjectId, Description, TtsUrl);
}

void AFusionMode::BroadcastVoiceAnswerToUI(const FString& Transcript, const FString& TtsUrl)
{
    LogOnScreen(ELogVerbosity::Log, TEXT("Broadcasting voice answer"));
    OnVoiceAnswerReceived.Broadcast(Transcript, TtsUrl);
}

void AFusionMode::BroadcastBackToUI()
{
    LogOnScreen(ELogVerbosity::Log, TEXT("Broadcasting back gesture"));
    OnBackRequested.Broadcast();
}
