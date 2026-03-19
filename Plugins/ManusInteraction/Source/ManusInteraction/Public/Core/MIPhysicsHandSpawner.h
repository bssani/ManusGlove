// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIPhysicsHandSpawner.generated.h"

class AMIPhysicsHand;
class UManusComponent;

/// @brief Helper component that automatically spawns AMIPhysicsHand actors.
/// Add this to your Manus Pawn Blueprint and assign the left/right
/// UManusComponent references. On BeginPlay the component spawns two
/// AMIPhysicsHand actors, configures them, and calls InitializeHand().
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

	/// Left hand Manus skeletal mesh component (from the existing Pawn).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner", meta = (UseComponentPicker))
	TObjectPtr<UManusComponent> LeftManusComponent;

	/// Right hand Manus skeletal mesh component (from the existing Pawn).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|HandSpawner", meta = (UseComponentPicker))
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
	AMIPhysicsHand* SpawnSingleHand(UManusComponent* ManusComp, EMIHandSide Side);

	bool bHasSpawned = false;
};
