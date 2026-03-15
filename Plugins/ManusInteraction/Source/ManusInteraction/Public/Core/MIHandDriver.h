// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIHandDriver.generated.h"

class UMIPhysicsFingerComponent;
class UManusComponent;

/// @brief Drives physics finger bodies toward their kinematic targets each tick.
/// Uses velocity-based driving by default — the standard approach in VR physics hand systems.
UCLASS(ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIHandDriver : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIHandDriver();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// Maximum linear speed for driving finger bodies (cm/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandDriver", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float MaxLinearSpeed = 800.0f;

	/// Maximum angular speed for driving finger bodies (deg/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandDriver", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float MaxAngularSpeed = 1080.0f;

	/// Drive mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandDriver")
	EMIHandDriveMode DriveMode = EMIHandDriveMode::Velocity;

	/// Force multiplier when using Force drive mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandDriver", meta = (EditCondition = "DriveMode == EMIHandDriveMode::Force", ClampMin = "1.0", ClampMax = "10000.0"))
	float ForceMultiplier = 1000.0f;

	/// Divergence threshold to trigger OnDivergenceExceeded (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandDriver", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float DivergenceThreshold = 5.0f;

	// ── Runtime State ──

	/// The maximum divergence across all fingers this frame.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|HandDriver")
	float MaxCurrentDivergence = 0.0f;

	// ── Events ──

	/// Fired when a finger's divergence exceeds the threshold.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnHandDivergence,
		EMIFingerName, Finger,
		float, Distance);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|HandDriver")
	FOnHandDivergence OnDivergenceExceeded;

	// ── Setup ──

	/// Initialize the driver with the source component and finger array.
	void Initialize(UManusComponent* InSourceComponent, const TArray<UMIPhysicsFingerComponent*>& InFingers, UPrimitiveComponent* InPalm);

private:
	/// Reference to the Manus skeletal mesh component (bone transform source).
	UPROPERTY()
	TWeakObjectPtr<UManusComponent> SourceManusComponent;

	/// References to all finger physics components.
	UPROPERTY()
	TArray<UMIPhysicsFingerComponent*> FingerComponents;

	/// Reference to the palm physics body.
	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> PalmComponent;

	/// Drive a single physics body toward a target transform.
	void DriveBodyToTarget(UPrimitiveComponent* Body, const FVector& TargetLocation, const FQuat& TargetRotation, float DeltaTime);
};
