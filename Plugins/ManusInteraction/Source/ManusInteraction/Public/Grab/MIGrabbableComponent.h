// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MIGrabbableComponent.generated.h"

/// @brief Marks an Actor as physics-grabbable by the physics hand system.
/// Add this component to any Actor that should be pickable via physics grab.
///
/// The Actor's root mesh must have physics simulation enabled, or this component
/// will enable it automatically on BeginPlay.
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIGrabbableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIGrabbableComponent();

	virtual void BeginPlay() override;

	// ── Configuration ──

	/// Whether this object is currently grabbable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grabbable")
	bool bIsGrabbable = true;

	/// Whether to apply gravity when released (dropped).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grabbable")
	bool bUseGravityWhenReleased = true;

	/// Whether to enable gravity while held (false = object floats while grabbed).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grabbable")
	bool bUseGravityWhileHeld = false;

	/// Maximum mass that can be grabbed (kg). 0 = no limit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grabbable", meta = (ClampMin = "0.0"))
	float MaxGrabbableMass = 0.0f;

	// ── State ──

	/// Whether this object is currently being held.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Grabbable")
	bool bIsHeld = false;

	// ── Events ──

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrabbableEvent, AActor*, GrabbingActor);

	/// Fired when this object is grabbed.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Grabbable")
	FOnGrabbableEvent OnGrabbed;

	/// Fired when this object is released.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Grabbable")
	FOnGrabbableEvent OnReleased;

	// ── Internal ──

	/// Called by MIPhysicsGrabComponent when this object is grabbed.
	void NotifyGrabbed(AActor* GrabbingActor);

	/// Called by MIPhysicsGrabComponent when this object is released.
	void NotifyReleased(AActor* GrabbingActor);

private:
	/// Cache the original gravity setting.
	bool bOriginalGravity = true;
};
