// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/RecorderComponent.h"
#include "HAL/FileManager.h"
#include "Http.h"
#include "HttpModule.h"
#include "Fusion/FusionMode.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Sound/SoundSubmix.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyClass, Log, All);

// Sets default values for this component's properties
URecorderComponent::URecorderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	CachedSampleRate = 0;
	CachedNumChannels = 0;
	bIsRecording = false;
	AudioCapture = nullptr;
	AudioCaptureHandle = FAudioGeneratorHandle();
}

void URecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void URecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AudioCapture)
	{
		if (AudioCapture->IsCapturingAudio())
		{
			AudioCapture->StopCapturingAudio();
		}
		if (AudioCaptureHandle.Id != INDEX_NONE)
		{
			AudioCapture->RemoveGeneratorDelegate(AudioCaptureHandle);
			AudioCaptureHandle = FAudioGeneratorHandle();
		}
	}

	ActiveUploadRequest.Reset();

	Super::EndPlay(EndPlayReason);
}

void URecorderComponent::HandleCaptureBuffer(const float* AudioData, int32 NumSamples)
{
	if (!AudioData || NumSamples <= 0)
	{
		return;
	}

	FScopeLock Lock(&DataCriticalSection);
	if (!bIsRecording)
	{
		return;
	}

	PCMData.Reserve(PCMData.Num() + NumSamples);
	for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		const float Clamped = FMath::Clamp(AudioData[SampleIndex], -1.0f, 1.0f);
		PCMData.Add(static_cast<int16>(Clamped * 32767.0f));
	}
}

bool URecorderComponent::EnsureAudioCaptureInitialized()
{
	if (!AudioCapture)
	{
		AudioCapture = NewObject<UAudioCapture>(this);
		if (!AudioCapture)
		{
			return false;
		}

		if (!AudioCapture->OpenDefaultAudioStream())
		{
			UE_LOG(LogMyClass, Error, TEXT("Unable to open default audio capture stream."));
			AudioCapture = nullptr;
			return false;
		}
	}

	if (AudioCaptureHandle.Id == INDEX_NONE)
	{
		TWeakObjectPtr<URecorderComponent> WeakThis = this;
		AudioCaptureHandle = AudioCapture->AddGeneratorDelegate([WeakThis](const float* InAudio, int32 NumSamples)
		{
			if (URecorderComponent* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleCaptureBuffer(InAudio, NumSamples);
			}
		});
	}

	CachedSampleRate = AudioCapture->GetSampleRate();
	CachedNumChannels = AudioCapture->GetNumChannels();

	return AudioCapture->GetNumChannels() > 0 && AudioCapture->GetSampleRate() > 0;
}

void URecorderComponent::StartRecording()
{
	if (!EnsureAudioCaptureInitialized())
	{
		UE_LOG(LogMyClass, Error, TEXT("Failed to initialise audio capture device."));
		OnVoiceUploadCompleted.Broadcast(false, 0, TEXT("Audio capture initialisation failed"));
		return;
	}

	FScopeLock Lock(&DataCriticalSection);
	PCMData.Reset();
	CachedSampleRate = AudioCapture ? AudioCapture->GetSampleRate() : 0;
	CachedNumChannels = AudioCapture ? AudioCapture->GetNumChannels() : 0;
	bIsRecording = true;

	if (AudioCapture && !AudioCapture->IsCapturingAudio())
	{
		AudioCapture->StartCapturingAudio();
	}
}

void URecorderComponent::StopRecordingAndSave(const FString& FilePath)
{
	if (AudioCapture && AudioCapture->IsCapturingAudio())
	{
		AudioCapture->StopCapturingAudio();
	}

	TArray<int16> LocalPCMData;
	int32 SampleRate = 0;
	int32 NumChannels = 0;

	{
		FScopeLock Lock(&DataCriticalSection);
		if (!bIsRecording && PCMData.Num() == 0)
		{
			UE_LOG(LogMyClass, Warning, TEXT("StopRecordingAndSave called without active recording or captured data."));
		}

		bIsRecording = false;
		LocalPCMData = PCMData;
		SampleRate = CachedSampleRate;
		NumChannels = CachedNumChannels;
	}

	if (LocalPCMData.Num() == 0 || SampleRate <= 0 || NumChannels <= 0)
	{
		UE_LOG(LogMyClass, Warning, TEXT("No audio data captured; skipping save."));
		OnVoiceUploadCompleted.Broadcast(false, 0, TEXT("No audio data captured"));
		return;
	}

	FString ResolvedPath = FilePath;
	if (FPaths::GetPath(ResolvedPath).IsEmpty())
	{
		ResolvedPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Recordings"), ResolvedPath);
	}
	else if (FPaths::IsRelative(ResolvedPath))
	{
		ResolvedPath = FPaths::ConvertRelativePathToFull(ResolvedPath);
	}

	if (!ResolvedPath.EndsWith(TEXT(".wav"), ESearchCase::IgnoreCase))
	{
		ResolvedPath.Append(TEXT(".wav"));
	}

	if (!WriteWavFile(ResolvedPath, LocalPCMData, SampleRate, NumChannels))
	{
		UE_LOG(LogMyClass, Error, TEXT("Failed to write wav file: %s"), *ResolvedPath);
		OnVoiceUploadCompleted.Broadcast(false, 0, TEXT("Failed to write wav file"));
		return;
	}

	UE_LOG(LogMyClass, Log, TEXT("Saved wav file to %s"), *ResolvedPath);
	UploadWavFile(ResolvedPath);
}

