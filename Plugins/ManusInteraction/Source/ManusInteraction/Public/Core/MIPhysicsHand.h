// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Utils/MITypes.h"
#include "MIPhysicsHand.generated.h"
class UManusComponent;
class UMIPhysicsFingerComponent;
class UMIHandDriver;
class UMIHapticFeedbackManager;

/// @brief Physics hand actor that creates collision-aware finger bodies.
/// Reads bone transforms from a UManusComponent (kinematic) and drives
/// physics sphere bodies toward those targets. The physics bodies stop at
/// surfaces, push interactables, and report contact events.
///
/// Usage: Spawn one per hand. Assign SourceManusComponent and call InitializeHand().
UCLASS(Blueprintable, BlueprintType)
class MANUSINTERACTION_API AMIPhysicsHand : public AActor
{
	GENERATED_BODY()

public:
	AMIPhysicsHand();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ── Configuration ──

	/// The Manus skeletal mesh component to read bone transforms from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand")
	UManusComponent* SourceManusComponent = nullptr;

	/// Which hand this represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand")
	EMIHandSide HandSide = EMIHandSide::Left;

	/// Physics hand configuration.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand")
	FMIPhysicsHandConfig Config;

	/// Bone names for each fingertip. Must match the Manus skeleton.
	/// Default values are for the standard Manus hand skeleton.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand")
	TMap<EMIFingerName, FName> FingerTipBoneNames;

	/// Bone name for the palm body.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand")
	FName PalmBoneName;

	/// Whether to show debug visualization spheres.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand|Debug")
	bool bShowDebugVisualization = false;

	/// Debug sphere color.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|PhysicsHand|Debug")
	FLinearColor DebugColor = FLinearColor(0.0f, 1.0f, 0.5f, 0.5f);

	// ── Components ──

	/// The root scene component.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ManusInteraction|PhysicsHand")
	USceneComponent* RootSceneComponent = nullptr;

	/// The hand driver component (velocity/force drive logic).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ManusInteraction|PhysicsHand")
	UMIHandDriver* HandDriver = nullptr;

	/// The haptic feedback manager.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ManusInteraction|PhysicsHand")
	UMIHapticFeedbackManager* HapticManager = nullptr;

	// ── Public API ──

	/// Initialize the physics hand. Call after setting SourceManusComponent.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|PhysicsHand")
	void InitializeHand();

	/// Get the physics finger component for a specific finger.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|PhysicsHand")
	UMIPhysicsFingerComponent* GetFingerComponent(EMIFingerName Finger) const;

	/// Get the state of all fingers.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|PhysicsHand")
	TArray<FMIFingerState> GetAllFingerStates() const;

	/// Get the palm physics component.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|PhysicsHand")
	UPrimitiveComponent* GetPalmComponent() const { return PalmBody; }

	/// Check if the hand has been initialized.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|PhysicsHand")
	bool IsHandInitialized() const { return bIsInitialized; }

private:
	/// Create physics finger body for a given finger.
	UMIPhysicsFingerComponent* CreateFingerBody(EMIFingerName Finger, FName BoneName);

	/// Create the palm physics body.
	void CreatePalmBody();

	/// Set default bone names based on standard Manus hand skeleton.
	void SetDefaultBoneNames();

	/// Physics finger components indexed by EMIFingerName.
	UPROPERTY()
	TMap<EMIFingerName, UMIPhysicsFingerComponent*> FingerComponents;

	/// Palm physics body.
	UPROPERTY()
	USphereComponent* PalmBody = nullptr;

	bool bIsInitialized = false;
};
