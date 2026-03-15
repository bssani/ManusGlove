// Copyright 2024-2025 ManusInteraction Plugin

#include "Grab/MIGrabbableComponent.h"
#include "ManusInteraction.h"
#include "Components/PrimitiveComponent.h"

UMIGrabbableComponent::UMIGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMIGrabbableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure the owning actor's root component has physics enabled
	if (AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (RootPrimitive)
		{
			if (!RootPrimitive->IsSimulatingPhysics())
			{
				RootPrimitive->SetSimulatePhysics(true);
				UE_LOG(LogManusInteraction, Log, TEXT("MIGrabbableComponent: Enabled physics on %s"), *Owner->GetName());
			}
			bOriginalGravity = RootPrimitive->IsGravityEnabled();
		}
	}
}

void UMIGrabbableComponent::NotifyGrabbed(AActor* GrabbingActor)
{
	bIsHeld = true;

	// Adjust gravity while held
	if (AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (RootPrimitive)
		{
			RootPrimitive->SetEnableGravity(bUseGravityWhileHeld);
		}
	}

	OnGrabbed.Broadcast(GrabbingActor);
}

void UMIGrabbableComponent::NotifyReleased(AActor* GrabbingActor)
{
	bIsHeld = false;

	// Restore gravity on release
	if (AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (RootPrimitive)
		{
			RootPrimitive->SetEnableGravity(bUseGravityWhenReleased ? bOriginalGravity : false);
		}
	}

	OnReleased.Broadcast(GrabbingActor);
}