void URecorderComponent::UploadWavFile(const FString& FilePath)
{
	// if (VoiceUploadEndpoint.IsEmpty())
	// {
	// 	UE_LOG(LogMyClass, Warning, TEXT("VoiceUploadEndpoint is not set; skipping upload."));
	// 	OnVoiceUploadCompleted.Broadcast(false, 0, TEXT("VoiceUploadEndpoint not configured"));
	// 	return;
	// }

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
	{
		UE_LOG(LogMyClass, Error, TEXT("Failed to load wav file for upload: %s"), *FilePath);
		OnVoiceUploadCompleted.Broadcast(false, 0, TEXT("Failed to load wav file"));
		return;
	}

	UE_LOG(LogMyClass, Log, TEXT("Uploading wav file (%d bytes) to %s"), FileData.Num(), *VoiceUploadEndpoint);

	AGameModeBase* GM = UGameplayStatics::GetGameMode(GetWorld());

	if (GM)
	{
		AFusionMode* FM = Cast<AFusionMode>(GM);
		FM->SendVoiceQuery(FilePath);
		return;
	}
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(VoiceUploadEndpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("audio/wav"));
	if (!ApiToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), ApiToken);
	}

	Request->SetContent(MoveTemp(FileData));
	Request->OnProcessRequestComplete().BindUObject(this, &URecorderComponent::OnUploadCompleted);
	ActiveUploadRequest = Request;
	Request->ProcessRequest();
}

bool URecorderComponent::WriteWavFile(const FString& FilePath, const TArray<int16>& InPCMData, int32 SampleRate,
	int32 NumChannels) const
{
	const int32 NumAudioBytes = InPCMData.Num() * sizeof(int16);
	const uint16 BitsPerSample = 16;
	const uint16 BlockAlign = NumChannels * (BitsPerSample / 8);
	const uint32 ByteRate = SampleRate * BlockAlign;

	TArray<uint8> WavData;
	WavData.Reserve(44 + NumAudioBytes);

	auto AppendAnsi = [&WavData](const ANSICHAR* Text, int32 Length)
	{
		for (int32 Index = 0; Index < Length; ++Index)
		{
			WavData.Add(static_cast<uint8>(Text[Index]));
		}
	};

	auto AppendUint32 = [&WavData](uint32 Value)
	{
		WavData.Add(static_cast<uint8>(Value & 0xFF));
		WavData.Add(static_cast<uint8>((Value >> 8) & 0xFF));
		WavData.Add(static_cast<uint8>((Value >> 16) & 0xFF));
		WavData.Add(static_cast<uint8>((Value >> 24) & 0xFF));
	};

	auto AppendUint16 = [&WavData](uint16 Value)
	{
		WavData.Add(static_cast<uint8>(Value & 0xFF));
		WavData.Add(static_cast<uint8>((Value >> 8) & 0xFF));
	};

	AppendAnsi("RIFF", 4);
	AppendUint32(36 + NumAudioBytes);
	AppendAnsi("WAVE", 4);
	AppendAnsi("fmt ", 4);
	AppendUint32(16);
	AppendUint16(1);
	AppendUint16(static_cast<uint16>(NumChannels));
	AppendUint32(static_cast<uint32>(SampleRate));
	AppendUint32(ByteRate);
	AppendUint16(BlockAlign);
	AppendUint16(BitsPerSample);
	AppendAnsi("data", 4);
	AppendUint32(NumAudioBytes);

	const uint8* PCMBytes = reinterpret_cast<const uint8*>(InPCMData.GetData());
	WavData.Append(PCMBytes, NumAudioBytes);

	const FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	return FFileHelper::SaveArrayToFile(WavData, *FilePath);
}

void URecorderComponent::OnUploadCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	ActiveUploadRequest.Reset();

	ActiveUploadRequest.Reset();

	const int32 StatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	const FString ResponseContent = Response.IsValid() ? Response->GetContentAsString() : FString();
	const bool bSucceeded = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(StatusCode);

	if (bSucceeded)
	{
		UE_LOG(LogMyClass, Log, TEXT("Voice upload succeeded with status %d"), StatusCode);
	}
	else
	{
		UE_LOG(LogMyClass, Error, TEXT("Voice upload failed (status %d, bWasSuccessful=%s)"), StatusCode, bWasSuccessful ? TEXT("true") : TEXT("false"));
	}

	OnVoiceUploadCompleted.Broadcast(bSucceeded, StatusCode, ResponseContent);
}





