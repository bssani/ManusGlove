// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MIInteractableBase.h"
class UPhysicsConstraintComponent;

#include "MIDialInteractable.generated.h"

/// @brief Dial/knob interactable — rotational control around one axis.
/// Covers: volume knobs, temperature dials, mirror adjustment knobs.
///
/// Usage:
/// 1. Create Actor Blueprint with a StaticMesh (the knob)
/// 2. Add MIDialInteractable component
/// 3. Set RotationAxis, MinAngle, MaxAngle (or bInfiniteRotation)
/// 4. Bind OnDialRotated for value changes
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIDialInteractable : public UMIInteractableBase
{
	GENERATED_BODY()

public:
	UMIDialInteractable();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Dial Configuration ──

	/// The rotation axis for the dial.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial")
	EMIAxis RotationAxis = EMIAxis::Z;

	/// Minimum rotation angle (degrees). Ignored if bInfiniteRotation is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial", meta = (EditCondition = "!bInfiniteRotation"))
	float MinAngle = 0.0f;

	/// Maximum rotation angle (degrees). Ignored if bInfiniteRotation is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial", meta = (EditCondition = "!bInfiniteRotation"))
	float MaxAngle = 270.0f;

	/// Allow infinite rotation (like a volume knob with no stops).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial")
	bool bInfiniteRotation = false;

	/// Number of discrete steps (0 = continuous). E.g., 10 for a 10-step dial.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Steps = 0;

	/// Rotational damping.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float DialDamping = 3.0f;

	/// Mass of the dial body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Dial", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float DialMass = 0.05f;

	// ── State ──

	/// Current rotation angle (degrees).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Dial")
	float CurrentAngle = 0.0f;

	/// Current step index (if Steps > 0).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Dial")
	int32 CurrentStep = 0;

	// ── Events ──

	/// Fired when the dial rotation changes.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnDialRotated,
		float, Angle,
		float, NormalizedValue);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Dial")
	FOnDialRotated OnDialRotated;

	/// Fired when the dial snaps to a discrete step.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialStepped, int32, StepIndex);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Dial")
	FOnDialStepped OnDialStepped;

private:
	UPROPERTY()
	class UPhysicsConstraintComponent* DialConstraint = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> DialMesh;

	float PreviousAngle = 0.0f;
	int32 PreviousStep = -1;

	void SetupPhysicsConstraint();
	void UpdateAngleFromPhysics();
};
