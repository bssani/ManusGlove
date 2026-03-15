// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MIButtonInteractable.h"
#include "ManusInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UMIButtonInteractable::UMIButtonInteractable()
{
}

void UMIButtonInteractable::BeginPlay()
{
	Super::BeginPlay();
	SetupPhysicsConstraint();
}

void UMIButtonInteractable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled)
	{
		return;
	}

	UpdateDepthFromPhysics();

	// Check press state
	const bool bNowPressed = CurrentDepth >= ActivationThreshold;

	if (bNowPressed && !bWasPressedLastFrame)
	{
		bIsPressed = true;

		if (bIsToggle)
		{
			bToggleState = !bToggleState;
			OnButtonToggled.Broadcast(bToggleState);
		}

		OnButtonPressed.Broadcast();
		NotifyInteractionBegin(EMIFingerName::Index, CurrentDepth);
	}
	else if (!bNowPressed && bWasPressedLastFrame)
	{
		bIsPressed = false;
		OnButtonReleased.Broadcast();
		NotifyInteractionEnd(EMIFingerName::Index, CurrentDepth);
	}
	else if (bNowPressed)
	{
		NotifyInteractionUpdate(EMIFingerName::Index, CurrentDepth);
	}

	bWasPressedLastFrame = bNowPressed;
	CurrentValue = CurrentDepth;

	// Spring return force when not being pressed by a finger
	if (ButtonMesh.IsValid() && ButtonMesh->IsSimulatingPhysics() && CurrentDepth > 0.01f)
	{
		FVector ReturnDir;
		switch (PressAxis)
		{
		case EMIAxis::X: ReturnDir = GetOwner()->GetActorForwardVector(); break;
		case EMIAxis::Y: ReturnDir = GetOwner()->GetActorRightVector(); break;
		case EMIAxis::Z: ReturnDir = GetOwner()->GetActorUpVector(); break;
		}
		// Push back toward initial position
		ButtonMesh->AddForce(ReturnDir * ReturnForce * CurrentDepth);
	}
}

void UMIButtonInteractable::SetupPhysicsConstraint()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UPrimitiveComponent* Mesh = FindPhysicsMesh();
	if (!Mesh)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MIButtonInteractable: No mesh found on %s"), *Owner->GetName());
		return;
	}

	ButtonMesh = Mesh;
	InitialLocalPosition = Mesh->GetRelativeLocation();

	// Enable physics
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(false);
	Mesh->SetMassOverrideInKg(NAME_None, ButtonMass);
	Mesh->SetLinearDamping(5.0f);
	Mesh->SetAngularDamping(100.0f); // Prevent rotation

	// Create constraint for linear-only movement
	ButtonConstraint = NewObject<UPhysicsConstraintComponent>(Owner, TEXT("ButtonConstraint"));
	ButtonConstraint->SetupAttachment(Owner->GetRootComponent());
	ButtonConstraint->RegisterComponent();
	ButtonConstraint->SetConstrainedComponents(nullptr, NAME_None, Mesh, NAME_None);

	// Lock all angular axes
	ButtonConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	ButtonConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	ButtonConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);

	// Allow movement only along press axis
	switch (PressAxis)
	{
	case EMIAxis::X:
		ButtonConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, PressDepth);
		ButtonConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		ButtonConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		break;
	case EMIAxis::Y:
		ButtonConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		ButtonConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, PressDepth);
		ButtonConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		break;
	case EMIAxis::Z:
		ButtonConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		ButtonConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
		ButtonConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, PressDepth);
		break;
	}

	UE_LOG(LogManusInteraction, Log, TEXT("MIButtonInteractable: Setup button on %s — Axis=%d, Depth=%.1f"),
		*Owner->GetName(), static_cast<int32>(PressAxis), PressDepth);
}

void UMIButtonInteractable::UpdateDepthFromPhysics()
{
	if (!ButtonMesh.IsValid() || !GetOwner())
	{
		return;
	}

	const FVector CurrentLocal = ButtonMesh->GetRelativeLocation();
	const FVector Delta = CurrentLocal - InitialLocalPosition;

	float LinearDepth = 0.0f;
	switch (PressAxis)
	{
	case EMIAxis::X: LinearDepth = -Delta.X; break; // Negative because pressing goes inward
	case EMIAxis::Y: LinearDepth = -Delta.Y; break;
	case EMIAxis::Z: LinearDepth = -Delta.Z; break;
	}

	CurrentDepth = FMath::Clamp(LinearDepth / PressDepth, 0.0f, 1.0f);
}
