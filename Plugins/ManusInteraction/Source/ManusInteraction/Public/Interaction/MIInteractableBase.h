// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIInteractableBase.generated.h"

/// @brief Base class for all physics-based interactable components.
/// Attach this (or a derived class) to any Actor with a physics-enabled mesh
/// to make it interactable with the physics hand system.
///
/// Subclasses: MIHingeInteractable, MIButtonInteractable, MIDialInteractable,
///             MISliderInteractable, MISurfaceInteractable
UCLASS(Abstract, Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIInteractableBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIInteractableBase();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// Which fingers can interact with this object. Empty = all fingers allowed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Interactable")
	TArray<EMIFingerName> AllowedFingers;

	/// Enable haptic feedback when interacting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Interactable")
	bool bEnableHapticFeedback = true;

	/// Haptic feedback intensity (0.0 to 1.0).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Interactable", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableHapticFeedback"))
	float HapticStrength = 0.5f;

	/// Whether this interactable is currently enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Interactable")
	bool bIsEnabled = true;

	// ── Events ──

	/// Fired when interaction begins (finger makes contact).
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnMIInteraction,
		EMIFingerName, Finger,
		float, Value);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Interactable")
	FOnMIInteraction OnInteractionBegin;

	/// Fired each tick during interaction with updated value.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Interactable")
	FOnMIInteraction OnInteractionUpdate;

	/// Fired when interaction ends (finger leaves contact).
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Interactable")
	FOnMIInteraction OnInteractionEnd;

	// ── Public API ──

	/// Check if a finger is allowed to interact with this object.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Interactable")
	bool IsFingerAllowed(EMIFingerName Finger) const;

	/// Get the current normalized value (0.0 to 1.0) — meaning depends on subclass.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Interactable")
	virtual float GetCurrentValue() const { return CurrentValue; }

	/// Whether this interactable is currently being interacted with.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Interactable")
	bool IsBeingInteracted() const { return bIsBeingInteracted; }

protected:
	/// Current normalized value (0.0 to 1.0).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Interactable")
	float CurrentValue = 0.0f;

	/// Whether currently being interacted with.
	bool bIsBeingInteracted = false;

	/// The finger currently interacting, if any.
	EMIFingerName ActiveFinger = EMIFingerName::Index;

	/// Called by subclasses when interaction starts.
	void NotifyInteractionBegin(EMIFingerName Finger, float Value);

	/// Called by subclasses when interaction updates.
	void NotifyInteractionUpdate(EMIFingerName Finger, float Value);

	/// Called by subclasses when interaction ends.
	void NotifyInteractionEnd(EMIFingerName Finger, float Value);

	/// Find the physics mesh component on the owning actor (utility for subclasses).
	UPrimitiveComponent* FindPhysicsMesh() const;
};
