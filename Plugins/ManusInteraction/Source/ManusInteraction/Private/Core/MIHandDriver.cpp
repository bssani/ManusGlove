// Copyright 2024-2025 ManusInteraction Plugin

#include "Core/MIHandDriver.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusComponent.h"
#include "ManusInteraction.h"

UMIHandDriver::UMIHandDriver()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UMIHandDriver::Initialize(UManusComponent* InSourceComponent, const TArray<UMIPhysicsFingerComponent*>& InFingers, UPrimitiveComponent* InPalm)
{
	SourceManusComponent = InSourceComponent;
	FingerComponents = InFingers;
	PalmComponent = InPalm;
}

void UMIHandDriver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!SourceManusComponent.IsValid() || DeltaTime <= 0.0f)
	{
		return;
	}

	UManusComponent* Source = SourceManusComponent.Get();
	MaxCurrentDivergence = 0.0f;

	// Drive each finger toward its bone target
	for (UMIPhysicsFingerComponent* Finger : FingerComponents)
	{
		if (!Finger || Finger->TargetBoneName.IsNone())
		{
			continue;
		}

		// Get the bone transform from the Manus kinematic hand
		const FTransform BoneTransform = Source->GetSocketTransform(Finger->TargetBoneName, RTS_World);
		const FVector TargetLocation = BoneTransform.GetLocation();
		const FQuat TargetRotation = BoneTransform.GetRotation();

		// Update the finger's kinematic target (for divergence calculation)
		Finger->SetKinematicTarget(TargetLocation, TargetRotation);

		// Drive the physics body toward the target
		DriveBodyToTarget(Finger, TargetLocation, TargetRotation, DeltaTime);

		// Track divergence
		const float Divergence = Finger->CurrentDivergence;
		MaxCurrentDivergence = FMath::Max(MaxCurrentDivergence, Divergence);

		if (Divergence > DivergenceThreshold)
		{
			OnDivergenceExceeded.Broadcast(Finger->FingerName, Divergence);
		}
	}

	// Drive palm body if available
	if (PalmComponent.IsValid() && SourceManusComponent.IsValid())
	{
		// Use the wrist/root bone for palm positioning
		// The exact bone name depends on the skeleton setup — can be configured
		const FTransform RootTransform = Source->GetComponentTransform();
		DriveBodyToTarget(PalmComponent.Get(), RootTransform.GetLocation(), RootTransform.GetRotation(), DeltaTime);
	}
}

void UMIHandDriver::DriveBodyToTarget(UPrimitiveComponent* Body, const FVector& TargetLocation, const FQuat& TargetRotation, float DeltaTime)
{
	if (!Body || !Body->IsSimulatingPhysics())
	{
		return;
	}

	const FVector CurrentLocation = Body->GetComponentLocation();
	const FQuat CurrentRotation = Body->GetComponentQuat();

	if (DriveMode == EMIHandDriveMode::Velocity)
	{
		// ── Velocity Drive ──
		// Calculate the velocity needed to reach the target in one frame,
		// then clamp to MaxLinearSpeed to prevent instability.
		FVector DesiredVelocity = (TargetLocation - CurrentLocation) / DeltaTime;

		const float SpeedSquared = DesiredVelocity.SizeSquared();
		const float MaxSpeedSquared = MaxLinearSpeed * MaxLinearSpeed;
		if (SpeedSquared > MaxSpeedSquared)
		{
			DesiredVelocity = DesiredVelocity.GetSafeNormal() * MaxLinearSpeed;
		}

		Body->SetPhysicsLinearVelocity(DesiredVelocity);

		// Angular velocity
		const FQuat DeltaRotation = TargetRotation * CurrentRotation.Inverse();
		FVector Axis;
		float Angle;
		DeltaRotation.ToAxisAndAngle(Axis, Angle);

		// Normalize angle to [-PI, PI]
		if (Angle > PI)
		{
			Angle -= 2.0f * PI;
		}

		const float MaxAngularSpeedRad = FMath::DegreesToRadians(MaxAngularSpeed);
		float DesiredAngularSpeed = Angle / DeltaTime;
		DesiredAngularSpeed = FMath::Clamp(DesiredAngularSpeed, -MaxAngularSpeedRad, MaxAngularSpeedRad);

		// Convert to angular velocity vector (radians/s)
		const FVector AngularVelocity = Axis * DesiredAngularSpeed;
		Body->SetPhysicsAngularVelocityInRadians(AngularVelocity);
	}
	else // Force mode
	{
		// ── Force Drive ──
		const FVector Displacement = TargetLocation - CurrentLocation;
		const FVector Force = Displacement * ForceMultiplier;
		Body->AddForce(Force);

		// Angular
		const FQuat DeltaRotation = TargetRotation * CurrentRotation.Inverse();
		FVector Axis;
		float Angle;
		DeltaRotation.ToAxisAndAngle(Axis, Angle);
		if (Angle > PI)
		{
			Angle -= 2.0f * PI;
		}
		const FVector Torque = Axis * Angle * ForceMultiplier;
		Body->AddTorqueInRadians(Torque);
	}
}
