// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MISliderInteractable.h"
#include "ManusInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UMISliderInteractable::UMISliderInteractable()
{
}

void UMISliderInteractable::BeginPlay()
{
	Super::BeginPlay();
	SetupPhysicsConstraint();
}

void UMISliderInteractable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled)
	{
		return;
	}

	UpdatePositionFromPhysics();

	CurrentValue = CurrentPosition;

	// Detect changes
	if (!FMath::IsNearlyEqual(CurrentPosition, PreviousPosition, 0.001f))
	{
		OnSliderMoved.Broadcast(CurrentPosition);
		NotifyInteractionUpdate(EMIFingerName::Index, CurrentPosition);
		PreviousPosition = CurrentPosition;
	}

	// Step snapping
	if (Steps > 0)
	{
		CurrentStep = FMath::RoundToInt32(CurrentPosition * static_cast<float>(Steps));
		CurrentStep = FMath::Clamp(CurrentStep, 0, Steps);

		if (CurrentStep != PreviousStep)
		{
			OnSliderStepped.Broadcast(CurrentStep);
			PreviousStep = CurrentStep;
		}
	}
}

void UMISliderInteractable::SetupPhysicsConstraint()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UPrimitiveComponent* Mesh = FindPhysicsMesh();
	if (!Mesh)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MISliderInteractable: No mesh found on %s"), *Owner->GetName());
		return;
	}

	SliderMesh = Mesh;
	InitialLocalPosition = Mesh->GetRelativeLocation();

	// Enable physics
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(false);
	Mesh->SetMassOverrideInKg(NAME_None, SliderMass);
	Mesh->SetLinearDamping(SliderDamping);
	Mesh->SetAngularDamping(100.0f); // Prevent rotation

	// Create constraint
	SliderConstraint = NewObject<UPhysicsConstraintComponent>(Owner, TEXT("SliderConstraint"));
	SliderConstraint->SetupAttachment(Owner->GetRootComponent());
	SliderConstraint->RegisterComponent();
	SliderConstraint->SetConstrainedComponents(nullptr, NAME_None, Mesh, NAME_None);

	// Lock all angular axes
	SliderConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	SliderConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	SliderConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);

	// Allow movement only along slide axis
	const float HalfDistance = SlideDistance * 0.5f;

	switch (SlideAxis)
	{
	case EMIAxis::X:
		SliderConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, HalfDistance);
		SliderConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		SliderConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		break;
	case EMIAxis::Y:
		SliderConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		SliderConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, HalfDistance);
		SliderConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		break;
	case EMIAxis::Z:
		SliderConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		SliderConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		SliderConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, HalfDistance);
		break;
	}

	UE_LOG(LogManusInteraction, Log, TEXT("MISliderInteractable: Setup slider on %s — Axis=%d, Distance=%.1f"),
		*Owner->GetName(), static_cast<int32>(SlideAxis), SlideDistance);
}

void UMISliderInteractable::UpdatePositionFromPhysics()
{
	if (!SliderMesh.IsValid())
	{
		return;
	}

	const FVector CurrentLocal = SliderMesh->GetRelativeLocation();
	const FVector Delta = CurrentLocal - InitialLocalPosition;

	float LinearPos = 0.0f;
	switch (SlideAxis)
	{
	case EMIAxis::X: LinearPos = Delta.X; break;
	case EMIAxis::Y: LinearPos = Delta.Y; break;
	case EMIAxis::Z: LinearPos = Delta.Z; break;
	}

	// Normalize to 0-1 (0 = initial, 1 = max distance)
	CurrentPosition = FMath::Clamp((LinearPos + SlideDistance * 0.5f) / SlideDistance, 0.0f, 1.0f);
}
