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
#include "Dom/JsonValue.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "HandViewportMapperComponent.h"
#include "Components/Widget.h"

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

    HandViewportMapper = CreateDefaultSubobject<UHandViewportMapperComponent>(TEXT("HandViewportMapper"));
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
    // LogOnScreen(ELogVerbosity::Verbose, TEXT("Gesture message received: %s"), *Message);
    OnGesturePayloadReceived.Broadcast(Message);

    TSharedPtr<FJsonObject> JsonPayload;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonPayload) || !JsonPayload.IsValid())
    {
        return;
    }
    TArray<FFusionHandSnapshot> ParsedHands;
    PopulateHandsFromJson(JsonPayload, ParsedHands);
    OnGestureFrameReceived.Broadcast(ParsedHands);
    // if (ParsedHands.Num() > 0)
    // {
    //     FFusionWidgetHitResult HitResult;
    //     HandViewportMapper->FindWidgetAlongDirection(ParsedHands[0],7,8,1000,HitResult);
    // }

    FString ParsedGesture;
    JsonPayload->TryGetStringField(TEXT("gesture"), ParsedGesture);
    if (ParsedGesture.IsEmpty())
    {
        JsonPayload->TryGetStringField(TEXT("hand_state"), ParsedGesture);
    }

    FString ParsedHand;
    JsonPayload->TryGetStringField(TEXT("hand"), ParsedHand);
    if (ParsedHand.IsEmpty())
    {
        JsonPayload->TryGetStringField(TEXT("handedness"), ParsedHand);
    }

    FString ParsedObjectHint;
    JsonPayload->TryGetStringField(TEXT("object_id"), ParsedObjectHint);
    if (ParsedObjectHint.IsEmpty())
    {
        JsonPayload->TryGetStringField(TEXT("object_hint"), ParsedObjectHint);
    }

    const bool bIsPointGesture = ParsedGesture.Equals(TEXT("point"), ESearchCase::IgnoreCase)
        || ParsedGesture.Equals(TEXT("select"), ESearchCase::IgnoreCase);
    if (bIsPointGesture && !ParsedObjectHint.IsEmpty())
    {
        RequestObjectDescription(ParsedObjectHint);
    }

    const bool bIsBackGesture = ParsedGesture.Equals(TEXT("fist"), ESearchCase::IgnoreCase)
        || ParsedGesture.Equals(TEXT("back"), ESearchCase::IgnoreCase)
        || ParsedHand.Equals(TEXT("fist"), ESearchCase::IgnoreCase);
    if (bIsBackGesture)
    {
        BroadcastBackToUI();
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

void AFusionMode::PopulateHandsFromJson(const TSharedPtr<FJsonObject>& JsonPayload, TArray<FFusionHandSnapshot>& OutHands) const
{
    OutHands.Reset();

    if (!JsonPayload.IsValid())
    {
        return;
    }

    TArray<TSharedPtr<FJsonObject>> HandObjects;

    const TArray<TSharedPtr<FJsonValue>>* HandsArray = nullptr;
    if (JsonPayload->TryGetArrayField(TEXT("hands"), HandsArray) && HandsArray)
    {
        for (const TSharedPtr<FJsonValue>& HandValue : *HandsArray)
        {
            const TSharedPtr<FJsonObject> HandObject = HandValue.IsValid() ? HandValue->AsObject() : nullptr;
            if (HandObject.IsValid())
            {
                HandObjects.Add(HandObject);
            }
        }
    }
    else
    {
        TSharedPtr<FJsonObject> SingleHandObject;
        if (JsonPayload->HasTypedField<EJson::Object>(TEXT("hand")))
        {
            SingleHandObject = JsonPayload->GetObjectField(TEXT("hand"));
        }
        else if (JsonPayload->HasField(TEXT("x_y_z")) || JsonPayload->HasField(TEXT("state")))
        {
            SingleHandObject = JsonPayload;
        }

        if (SingleHandObject.IsValid())
        {
            HandObjects.Add(SingleHandObject);
        }
    }

    if (HandObjects.Num() == 0)
    {
        return;
    }

    OutHands.Reserve(HandObjects.Num());

    for (const TSharedPtr<FJsonObject>& HandObject : HandObjects)
    {
        if (!HandObject.IsValid())
        {
            continue;
        }

        FFusionHandSnapshot HandSnapshot;
        FString ParsedState;
        if (HandObject->TryGetStringField(TEXT("state"), ParsedState)
            || HandObject->TryGetStringField(TEXT("hand_state"), ParsedState)
            || HandObject->TryGetStringField(TEXT("gesture"), ParsedState))
        {
            HandSnapshot.state = ParsedState;
        }
        else
        {
            HandSnapshot.state.Reset();
        }

        const TArray<TSharedPtr<FJsonValue>>* CoordinatesArray = nullptr;
        if (HandObject->TryGetArrayField(TEXT("x_y_z"), CoordinatesArray) && CoordinatesArray)
        {
            HandSnapshot.x_y_z.Reset(CoordinatesArray->Num());
            HandSnapshot.x_y_z.Reserve(CoordinatesArray->Num());
            for (const TSharedPtr<FJsonValue>& CoordinateValue : *CoordinatesArray)
            {
                if (!CoordinateValue.IsValid())
                {
                    continue;
                }

                double CoordinateNumber = 0.0;
                if (CoordinateValue->TryGetNumber(CoordinateNumber))
                {
                    HandSnapshot.x_y_z.Add(static_cast<float>(CoordinateNumber));
                    continue;
                }

                const TArray<TSharedPtr<FJsonValue>>* NestedArray = nullptr;
                if (CoordinateValue->TryGetArray(NestedArray) && NestedArray)
                {
                    for (const TSharedPtr<FJsonValue>& NestedValue : *NestedArray)
                    {
                        double NestedNumber = 0.0;
                        if (NestedValue.IsValid() && NestedValue->TryGetNumber(NestedNumber))
                        {
                            HandSnapshot.x_y_z.Add(static_cast<float>(NestedNumber));
                        }
                    }
                }
            }
        }

        OutHands.Add(MoveTemp(HandSnapshot));
    }
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
