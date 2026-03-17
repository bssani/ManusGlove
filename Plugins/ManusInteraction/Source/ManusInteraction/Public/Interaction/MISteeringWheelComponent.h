// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"

class AMIPhysicsHand;

#include "MISteeringWheelComponent.generated.h"

/// @brief Steering wheel interactable component.
/// Supports one or two hands simultaneously, preserves angle on hand-off,
/// auto-returns to center on release, and locks at a configurable max angle.
///
/// Usage:
/// 1. Create Actor Blueprint (the steering wheel actor)
/// 2. Add a Static Mesh component (the wheel mesh)
/// 3. Add MISteeringWheelComponent
/// 4. Assign LeftHand, RightHand (AMIPhysicsHand actors) and WheelMesh
/// 5. Set MaxLockAngle, WheelRotationAxis to match your mesh
/// 6. Bind OnWheelRotated for game logic (e.g. vehicle steering input)
///
/// Design: direct rotation (SetRelativeRotation) — no physics constraint.
/// Angle delta is calculated per-tick from hand position relative to wheel center.
/// This avoids physics fighting and makes multi-hand handoff trivial.
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMISteeringWheelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMISteeringWheelComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ── Hand References ──────────────────────────────────────────────────────

	/// Left physics hand actor. Assign in editor or at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	TObjectPtr<AMIPhysicsHand> LeftHand;

	/// Right physics hand actor. Assign in editor or at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	TObjectPtr<AMIPhysicsHand> RightHand;

	/// The mesh component to rotate. Must be on the same Actor as this component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	TObjectPtr<UPrimitiveComponent> WheelMesh;

	// ── Configuration ────────────────────────────────────────────────────────

	/// Maximum lock angle in each direction (degrees).
	/// E.g. 450 means the wheel can rotate -450° to +450°.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel",
		meta = (ClampMin = "45.0", ClampMax = "1440.0"))
	float MaxLockAngle = 450.0f;

	/// Automatically return to 0° when no hand is grabbing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	bool bAutoReturn = true;

	/// Return speed toward center when released (degrees per second).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel",
		meta = (EditCondition = "bAutoReturn", ClampMin = "1.0", ClampMax = "720.0"))
	float ReturnSpeed = 180.0f;

	/// Minimum number of fingers that must contact the wheel actor to register a grab.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel",
		meta = (ClampMin = "1", ClampMax = "5"))
	int32 MinGrabFingers = 2;

	/// Thumb must be one of the contacting fingers to grab.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	bool bRequireThumb = true;

	/// Local axis around which the wheel rotates.
	/// X = roll (typical steering wheel facing forward), Z = yaw (horizontal wheel).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel")
	EMIAxis WheelRotationAxis = EMIAxis::X;

	/// Minimum distance from wheel center (cm) for a hand to contribute rotation.
	/// Prevents jitter when hand is near the center.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|SteeringWheel",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float MinGrabRadius = 4.0f;

	// ── Runtime State ────────────────────────────────────────────────────────

	/// Current wheel angle in degrees. Positive = clockwise from driver's perspective.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|SteeringWheel")
	float CurrentAngle = 0.0f;

	/// True while the left hand is grabbing the wheel.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|SteeringWheel")
	bool bLeftHandGrabbing = false;

	/// True while the right hand is grabbing the wheel.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|SteeringWheel")
	bool bRightHandGrabbing = false;

	// ── Events ───────────────────────────────────────────────────────────────

	/// Fires each tick while the wheel is rotating or returning.
	/// @param Angle     Current angle in degrees.
	/// @param Normalized  -1.0 (full left) to +1.0 (full right).
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWheelRotated, float, Angle, float, Normalized);
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|SteeringWheel")
	FOnWheelRotated OnWheelRotated;

	/// Fires when a hand starts grabbing the wheel.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWheelHandGrabbed, EMIHandSide, HandSide);
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|SteeringWheel")
	FOnWheelHandGrabbed OnHandGrabbed;

	/// Fires when a hand releases the wheel.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWheelHandReleased, EMIHandSide, HandSide);
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|SteeringWheel")
	FOnWheelHandReleased OnHandReleased;

	/// Fires when the wheel reaches MaxLockAngle.
	/// @param bAtPositiveLimit  True = hit +MaxLockAngle, False = hit -MaxLockAngle.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWheelLocked, bool, bAtPositiveLimit);
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|SteeringWheel")
	FOnWheelLocked OnWheelLocked;

	// ── Blueprint Functions ──────────────────────────────────────────────────

	/// Current angle in degrees.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|SteeringWheel")
	float GetCurrentAngle() const { return CurrentAngle; }

	/// Normalized value: -1.0 (full left lock) to +1.0 (full right lock).
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|SteeringWheel")
	float GetNormalizedValue() const;

	/// True if either hand is currently grabbing.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|SteeringWheel")
	bool IsAnyHandGrabbing() const { return bLeftHandGrabbing || bRightHandGrabbing; }

	/// Programmatically set wheel angle. Clamped to ±MaxLockAngle.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|SteeringWheel")
	void SetAngle(float NewAngle);

private:
	// Per-hand last-frame angular position on the wheel plane (degrees).
	float LastHandAngle_Left  = 0.0f;
	float LastHandAngle_Right = 0.0f;

	// Previous-frame grab states for edge-detection.
	bool bWasLeftGrabbing  = false;
	bool bWasRightGrabbing = false;

	// Initial local rotation of WheelMesh saved at BeginPlay.
	FRotator BaseRotation = FRotator::ZeroRotator;

	// Whether we fired OnWheelLocked this frame (prevent repeated fires).
	bool bLockedLastFrame = false;

	/// Returns true if the hand has enough fingers contacting the wheel actor.
	bool CheckHandGrabbing(AMIPhysicsHand* Hand) const;

	/// Returns the hand's angular position (degrees) around the wheel's rotation axis.
	/// Uses the palm sphere position in wheel-local space.
	float GetHandAngleOnWheel(AMIPhysicsHand* Hand) const;

	/// Returns the 2D distance of the hand from the wheel center (wheel-local plane, cm).
	float GetHandRadiusOnWheel(AMIPhysicsHand* Hand) const;

	/// Apply CurrentAngle to WheelMesh via SetRelativeRotation.
	void ApplyAngleToMesh() const;

	/// Handle grab-start or grab-end for a single hand.
	void HandleGrabStateChange(EMIHandSide Side, bool bNowGrabbing, AMIPhysicsHand* Hand);
};
