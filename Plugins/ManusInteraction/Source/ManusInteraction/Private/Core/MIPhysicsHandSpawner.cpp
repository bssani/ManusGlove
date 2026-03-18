// Copyright 2024-2025 ManusInteraction Plugin

#include "Core/MIPhysicsHandSpawner.h"
#include "Core/MIPhysicsHand.h"
#include "ManusComponent.h"
#include "ManusInteraction.h"

UMIPhysicsHandSpawner::UMIPhysicsHandSpawner()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMIPhysicsHandSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnPhysicsHands();
}

void UMIPhysicsHandSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyPhysicsHands();
	Super::EndPlay(EndPlayReason);
}

void UMIPhysicsHandSpawner::SpawnPhysicsHands()
{
	if (bHasSpawned)
	{
		UE_LOG(LogManusInteraction, Warning,
			TEXT("MIPhysicsHandSpawner: Already spawned. Call DestroyPhysicsHands() first."));
		return;
	}

	if (!LeftManusComponent && !RightManusComponent)
	{
		UE_LOG(LogManusInteraction, Warning,
			TEXT("MIPhysicsHandSpawner: No ManusComponent assigned. "
				 "Set LeftManusComponent and/or RightManusComponent in the Details panel."));
		return;
	}

	if (LeftManusComponent)
	{
		LeftPhysicsHand = SpawnSingleHand(LeftManusComponent, EMIHandSide::Left);
	}

	if (RightManusComponent)
	{
		RightPhysicsHand = SpawnSingleHand(RightManusComponent, EMIHandSide::Right);
	}

	bHasSpawned = true;

	UE_LOG(LogManusInteraction, Log,
		TEXT("MIPhysicsHandSpawner: Spawned physics hands (Left=%s, Right=%s)."),
		LeftPhysicsHand ? TEXT("OK") : TEXT("None"),
		RightPhysicsHand ? TEXT("OK") : TEXT("None"));
}

AMIPhysicsHand* UMIPhysicsHandSpawner::SpawnSingleHand(UManusComponent* ManusComp, EMIHandSide Side)
{
	if (!ManusComp)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMIPhysicsHand* Hand = World->SpawnActor<AMIPhysicsHand>(
		AMIPhysicsHand::StaticClass(), FTransform::Identity, Params);

	if (!Hand)
	{
		UE_LOG(LogManusInteraction, Error,
			TEXT("MIPhysicsHandSpawner: Failed to spawn AMIPhysicsHand for %s hand."),
			Side == EMIHandSide::Left ? TEXT("Left") : TEXT("Right"));
		return nullptr;
	}

	Hand->SourceManusComponent = ManusComp;
	Hand->HandSide = Side;
	Hand->Config = Config;
	Hand->bShowDebugVisualization = bShowDebugVisualization;
	Hand->InitializeHand();

	return Hand;
}

void UMIPhysicsHandSpawner::DestroyPhysicsHands()
{
	if (LeftPhysicsHand)
	{
		LeftPhysicsHand->Destroy();
		LeftPhysicsHand = nullptr;
	}

	if (RightPhysicsHand)
	{
		RightPhysicsHand->Destroy();
		RightPhysicsHand = nullptr;
	}

	bHasSpawned = false;
}
