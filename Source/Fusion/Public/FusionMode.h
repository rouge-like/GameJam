#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "FusionMode.generated.h"

class IWebSocket;
class FJsonObject;
class FJsonValue;
class UHandViewportMapperComponent;

USTRUCT(BlueprintType)
struct FFusionHandLandmark
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Fusion|Gestures")
    int32 Id = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Fusion|Gestures")
    FVector Location = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FFusionHandSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Fusion|Gestures")
    FString Handedness;

    UPROPERTY(BlueprintReadOnly, Category = "Fusion|Gestures")
    float Score = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Fusion|Gestures")
    TArray<FFusionHandLandmark> Landmarks;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectDescriptionReceived, const FString&, ObjectId, const FString&, Description, const FString&, TtsUrl);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceAnswerReceived, const FString&, Transcript, const FString&, TtsUrl);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGesturePayloadReceived, const FString&, RawMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGestureFrameReceived, const TArray<FFusionHandSnapshot>&, Hands);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBackRequested);

/**
 * AFusionMode centralises all AI ↔ Unreal communication for gesture streams, description lookups, and voice queries.
 * It exposes Blueprint events so UX blueprints can react without duplicating networking code.
 */
UCLASS()
class FUSION_API AFusionMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AFusionMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Requests a description payload for the provided object id via REST. */
    UFUNCTION(BlueprintCallable, Category = "Fusion|Networking")
    void RequestObjectDescription(const FString& ObjectId);

    /** Sends a recorded wav file to the voice query endpoint for LLM processing. */
    UFUNCTION(BlueprintCallable, Category = "Fusion|Networking")
    void SendVoiceQuery(const FString& FilePath);

    /** Broadcast whenever a gesture frame arrives over the WebSocket. */
    UPROPERTY(BlueprintAssignable, Category = "Fusion|Events")
    FOnGesturePayloadReceived OnGesturePayloadReceived;

    /** Broadcast when a gesture payload is parsed into structured data. */
    UPROPERTY(BlueprintAssignable, Category = "Fusion|Events")
    FOnGestureFrameReceived OnGestureFrameReceived;

    /** Broadcast when the description endpoint returns data. */
    UPROPERTY(BlueprintAssignable, Category = "Fusion|Events")
    FOnObjectDescriptionReceived OnObjectDescriptionReceived;

    /** Broadcast when the voice query endpoint returns an answer. */
    UPROPERTY(BlueprintAssignable, Category = "Fusion|Events")
    FOnVoiceAnswerReceived OnVoiceAnswerReceived;

    /** Broadcast when a back/fist gesture is detected. */
    UPROPERTY(BlueprintAssignable, Category = "Fusion|Events")
    FOnBackRequested OnBackRequested;

protected:
    /** WebSocket bootstrapping and teardown. */
    void InitializeGestureWebSocket();
    void ShutdownGestureWebSocket();

    void HandleWebSocketConnected();
    void HandleWebSocketConnectionError(const FString& Error);
    void HandleWebSocketMessage(const FString& Message);
    void HandleWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);

    void ScheduleGestureKeepAlive();
    void SendGestureKeepAlive();

    void OnDescriptionRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnVoiceQueryComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void BroadcastDescriptionToUI(const FString& ObjectId, const FString& Description, const FString& TtsUrl);
    void BroadcastVoiceAnswerToUI(const FString& Transcript, const FString& TtsUrl);
    void BroadcastBackToUI();

    void PopulateHandsFromJson(const TSharedPtr<FJsonObject>& JsonPayload, TArray<FFusionHandSnapshot>& OutHands) const;

protected:
    /** WebSocket URL supplying gesture frames and hand state. */
    UPROPERTY(EditDefaultsOnly, Category = "Fusion|Networking")
    FString GestureStreamUrl;

    /** REST endpoint for triggering description requests. */
    UPROPERTY(EditDefaultsOnly, Category = "Fusion|Networking")
    FString DescribeEndpoint;

    /** REST endpoint for uploading wav voice queries. */
    UPROPERTY(EditDefaultsOnly, Category = "Fusion|Networking")
    FString VoiceQueryEndpoint;

    /** Interval in seconds for sending lightweight keep-alive pings over the WebSocket. */
    UPROPERTY(EditDefaultsOnly, Category = "Fusion|Networking", meta = (ClampMin = "0.1"))
    float GestureKeepAliveInterval;

    /** Optional token or API key forwarded with REST requests. */
    UPROPERTY(EditDefaultsOnly, Category = "Fusion|Networking")
    FString ApiToken;

private:
    FTimerHandle GestureKeepAliveHandle;
    FTimerHandle GestureReconnectHandle;
    TSharedPtr<IWebSocket> GestureSocket;

    void LogOnScreen(ELogVerbosity::Type Verbosity, const TCHAR* Format, ...) const;
    FColor GetLogColor(ELogVerbosity::Type Verbosity) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fusion|Mapping", meta = (AllowPrivateAccess = "true"))
    UHandViewportMapperComponent* HandViewportMapper;
};
