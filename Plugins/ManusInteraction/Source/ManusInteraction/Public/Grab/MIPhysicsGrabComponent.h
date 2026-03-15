// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIPhysicsGrabComponent.generated.h"

class UMIPhysicsFingerComponent;
class UPhysicsConstraintComponent;
class UMIGrabbableComponent;

/// @brief Physics-based grab component. Detects when enough fingers contact
/// a grabbable object and creates a runtime physics constraint to hold it.
/// The object stays in the physics simulation while grabbed (not attached).
///
/// Attach this to a MIPhysicsHand actor.
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIPhysicsGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIPhysicsGrabComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// Minimum number of fingers that must be in contact to trigger a grab.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MinContactFingers = 2;

	/// Whether the thumb must be one of the contacting fingers.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab")
	bool bRequireThumb = true;

	/// Stiffness of the grab constraint (higher = more rigid hold).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab", meta = (ClampMin = "100.0", ClampMax = "50000.0"))
	float GrabStiffness = 5000.0f;

	/// Damping of the grab constraint.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab", meta = (ClampMin = "10.0", ClampMax = "5000.0"))
	float GrabDamping = 500.0f;

	/// Force at which the grab constraint breaks (N). 0 = unbreakable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab", meta = (ClampMin = "0.0"))
	float BreakForce = 10000.0f;

	/// Torque at which the grab constraint breaks. 0 = unbreakable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Grab", meta = (ClampMin = "0.0"))
	float BreakTorque = 10000.0f;

	// ── State ──

	/// The actor currently being grabbed, if any.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Grab")
	AActor* CurrentlyGrabbedActor = nullptr;

	/// Whether currently holding an object.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Grab")
	bool bIsGrabbing = false;

	// ── Events ──

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrabEvent, AActor*, GrabbedActor);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Grab")
	FOnGrabEvent OnGrabbed;

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Grab")
	FOnGrabEvent OnReleased;

	// ── Public API ──

	/// Set the finger components to monitor for grab detection.
	void SetFingerComponents(const TArray<UMIPhysicsFingerComponent*>& InFingers);

	/// Force release the currently grabbed object.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Grab")
	void ForceRelease();

private:
	UPROPERTY()
	TArray<UMIPhysicsFingerComponent*> FingerComponents;

	UPROPERTY()
	UPhysicsConstraintComponent* GrabConstraint = nullptr;

	/// Detect if a grab should start.
	AActor* DetectGrabCandidate() const;

	/// Start grabbing an actor.
	void StartGrab(AActor* Target);

	/// End the current grab.
	void EndGrab();

	/// Check if the grab should be maintained.
	bool ShouldMaintainGrab() const;
};
