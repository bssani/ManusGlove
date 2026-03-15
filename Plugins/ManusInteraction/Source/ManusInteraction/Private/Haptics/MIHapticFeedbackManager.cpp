// Copyright 2024-2025 ManusInteraction Plugin

#include "Haptics/MIHapticFeedbackManager.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusComponent.h"
#include "ManusBlueprintLibrary.h"
#include "ManusInteraction.h"

UMIHapticFeedbackManager::UMIHapticFeedbackManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UMIHapticFeedbackManager::Initialize(
	UManusComponent* InSourceComponent,
	const TArray<UMIPhysicsFingerComponent*>& InFingers,
	EMIHandSide InHandSide)
{
	SourceManusComponent = InSourceComponent;
	FingerComponents = InFingers;
	HandSide = InHandSide;

	CheckHapticSupport();
}

void UMIHapticFeedbackManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled || !bGloveSupportsHaptics || !SourceManusComponent.IsValid())
	{
		return;
	}

	// Collect per-finger haptic intensities
	float Powers[(int32)EMIFingerName::Max] = { 0.0f };

	for (const UMIPhysicsFingerComponent* Finger : FingerComponents)
	{
		if (!Finger)
		{
			continue;
		}

		const int32 FingerIndex = static_cast<int32>(Finger->FingerName);
		if (FingerIndex < 0 || FingerIndex >= static_cast<int32>(EMIFingerName::Max))
		{
			continue;
		}

		float Intensity = 0.0f;

		// Divergence-based haptics
		if (Finger->CurrentDivergence > MinDivergenceForHaptic)
		{
			Intensity = CalculateHapticIntensity(Finger->CurrentDivergence);
		}

		// Contact-based haptics (override if stronger)
		if (bHapticOnContact && Finger->bIsInContact)
		{
			Intensity = FMath::Max(Intensity, ContactHapticIntensity);
		}

		Powers[FingerIndex] = FMath::Clamp(Intensity * IntensityMultiplier, 0.0f, 1.0f);
	}

	// Send haptic command to the Manus glove
	const EManusHandType ManusHandType = (HandSide == EMIHandSide::Left) ? EManusHandType::Left : EManusHandType::Right;
	SourceManusComponent->VibrateManusGloveFingers(
		ManusHandType,
		Powers[(int32)EMIFingerName::Thumb],
		Powers[(int32)EMIFingerName::Index],
		Powers[(int32)EMIFingerName::Middle],
		Powers[(int32)EMIFingerName::Ring],
		Powers[(int32)EMIFingerName::Pinky]);
}

void UMIHapticFeedbackManager::TriggerFingerHaptic(EMIFingerName Finger, float Intensity)
{
	if (!bEnabled || !bGloveSupportsHaptics || !SourceManusComponent.IsValid())
	{
		return;
	}

	float Powers[(int32)EMIFingerName::Max] = { 0.0f };
	const int32 FingerIndex = static_cast<int32>(Finger);
	if (FingerIndex >= 0 && FingerIndex < static_cast<int32>(EMIFingerName::Max))
	{
		Powers[FingerIndex] = FMath::Clamp(Intensity * IntensityMultiplier, 0.0f, 1.0f);
	}

	const EManusHandType ManusHandType = (HandSide == EMIHandSide::Left) ? EManusHandType::Left : EManusHandType::Right;
	SourceManusComponent->VibrateManusGloveFingers(
		ManusHandType,
		Powers[(int32)EMIFingerName::Thumb],
		Powers[(int32)EMIFingerName::Index],
		Powers[(int32)EMIFingerName::Middle],
		Powers[(int32)EMIFingerName::Ring],
		Powers[(int32)EMIFingerName::Pinky]);
}

void UMIHapticFeedbackManager::CheckHapticSupport()
{
	if (!SourceManusComponent.IsValid())
	{
		bGloveSupportsHaptics = false;
		return;
	}

	const int64 SkeletonId = SourceManusComponent->GetManusSkeletonId();
	const bool bIsLeft = (HandSide == EMIHandSide::Left);
	bGloveSupportsHaptics = UManusBlueprintLibrary::DoesSkeletonHaveHaptics(SkeletonId, bIsLeft);

	if (!bGloveSupportsHaptics)
	{
		UE_LOG(LogManusInteraction, Log, TEXT("MIHapticFeedbackManager: Glove does not support haptics. Haptic feedback disabled."));
	}
}

float UMIHapticFeedbackManager::CalculateHapticIntensity(float Divergence) const
{
	if (Divergence <= MinDivergenceForHaptic)
	{
		return 0.0f;
	}

	const float Range = MaxDivergenceForFullHaptic - MinDivergenceForHaptic;
	if (Range <= 0.0f)
	{
		return 1.0f;
	}

	const float Normalized = (Divergence - MinDivergenceForHaptic) / Range;
	return FMath::Clamp(Normalized, 0.0f, 1.0f);
}
