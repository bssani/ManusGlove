// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Utils/MITypes.h"
#include "MIPhysicsFingerComponent.generated.h"

/// @brief A sphere physics body that represents a single fingertip.
/// Driven toward a kinematic bone target by MIHandDriver each tick.
/// Participates in UE5 physics simulation — collides with surfaces and stops.
UCLASS(ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIPhysicsFingerComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UMIPhysicsFingerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// ── Configuration ──

	/// The bone name on the source UManusComponent to track.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Finger")
	FName TargetBoneName;

	/// Which finger this component represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Finger")
	EMIFingerName FingerName = EMIFingerName::Index;

	// ── Runtime State ──

	/// Distance between this physics body and its kinematic target (cm).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Finger")
	float CurrentDivergence = 0.0f;

	/// Whether this finger is currently touching any surface.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Finger")
	bool bIsInContact = false;

	/// The actor currently being touched, if any.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Finger")
	TWeakObjectPtr<AActor> ContactActor;

	// ── Events ──

	/// Fired when this finger begins contact with another actor.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnFingerContactBegin,
		EMIFingerName, Finger,
		AActor*, OtherActor,
		const FHitResult&, HitResult);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Finger")
	FOnFingerContactBegin OnFingerContactBegin;

	/// Fired when this finger ends contact with another actor.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnFingerContactEnd,
		EMIFingerName, Finger,
		AActor*, OtherActor);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Finger")
	FOnFingerContactEnd OnFingerContactEnd;

	// ── Internal ──

	/// Update the kinematic target location (called by MIHandDriver).
	void SetKinematicTarget(const FVector& TargetLocation, const FQuat& TargetRotation);

	/// Get the current kinematic target location.
	FVector GetKinematicTargetLocation() const { return KinematicTargetLocation; }

	/// Get the current kinematic target rotation.
	FQuat GetKinematicTargetRotation() const { return KinematicTargetRotation; }

private:
	UFUNCTION()
	void HandleComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	FVector KinematicTargetLocation = FVector::ZeroVector;
	FQuat KinematicTargetRotation = FQuat::Identity;

	/// Track the number of overlapping actors to manage bIsInContact.
	int32 OverlapCount = 0;
};
