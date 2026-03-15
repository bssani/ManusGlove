// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MISurfaceInteractable.h"
#include "Core/MIPhysicsHand.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusInteraction.h"
#include "Kismet/GameplayStatics.h"

UMISurfaceInteractable::UMISurfaceInteractable()
{
	// Initialize touch states for all fingers
	for (int32 i = 0; i < static_cast<int32>(EMIFingerName::Max); ++i)
	{
		FingerTouchStates.Add(static_cast<EMIFingerName>(i), FFingerTouchState());
	}
}

void UMISurfaceInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void UMISurfaceInteractable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled || !GetOwner())
	{
		return;
	}

	// Find all physics hands in the world and check finger proximity
	TArray<AActor*> PhysicsHands;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMIPhysicsHand::StaticClass(), PhysicsHands);

	for (AActor* HandActor : PhysicsHands)
	{
		AMIPhysicsHand* Hand = Cast<AMIPhysicsHand>(HandActor);
		if (!Hand || !Hand->IsHandInitialized())
		{
			continue;
		}

		// Check each finger
		for (int32 i = 0; i < static_cast<int32>(EMIFingerName::Max); ++i)
		{
			const EMIFingerName FingerName = static_cast<EMIFingerName>(i);
			if (!IsFingerAllowed(FingerName))
			{
				continue;
			}

			UMIPhysicsFingerComponent* Finger = Hand->GetFingerComponent(FingerName);
			if (!Finger)
			{
				continue;
			}

			// Convert finger world position to surface UV
			const FVector FingerWorldPos = Finger->GetComponentLocation();
			const FVector2D UV = WorldToSurfaceUV(FingerWorldPos);

			// Check if finger is within surface bounds and close enough
			const FTransform OwnerTransform = GetOwner()->GetActorTransform();
			const FVector LocalPos = OwnerTransform.InverseTransformPosition(FingerWorldPos) - SurfaceOffset;

			// Distance along surface normal
			float NormalDistance = 0.0f;
			switch (SurfaceNormalAxis)
			{
			case EMIAxis::X: NormalDistance = FMath::Abs(LocalPos.X); break;
			case EMIAxis::Y: NormalDistance = FMath::Abs(LocalPos.Y); break;
			case EMIAxis::Z: NormalDistance = FMath::Abs(LocalPos.Z); break;
			}

			const bool bWithinBounds = UV.X >= 0.0f && UV.X <= 1.0f && UV.Y >= 0.0f && UV.Y <= 1.0f;
			const bool bCloseEnough = NormalDistance <= TouchDetectionDistance;
			const bool bIsTouching = bWithinBounds && bCloseEnough;

			FFingerTouchState& State = FingerTouchStates.FindOrAdd(FingerName);

			// Calculate pressure based on how close the finger is
			const float Pressure = bIsTouching
				? FMath::Clamp(1.0f - (NormalDistance / TouchDetectionDistance), 0.0f, 1.0f)
				: 0.0f;

			if (bIsTouching && !State.bIsTouching)
			{
				// Touch began
				State.bIsTouching = true;
				State.LastUV = UV;
				OnSurfaceTouched.Broadcast(FingerName, UV, Pressure);
				NotifyInteractionBegin(FingerName, Pressure);
			}
			else if (bIsTouching && State.bIsTouching)
			{
				// Touch moved
				if (!UV.Equals(State.LastUV, 0.001f))
				{
					State.LastUV = UV;
					OnSurfaceMoved.Broadcast(FingerName, UV, Pressure);
					NotifyInteractionUpdate(FingerName, Pressure);
				}
			}
			else if (!bIsTouching && State.bIsTouching)
			{
				// Touch ended
				State.bIsTouching = false;
				OnSurfaceReleased.Broadcast(FingerName, State.LastUV, 0.0f);
				NotifyInteractionEnd(FingerName, 0.0f);
				State.LastUV = FVector2D(-1.0f, -1.0f);
			}
		}
	}
}

FVector2D UMISurfaceInteractable::GetTouchLocationUV(EMIFingerName Finger) const
{
	const FFingerTouchState* State = FingerTouchStates.Find(Finger);
	if (State && State->bIsTouching)
	{
		return State->LastUV;
	}
	return FVector2D(-1.0f, -1.0f);
}

FVector2D UMISurfaceInteractable::WorldToSurfaceUV(FVector WorldLocation) const
{
	if (!GetOwner())
	{
		return FVector2D(-1.0f, -1.0f);
	}

	const FTransform OwnerTransform = GetOwner()->GetActorTransform();
	const FVector LocalPos = OwnerTransform.InverseTransformPosition(WorldLocation) - SurfaceOffset;

	// Map local position to UV based on surface normal axis
	FVector2D UV;
	switch (SurfaceNormalAxis)
	{
	case EMIAxis::X:
		// Surface faces X; Y is U, Z is V
		UV.X = (LocalPos.Y / SurfaceExtent.X) + 0.5f;
		UV.Y = (LocalPos.Z / SurfaceExtent.Y) + 0.5f;
		break;
	case EMIAxis::Y:
		// Surface faces Y; X is U, Z is V
		UV.X = (LocalPos.X / SurfaceExtent.X) + 0.5f;
		UV.Y = (LocalPos.Z / SurfaceExtent.Y) + 0.5f;
		break;
	case EMIAxis::Z:
		// Surface faces Z; X is U, Y is V
		UV.X = (LocalPos.X / SurfaceExtent.X) + 0.5f;
		UV.Y = (LocalPos.Y / SurfaceExtent.Y) + 0.5f;
		break;
	}

	return UV;
}

int32 UMISurfaceInteractable::GetActiveTouchCount() const
{
	int32 Count = 0;
	for (const auto& Pair : FingerTouchStates)
	{
		if (Pair.Value.bIsTouching)
		{
			Count++;
		}
	}
	return Count;
}
