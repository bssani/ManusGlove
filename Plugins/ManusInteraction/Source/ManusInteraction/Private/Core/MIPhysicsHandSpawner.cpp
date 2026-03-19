// Copyright 2024-2025 ManusInteraction Plugin

#include "Core/MIPhysicsHandSpawner.h"
#include "Core/MIPhysicsHand.h"
#include "ManusComponent.h"
#include "ManusSkeleton.h"
#include "ManusBlueprintTypes.h"
#include "ManusInteraction.h"

UMIPhysicsHandSpawner::UMIPhysicsHandSpawner()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMIPhysicsHandSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Auto-discover if not manually assigned.
	if (!LeftManusComponent && !RightManusComponent)
	{
		AutoDiscoverManusComponents();
	}

	SpawnPhysicsHands();
}

void UMIPhysicsHandSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyPhysicsHands();
	Super::EndPlay(EndPlayReason);
}

void UMIPhysicsHandSpawner::AutoDiscoverManusComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UManusComponent*> ManusComps;
	Owner->GetComponents<UManusComponent>(ManusComps);

	if (ManusComps.Num() == 0)
	{
		UE_LOG(LogManusInteraction, Warning,
			TEXT("MIPhysicsHandSpawner: No UManusComponent found on '%s'. "
				 "Add ManusComponents to the Actor or assign them manually."),
			*Owner->GetName());
		return;
	}

	for (UManusComponent* Comp : ManusComps)
	{
		if (!Comp || !Comp->ManusSkeleton)
		{
			continue;
		}

		// Determine hand side from the skeleton's chain setup.
		bool bIsRight = false;
		for (const TPair<FName, FManusChainSetup>& ChainPair : Comp->ManusSkeleton->ChainsIndexMap)
		{
			if (ChainPair.Value.Side == EManusSide::Right)
			{
				bIsRight = true;
			}
			break;
		}

		if (bIsRight)
		{
			if (!RightManusComponent)
			{
				RightManusComponent = Comp;
				UE_LOG(LogManusInteraction, Log,
					TEXT("MIPhysicsHandSpawner: Auto-assigned Right ManusComponent '%s'."),
					*Comp->GetName());
			}
		}
		else
		{
			if (!LeftManusComponent)
			{
				LeftManusComponent = Comp;
				UE_LOG(LogManusInteraction, Log,
					TEXT("MIPhysicsHandSpawner: Auto-assigned Left ManusComponent '%s'."),
					*Comp->GetName());
			}
		}

		// Stop if both are found.
		if (LeftManusComponent && RightManusComponent)
		{
			break;
		}
	}
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
			TEXT("MIPhysicsHandSpawner: No ManusComponent assigned or discovered. "
				 "Ensure ManusComponents with valid ManusSkeleton are present on the Actor."));
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
