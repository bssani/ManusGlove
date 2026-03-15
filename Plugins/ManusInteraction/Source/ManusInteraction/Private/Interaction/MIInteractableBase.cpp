// Copyright 2024-2025 ManusInteraction Plugin

#include "Interaction/MIInteractableBase.h"
#include "ManusInteraction.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"

UMIInteractableBase::UMIInteractableBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMIInteractableBase::BeginPlay()
{
	Super::BeginPlay();
}

void UMIInteractableBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UMIInteractableBase::IsFingerAllowed(EMIFingerName Finger) const
{
	// Empty array = all fingers allowed
	if (AllowedFingers.Num() == 0)
	{
		return true;
	}
	return AllowedFingers.Contains(Finger);
}

void UMIInteractableBase::NotifyInteractionBegin(EMIFingerName Finger, float Value)
{
	bIsBeingInteracted = true;
	ActiveFinger = Finger;
	CurrentValue = Value;
	OnInteractionBegin.Broadcast(Finger, Value);
}

void UMIInteractableBase::NotifyInteractionUpdate(EMIFingerName Finger, float Value)
{
	CurrentValue = Value;
	OnInteractionUpdate.Broadcast(Finger, Value);
}

void UMIInteractableBase::NotifyInteractionEnd(EMIFingerName Finger, float Value)
{
	bIsBeingInteracted = false;
	CurrentValue = Value;
	OnInteractionEnd.Broadcast(Finger, Value);
}

UPrimitiveComponent* UMIInteractableBase::FindPhysicsMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// First try to find a static mesh component
	UStaticMeshComponent* StaticMesh = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (StaticMesh)
	{
		return StaticMesh;
	}

	// Fallback to any primitive component
	return Owner->FindComponentByClass<UPrimitiveComponent>();
}
