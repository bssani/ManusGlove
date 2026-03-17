// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MISteeringWheelComponent.h"
#include "Core/MIPhysicsHand.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusInteraction.h"

UMISteeringWheelComponent::UMISteeringWheelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Run after physics so UMIPhysicsFingerComponent ContactActor is already settled
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UMISteeringWheelComponent::BeginPlay()
{
	Super::BeginPlay();

	if (WheelMesh)
	{
		BaseRotation = WheelMesh->GetRelativeRotation();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// TickComponent — main update loop
// ─────────────────────────────────────────────────────────────────────────────

void UMISteeringWheelComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!WheelMesh)
	{
		return;
	}

	// 1. Detect current grab state for each hand
	const bool bLeftNow  = CheckHandGrabbing(LeftHand);
	const bool bRightNow = CheckHandGrabbing(RightHand);

	// 2. Handle state transitions (grab start / release)
	if (bLeftNow != bWasLeftGrabbing)
	{
		HandleGrabStateChange(EMIHandSide::Left, bLeftNow, LeftHand);
	}
	if (bRightNow != bWasRightGrabbing)
	{
		HandleGrabStateChange(EMIHandSide::Right, bRightNow, RightHand);
	}

	bLeftHandGrabbing  = bLeftNow;
	bRightHandGrabbing = bRightNow;
	bWasLeftGrabbing   = bLeftNow;
	bWasRightGrabbing  = bRightNow;

	// 3. Compute rotation delta from grabbing hands
	float TotalDelta = 0.0f;
	int32 ActiveHands = 0;

	auto AccumulateDelta = [&](AMIPhysicsHand* Hand, bool bGrabbing, float& LastAngle)
	{
		if (!bGrabbing || !Hand)
		{
			return;
		}

		// Ignore hands too close to center (angle becomes unstable)
		if (GetHandRadiusOnWheel(Hand) < MinGrabRadius)
		{
			// Update reference to avoid a jump when hand moves away from center
			LastAngle = GetHandAngleOnWheel(Hand);
			return;
		}

		const float CurrentHandAngle = GetHandAngleOnWheel(Hand);
		const float Delta = FMath::UnwindDegrees(CurrentHandAngle - LastAngle);
		TotalDelta += Delta;
		LastAngle = CurrentHandAngle;
		++ActiveHands;
	};

	AccumulateDelta(LeftHand,  bLeftNow,  LastHandAngle_Left);
	AccumulateDelta(RightHand, bRightNow, LastHandAngle_Right);

	if (ActiveHands > 1)
	{
		TotalDelta /= static_cast<float>(ActiveHands);
	}

	const float PreviousAngle = CurrentAngle;

	// 4. Apply delta
	if (ActiveHands > 0)
	{
		CurrentAngle += TotalDelta;
	}
	else if (bAutoReturn && !FMath::IsNearlyZero(CurrentAngle, 0.1f))
	{
		// Auto-return toward 0°
		const float Direction = FMath::Sign(-CurrentAngle);
		const float Step = Direction * ReturnSpeed * DeltaTime;
		// Don't overshoot zero
		if (FMath::Abs(Step) >= FMath::Abs(CurrentAngle))
		{
			CurrentAngle = 0.0f;
		}
		else
		{
			CurrentAngle += Step;
		}
	}

	// 5. Clamp to ±MaxLockAngle and fire lock event on transition
	const float ClampedAngle = FMath::Clamp(CurrentAngle, -MaxLockAngle, MaxLockAngle);
	const bool bAtLimit = !FMath::IsNearlyEqual(ClampedAngle, CurrentAngle);
	if (bAtLimit && !bLockedLastFrame)
	{
		OnWheelLocked.Broadcast(CurrentAngle > 0.0f);
	}
	bLockedLastFrame = bAtLimit;
	CurrentAngle = ClampedAngle;

	// 6. Apply to mesh
	ApplyAngleToMesh();

	// 7. Broadcast rotation event if anything changed
	if (!FMath::IsNearlyEqual(CurrentAngle, PreviousAngle))
	{
		OnWheelRotated.Broadcast(CurrentAngle, GetNormalizedValue());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

float UMISteeringWheelComponent::GetNormalizedValue() const
{
	if (FMath::IsNearlyZero(MaxLockAngle))
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentAngle / MaxLockAngle, -1.0f, 1.0f);
}

void UMISteeringWheelComponent::SetAngle(float NewAngle)
{
	CurrentAngle = FMath::Clamp(NewAngle, -MaxLockAngle, MaxLockAngle);
	ApplyAngleToMesh();
	OnWheelRotated.Broadcast(CurrentAngle, GetNormalizedValue());
}

// ─────────────────────────────────────────────────────────────────────────────
// Private Helpers
// ─────────────────────────────────────────────────────────────────────────────

bool UMISteeringWheelComponent::CheckHandGrabbing(AMIPhysicsHand* Hand) const
{
	if (!Hand)
	{
		return false;
	}

	AActor* WheelActor = GetOwner();
	if (!WheelActor)
	{
		return false;
	}

	int32 ContactCount = 0;
	bool bThumbContact = false;

	const TArray<FMIFingerState> FingerStates = Hand->GetAllFingerStates();
	for (const FMIFingerState& State : FingerStates)
	{
		UMIPhysicsFingerComponent* Finger = Hand->GetFingerComponent(State.FingerName);
		if (!Finger)
		{
			continue;
		}

		if (Finger->bIsInContact && Finger->ContactActor.Get() == WheelActor)
		{
			++ContactCount;
			if (State.FingerName == EMIFingerName::Thumb)
			{
				bThumbContact = true;
			}
		}
	}

	if (ContactCount < MinGrabFingers)
	{
		return false;
	}
	if (bRequireThumb && !bThumbContact)
	{
		return false;
	}
	return true;
}

float UMISteeringWheelComponent::GetHandAngleOnWheel(AMIPhysicsHand* Hand) const
{
	if (!Hand || !WheelMesh || !GetOwner())
	{
		return 0.0f;
	}

	// Use the palm body as the hand's reference point
	UPrimitiveComponent* Palm = Hand->GetPalmComponent();
	const FVector WorldPos = Palm ? Palm->GetComponentLocation()
	                               : Hand->GetActorLocation();

	// IMPORTANT: Use the wheel ACTOR's transform (not the mesh's transform).
	// The mesh rotates every tick, so its local space rotates with it.
	// Working in actor-local space gives a fixed reference frame regardless
	// of how much the wheel mesh has already rotated.
	const FVector LocalPos = GetOwner()->GetActorTransform().InverseTransformPosition(WorldPos);

	// Project onto the plane perpendicular to the rotation axis and get angle
	float Angle = 0.0f;
	switch (WheelRotationAxis)
	{
	case EMIAxis::X:
		// Rotation around X → use YZ plane
		Angle = FMath::RadiansToDegrees(FMath::Atan2(LocalPos.Z, LocalPos.Y));
		break;
	case EMIAxis::Y:
		// Rotation around Y → use XZ plane
		Angle = FMath::RadiansToDegrees(FMath::Atan2(LocalPos.Z, LocalPos.X));
		break;
	case EMIAxis::Z:
		// Rotation around Z → use XY plane
		Angle = FMath::RadiansToDegrees(FMath::Atan2(LocalPos.Y, LocalPos.X));
		break;
	}
	return Angle;
}

float UMISteeringWheelComponent::GetHandRadiusOnWheel(AMIPhysicsHand* Hand) const
{
	if (!Hand || !WheelMesh || !GetOwner())
	{
		return 0.0f;
	}

	UPrimitiveComponent* Palm = Hand->GetPalmComponent();
	const FVector WorldPos = Palm ? Palm->GetComponentLocation()
	                               : Hand->GetActorLocation();

	// Same as GetHandAngleOnWheel: use actor-local space, not mesh-local space.
	const FVector LocalPos = GetOwner()->GetActorTransform().InverseTransformPosition(WorldPos);

	switch (WheelRotationAxis)
	{
	case EMIAxis::X:
		return FVector2D(LocalPos.Y, LocalPos.Z).Size();
	case EMIAxis::Y:
		return FVector2D(LocalPos.X, LocalPos.Z).Size();
	case EMIAxis::Z:
	default:
		return FVector2D(LocalPos.X, LocalPos.Y).Size();
	}
}

void UMISteeringWheelComponent::ApplyAngleToMesh() const
{
	if (!WheelMesh)
	{
		return;
	}

	FRotator NewRotation = BaseRotation;
	switch (WheelRotationAxis)
	{
	case EMIAxis::X:
		NewRotation.Roll  += CurrentAngle;
		break;
	case EMIAxis::Y:
		NewRotation.Pitch += CurrentAngle;
		break;
	case EMIAxis::Z:
		NewRotation.Yaw   += CurrentAngle;
		break;
	}

	WheelMesh->SetRelativeRotation(NewRotation);
}

void UMISteeringWheelComponent::HandleGrabStateChange(EMIHandSide Side, bool bNowGrabbing,
	AMIPhysicsHand* Hand)
{
	if (bNowGrabbing)
	{
		// Initialise reference angle to current hand position so first delta = 0.
		// This prevents a sudden wheel jump on grab.
		if (Hand)
		{
			if (Side == EMIHandSide::Left)
			{
				LastHandAngle_Left = GetHandAngleOnWheel(Hand);
			}
			else
			{
				LastHandAngle_Right = GetHandAngleOnWheel(Hand);
			}
		}
		OnHandGrabbed.Broadcast(Side);
	}
	else
	{
		OnHandReleased.Broadcast(Side);
	}
}
