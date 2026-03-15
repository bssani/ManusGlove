// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MIDialInteractable.h"
#include "ManusInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UMIDialInteractable::UMIDialInteractable()
{
}

void UMIDialInteractable::BeginPlay()
{
	Super::BeginPlay();
	SetupPhysicsConstraint();
}

void UMIDialInteractable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled)
	{
		return;
	}

	UpdateAngleFromPhysics();

	// Normalize value
	if (!bInfiniteRotation)
	{
		const float Range = MaxAngle - MinAngle;
		CurrentValue = (Range > 0.0f) ? FMath::Clamp((CurrentAngle - MinAngle) / Range, 0.0f, 1.0f) : 0.0f;
	}
	else
	{
		// For infinite rotation, value is based on total rotation
		CurrentValue = CurrentAngle / 360.0f;
	}

	// Detect changes
	if (!FMath::IsNearlyEqual(CurrentAngle, PreviousAngle, 0.1f))
	{
		OnDialRotated.Broadcast(CurrentAngle, CurrentValue);
		NotifyInteractionUpdate(EMIFingerName::Index, CurrentValue);
		PreviousAngle = CurrentAngle;
	}

	// Step snapping
	if (Steps > 0 && !bInfiniteRotation)
	{
		const float Range = MaxAngle - MinAngle;
		const float StepSize = Range / static_cast<float>(Steps);
		CurrentStep = FMath::RoundToInt32((CurrentAngle - MinAngle) / StepSize);
		CurrentStep = FMath::Clamp(CurrentStep, 0, Steps);

		if (CurrentStep != PreviousStep)
		{
			OnDialStepped.Broadcast(CurrentStep);
			PreviousStep = CurrentStep;
		}
	}
}

void UMIDialInteractable::SetupPhysicsConstraint()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UPrimitiveComponent* Mesh = FindPhysicsMesh();
	if (!Mesh)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MIDialInteractable: No mesh found on %s"), *Owner->GetName());
		return;
	}

	DialMesh = Mesh;

	// Enable physics
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(false);
	Mesh->SetMassOverrideInKg(NAME_None, DialMass);
	Mesh->SetAngularDamping(DialDamping);
	Mesh->SetLinearDamping(100.0f); // Prevent translation

	// Create constraint
	DialConstraint = NewObject<UPhysicsConstraintComponent>(Owner, TEXT("DialConstraint"));
	DialConstraint->SetupAttachment(Owner->GetRootComponent());
	DialConstraint->RegisterComponent();
	DialConstraint->SetConstrainedComponents(nullptr, NAME_None, Mesh, NAME_None);

	// Lock all linear axes
	DialConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	DialConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	DialConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);

	// Configure rotation based on axis
	const float HalfRange = bInfiniteRotation ? 0.0f : ((MaxAngle - MinAngle) * 0.5f);
	const EAngularConstraintMotion RotMotion = bInfiniteRotation ? EAngularConstraintMotion::ACM_Free : EAngularConstraintMotion::ACM_Limited;

	switch (RotationAxis)
	{
	case EMIAxis::X:
		DialConstraint->SetAngularSwing1Limit(RotMotion, HalfRange);
		DialConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		DialConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		break;
	case EMIAxis::Y:
		DialConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		DialConstraint->SetAngularSwing2Limit(RotMotion, HalfRange);
		DialConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		break;
	case EMIAxis::Z:
		DialConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		DialConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
		DialConstraint->SetAngularTwistLimit(RotMotion, HalfRange);
		break;
	}

	UE_LOG(LogManusInteraction, Log, TEXT("MIDialInteractable: Setup dial on %s — Axis=%d, Infinite=%d"),
		*Owner->GetName(), static_cast<int32>(RotationAxis), bInfiniteRotation);
}

void UMIDialInteractable::UpdateAngleFromPhysics()
{
	if (!DialMesh.IsValid() || !GetOwner())
	{
		return;
	}

	const FRotator BaseRotation = GetOwner()->GetActorRotation();
	const FRotator CurrentRotation = DialMesh->GetComponentRotation();
	const FRotator Delta = (CurrentRotation - BaseRotation).GetNormalized();

	switch (RotationAxis)
	{
	case EMIAxis::X: CurrentAngle = Delta.Roll; break;
	case EMIAxis::Y: CurrentAngle = Delta.Pitch; break;
	case EMIAxis::Z: CurrentAngle = Delta.Yaw; break;
	}

	if (!bInfiniteRotation)
	{
		CurrentAngle = FMath::Clamp(CurrentAngle, MinAngle, MaxAngle);
	}
}
