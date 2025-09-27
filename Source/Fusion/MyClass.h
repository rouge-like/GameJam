// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/CriticalSection.h"
#include "HttpFwd.h"
#include "AudioCapture.h"
#include "MyClass.generated.h"

class IHttpRequest;
class IHttpResponse;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVoiceUploadCompleted, bool, bWasSuccessful, int32, StatusCode, const FString&, ResponseContent);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FUSION_API UMyClass : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyClass();
	// 녹음 제어
	UFUNCTION(BlueprintCallable)
	void StartRecording();

	UFUNCTION(BlueprintCallable)
	void StopRecordingAndSave(const FString& FilePath);

	UFUNCTION(BlueprintCallable)
	void UploadWavFile(const FString& FilePath);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voice Recording")
	FString VoiceUploadEndpoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voice Recording")
	FString ApiToken;

	UPROPERTY(BlueprintAssignable, Category="Voice Recording")
	FOnVoiceUploadCompleted OnVoiceUploadCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleCaptureBuffer(const float* AudioData, int32 NumSamples);
	bool EnsureAudioCaptureInitialized();

	bool WriteWavFile(const FString& FilePath, const TArray<int16>& InPCMData, int32 SampleRate, int32 NumChannels) const;
	void OnUploadCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	TArray<int16> PCMData;
	int32 CachedSampleRate;
	int32 CachedNumChannels;
	bool bIsRecording;
	FCriticalSection DataCriticalSection;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveUploadRequest;

	UPROPERTY()
	UAudioCapture* AudioCapture;

	FAudioGeneratorHandle AudioCaptureHandle;
};
