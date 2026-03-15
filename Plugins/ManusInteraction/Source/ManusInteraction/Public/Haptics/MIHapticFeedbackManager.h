// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIHapticFeedbackManager.generated.h"

class UManusComponent;
class UMIPhysicsFingerComponent;

/// @brief Maps physics finger contact/divergence to Manus haptic vibration.
/// Automatically checks if the glove supports haptics. If not, the component
/// does nothing — safe to keep enabled regardless of glove model.
UCLASS(ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIHapticFeedbackManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIHapticFeedbackManager();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// Enable/disable haptic feedback entirely.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics")
	bool bEnabled = true;

	/// Overall intensity multiplier (0.0 to 1.0).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IntensityMultiplier = 1.0f;

	/// Minimum divergence (cm) before haptic feedback starts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MinDivergenceForHaptic = 0.5f;

	/// Divergence (cm) at which haptic feedback reaches full intensity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float MaxDivergenceForFullHaptic = 3.0f;

	/// Whether to also trigger haptics on initial contact (regardless of divergence).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics")
	bool bHapticOnContact = true;

	/// Intensity for contact-based haptic pulse (0.0 to 1.0).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Haptics", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bHapticOnContact"))
	float ContactHapticIntensity = 0.7f;

	// ── Setup ──

	/// Initialize with source component and finger references.
	void Initialize(UManusComponent* InSourceComponent, const TArray<UMIPhysicsFingerComponent*>& InFingers, EMIHandSide InHandSide);

	/// Manually trigger haptic feedback for a specific finger.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Haptics")
	void TriggerFingerHaptic(EMIFingerName Finger, float Intensity = 1.0f);

private:
	UPROPERTY()
	TWeakObjectPtr<UManusComponent> SourceManusComponent;

	UPROPERTY()
	TArray<UMIPhysicsFingerComponent*> FingerComponents;

	EMIHandSide HandSide = EMIHandSide::Left;

	/// Whether the glove supports haptics (cached at init).
	bool bGloveSupportsHaptics = false;

	/// Check if the connected glove supports haptics.
	void CheckHapticSupport();

	/// Calculate haptic intensity from divergence.
	float CalculateHapticIntensity(float Divergence) const;
};
