// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MIInteractableBase.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MIHingeInteractable.generated.h"

/// @brief Hinge-based interactable — single-axis rotation with constraints.
/// Covers: turn signal levers, wiper levers, door handles, sun visors,
///         glove box lids, center console lids, etc.
///
/// Usage:
/// 1. Create Actor Blueprint with a StaticMesh (the rotating part)
/// 2. Add MIHingeInteractable component
/// 3. Set Axis, MinAngle, MaxAngle, optional SnapPositions
/// 4. The mesh will rotate around the specified axis when pushed by physics fingers
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIHingeInteractable : public UMIInteractableBase
{
	GENERATED_BODY()

public:
	UMIHingeInteractable();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Hinge Configuration ──

	/// The rotation axis for the hinge.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge")
	EMIAxis HingeAxis = EMIAxis::Y;

	/// Minimum rotation angle (degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge")
	float MinAngle = -30.0f;

	/// Maximum rotation angle (degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge")
	float MaxAngle = 30.0f;

	/// Whether the hinge automatically returns to rest position when released.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge")
	bool bAutoReturn = false;

	/// Speed of auto-return (degrees per second).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge", meta = (EditCondition = "bAutoReturn", ClampMin = "1.0", ClampMax = "720.0"))
	float ReturnSpeed = 90.0f;

	/// Rest angle for auto-return (degrees). 0 = center.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge", meta = (EditCondition = "bAutoReturn"))
	float RestAngle = 0.0f;

	/// Angular damping on the hinge body.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float HingeDamping = 5.0f;

	/// Mass of the hinge body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge", meta = (ClampMin = "0.01", ClampMax = "10.0"))
	float HingeMass = 0.1f;

	/// Snap positions as normalized values (0.0 to 1.0).
	/// Example: {0.0, 0.5, 1.0} = three snap positions (min, center, max).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge")
	TArray<float> SnapPositions;

	/// Force required to move past a snap position (N).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Hinge", meta = (EditCondition = "SnapPositions.Num() > 0", ClampMin = "0.0", ClampMax = "100.0"))
	float SnapForce = 5.0f;

	// ── State ──

	/// Current angle in degrees.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Hinge")
	float CurrentAngle = 0.0f;

	/// The snap position index the hinge is currently at (-1 if not snapped).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Hinge")
	int32 CurrentSnapIndex = -1;

	// ── Events ──

	/// Fired when the hinge snaps to a position.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnHingeSnapped,
		int32, SnapIndex,
		float, NormalizedValue);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Hinge")
	FOnHingeSnapped OnHingeSnapped;

	// ── Public API ──

	/// Get the current angle in degrees.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Hinge")
	float GetCurrentAngle() const { return CurrentAngle; }

	/// Set the angle programmatically (degrees).
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Hinge")
	void SetAngle(float Angle);

private:
	/// The physics constraint for the hinge.
	UPROPERTY()
	UPhysicsConstraintComponent* HingeConstraint = nullptr;

	/// Reference to the mesh being constrained.
	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> ConstrainedMesh;

	/// Setup the physics constraint.
	void SetupPhysicsConstraint();

	/// Update current angle from the physics body.
	void UpdateAngleFromPhysics();

	/// Find the nearest snap position and apply it.
	void ApplySnapping();

	/// Convert angle to normalized value (0-1).
	float AngleToNormalized(float Angle) const;

	/// Convert normalized value to angle.
	float NormalizedToAngle(float Normalized) const;
};
