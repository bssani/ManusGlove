// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MIInteractableBase.h"
#include "MISliderInteractable.generated.h"

/// @brief Linear slider interactable — movement along one axis with range.
/// Covers: sunroof sliders, seat adjustments, temperature sliders.
///
/// Usage:
/// 1. Create Actor Blueprint with a StaticMesh (the slider handle)
/// 2. Add MISliderInteractable component
/// 3. Set SlideAxis, SlideDistance
/// 4. Bind OnSliderMoved for value changes
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMISliderInteractable : public UMIInteractableBase
{
	GENERATED_BODY()

public:
	UMISliderInteractable();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Slider Configuration ──

	/// The axis along which the slider moves (in local space).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Slider")
	EMIAxis SlideAxis = EMIAxis::Y;

	/// Total slide distance (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Slider", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float SlideDistance = 20.0f;

	/// Linear damping (friction).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Slider", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float SliderDamping = 5.0f;

	/// Mass of the slider body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Slider", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float SliderMass = 0.05f;

	/// Number of discrete steps (0 = continuous).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Slider", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Steps = 0;

	// ── State ──

	/// Current position (0.0 to 1.0 normalized).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Slider")
	float CurrentPosition = 0.0f;

	/// Current step index (if Steps > 0).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Slider")
	int32 CurrentStep = 0;

	// ── Events ──

	/// Fired when the slider position changes.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSliderMoved, float, NormalizedValue);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Slider")
	FOnSliderMoved OnSliderMoved;

	/// Fired when the slider snaps to a discrete step.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSliderStepped, int32, StepIndex);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Slider")
	FOnSliderStepped OnSliderStepped;

private:
	UPROPERTY()
	class UPhysicsConstraintComponent* SliderConstraint = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> SliderMesh;

	FVector InitialLocalPosition = FVector::ZeroVector;
	float PreviousPosition = -1.0f;
	int32 PreviousStep = -1;

	void SetupPhysicsConstraint();
	void UpdatePositionFromPhysics();
};
