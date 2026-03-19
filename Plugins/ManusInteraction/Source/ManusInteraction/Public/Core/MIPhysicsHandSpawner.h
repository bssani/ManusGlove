// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIPhysicsHandSpawner.generated.h"

class AMIPhysicsHand;
class UManusComponent;

/// @brief Helper component that automatically spawns AMIPhysicsHand actors.
/// Add this to your Manus Pawn Blueprint. On BeginPlay the component
/// auto-discovers UManusComponent instances on the owning Actor, determines
/// Left/Right from each component's ManusSkeleton chain setup, spawns two
/// AMIPhysicsHand actors, and calls InitializeHand().
///
/// Manual assignment via LeftManusComponent / RightManusComponent is also
/// supported; auto-discovery only runs when both are null.
///
/// The spawned hands are accessible via LeftPhysicsHand / RightPhysicsHand
/// and are automatically destroyed when this component is destroyed.
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIPhysicsHandSpawner : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIPhysicsHandSpawner();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ── Configuration ──

	/// Left hand Manus skeletal mesh component.
	/// If left null, auto-discovered from the owning Actor at BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner")
	TObjectPtr<UManusComponent> LeftManusComponent;

	/// Right hand Manus skeletal mesh component.
	/// If left null, auto-discovered from the owning Actor at BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner")
	TObjectPtr<UManusComponent> RightManusComponent;

	/// Physics hand configuration applied to both hands.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner")
	FMIPhysicsHandConfig Config;

	/// Show debug visualization on the spawned physics hands.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner|Debug")
	bool bShowDebugVisualization = false;

	// ── Runtime References (read-only) ──

	/// The spawned left physics hand actor. Valid after BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|HandSpawner")
	TObjectPtr<AMIPhysicsHand> LeftPhysicsHand;

	/// The spawned right physics hand actor. Valid after BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|HandSpawner")
	TObjectPtr<AMIPhysicsHand> RightPhysicsHand;

	// ── Public API ──

	/// Spawn both physics hands. Called automatically in BeginPlay.
	/// Can be called manually if LeftManusComponent / RightManusComponent
	/// are set after BeginPlay (e.g. late initialization).
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|HandSpawner")
	void SpawnPhysicsHands();

	/// Destroy the spawned physics hand actors. Called automatically in EndPlay.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|HandSpawner")
	void DestroyPhysicsHands();

private:
	/// Auto-discover UManusComponent instances on the owning Actor and
	/// assign LeftManusComponent / RightManusComponent based on
	/// ManusSkeleton chain side data.
	void AutoDiscoverManusComponents();

	AMIPhysicsHand* SpawnSingleHand(UManusComponent* ManusComp, EMIHandSide Side);

	bool bHasSpawned = false;
};
