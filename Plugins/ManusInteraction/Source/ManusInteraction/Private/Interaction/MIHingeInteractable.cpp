// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MIHingeInteractable.h"
#include "ManusInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UMIHingeInteractable::UMIHingeInteractable()
{
}

void UMIHingeInteractable::BeginPlay()
{
	Super::BeginPlay();
	SetupPhysicsConstraint();
}

void UMIHingeInteractable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled)
	{
		return;
	}

	UpdateAngleFromPhysics();

	// Auto-return logic
	if (bAutoReturn && !bIsBeingInteracted && ConstrainedMesh.IsValid())
	{
		if (!FMath::IsNearlyEqual(CurrentAngle, RestAngle, 0.5f))
		{
			const float Direction = FMath::Sign(RestAngle - CurrentAngle);
			const float DeltaAngle = Direction * ReturnSpeed * DeltaTime;
			float NewAngle = CurrentAngle + DeltaAngle;

			// Don't overshoot
			if ((Direction > 0 && NewAngle > RestAngle) || (Direction < 0 && NewAngle < RestAngle))
			{
				NewAngle = RestAngle;
			}

			SetAngle(NewAngle);
		}
	}

	// Update normalized value
	CurrentValue = AngleToNormalized(CurrentAngle);

	// Snapping
	if (SnapPositions.Num() > 0)
	{
		ApplySnapping();
	}
}

void UMIHingeInteractable::SetAngle(float Angle)
{
	CurrentAngle = FMath::Clamp(Angle, MinAngle, MaxAngle);

	if (ConstrainedMesh.IsValid())
	{
		FRotator DeltaRotation = FRotator::ZeroRotator;
		switch (HingeAxis)
		{
		case EMIAxis::X: DeltaRotation.Roll = CurrentAngle; break;
		case EMIAxis::Y: DeltaRotation.Pitch = CurrentAngle; break;
		case EMIAxis::Z: DeltaRotation.Yaw = CurrentAngle; break;
		}

		// Apply rotation relative to initial transform
		const FRotator BaseRotation = GetOwner()->GetActorRotation();
		ConstrainedMesh->SetWorldRotation(BaseRotation + DeltaRotation);
	}

	CurrentValue = AngleToNormalized(CurrentAngle);
}

void UMIHingeInteractable::SetupPhysicsConstraint()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find the mesh to constrain
	UPrimitiveComponent* Mesh = FindPhysicsMesh();
	if (!Mesh)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MIHingeInteractable: No mesh found on owning actor %s"), *Owner->GetName());
		return;
	}

	ConstrainedMesh = Mesh;

	// Enable physics on the mesh
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(false);
	Mesh->SetMassOverrideInKg(NAME_None, HingeMass);
	Mesh->SetAngularDamping(HingeDamping);
	Mesh->SetLinearDamping(10.0f); // High linear damping to prevent translation

	// Create the physics constraint
	HingeConstraint = NewObject<UPhysicsConstraintComponent>(Owner, TEXT("HingeConstraint"));
	HingeConstraint->SetupAttachment(Owner->GetRootComponent());
	HingeConstraint->RegisterComponent();

	// Configure constraint as a hinge
	HingeConstraint->SetConstrainedComponents(nullptr, NAME_None, Mesh, NAME_None);

	// Lock all linear axes
	HingeConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	HingeConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	HingeConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);

	// Configure angular limits based on axis
	const float HalfRange = (MaxAngle - MinAngle) * 0.5f;

	switch (HingeAxis)
	{
	case EMIAxis::X:
		HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, HalfRange);
		HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		break;
	case EMIAxis::Y:
		HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, HalfRange);
		HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		break;
	case EMIAxis::Z:
		HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, HalfRange);
		break;
	}

	UE_LOG(LogManusInteraction, Log, TEXT("MIHingeInteractable: Setup hinge on %s — Axis=%d, Range=[%.1f, %.1f]"),
		*Owner->GetName(), static_cast<int32>(HingeAxis), MinAngle, MaxAngle);
}

void UMIHingeInteractable::UpdateAngleFromPhysics()
{
	if (!ConstrainedMesh.IsValid() || !GetOwner())
	{
		return;
	}

	// Calculate current angle relative to owner's base rotation
	const FRotator BaseRotation = GetOwner()->GetActorRotation();
	const FRotator CurrentRotation = ConstrainedMesh->GetComponentRotation();
	const FRotator Delta = (CurrentRotation - BaseRotation).GetNormalized();

	switch (HingeAxis)
	{
	case EMIAxis::X: CurrentAngle = Delta.Roll; break;
	case EMIAxis::Y: CurrentAngle = Delta.Pitch; break;
	case EMIAxis::Z: CurrentAngle = Delta.Yaw; break;
	}

	CurrentAngle = FMath::Clamp(CurrentAngle, MinAngle, MaxAngle);
}

void UMIHingeInteractable::ApplySnapping()
{
	if (SnapPositions.Num() == 0)
	{
		return;
	}

	// Find nearest snap position
	float NearestDist = MAX_FLT;
	int32 NearestIndex = -1;

	for (int32 i = 0; i < SnapPositions.Num(); ++i)
	{
		const float SnapAngle = NormalizedToAngle(SnapPositions[i]);
		const float Dist = FMath::Abs(CurrentAngle - SnapAngle);
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			NearestIndex = i;
		}
	}

	// Check if we should snap (within snap force threshold)
	const float SnapThresholdAngle = SnapForce; // Simplified: use snap force as angle threshold
	if (NearestIndex >= 0 && NearestDist < SnapThresholdAngle && NearestIndex != CurrentSnapIndex)
	{
		CurrentSnapIndex = NearestIndex;
		OnHingeSnapped.Broadcast(CurrentSnapIndex, SnapPositions[CurrentSnapIndex]);
	}
}

float UMIHingeInteractable::AngleToNormalized(float Angle) const
{
	const float Range = MaxAngle - MinAngle;
	if (FMath::IsNearlyZero(Range))
	{
		return 0.5f;
	}
	return FMath::Clamp((Angle - MinAngle) / Range, 0.0f, 1.0f);
}

float UMIHingeInteractable::NormalizedToAngle(float Normalized) const
{
	return MinAngle + Normalized * (MaxAngle - MinAngle);
}
