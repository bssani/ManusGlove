// Copyright 2024-2025 ManusInteraction Plugin

#include "Core/MIPhysicsFingerComponent.h"
#include "ManusInteraction.h"

UMIPhysicsFingerComponent::UMIPhysicsFingerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Physics setup
	SetSimulatePhysics(true);
	SetEnableGravity(false);
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	SetGenerateOverlapEvents(true);
	BodyInstance.bNotifyRigidBodyCollision = true;

	// Sphere defaults
	SetSphereRadius(1.0f);

	// CCD to prevent tunneling through thin surfaces
	BodyInstance.bUseCCD = true;

	// Bind collision/overlap events
	OnComponentHit.AddDynamic(this, &UMIPhysicsFingerComponent::HandleComponentHit);
	OnComponentBeginOverlap.AddDynamic(this, &UMIPhysicsFingerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UMIPhysicsFingerComponent::HandleEndOverlap);
}

void UMIPhysicsFingerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Set mass (lightweight fingertip)
	SetMassOverrideInKg(NAME_None, 0.05f);
	SetLinearDamping(1.0f);
	SetAngularDamping(1.0f);
}

void UMIPhysicsFingerComponent::SetKinematicTarget(const FVector& TargetLocation, const FQuat& TargetRotation)
{
	KinematicTargetLocation = TargetLocation;
	KinematicTargetRotation = TargetRotation;

	// Calculate divergence
	CurrentDivergence = FVector::Dist(GetComponentLocation(), TargetLocation);
}

void UMIPhysicsFingerComponent::HandleComponentHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (OtherActor && OtherActor != GetOwner())
	{
		bIsInContact = true;
		ContactActor = OtherActor;
		OnFingerContactBegin.Broadcast(FingerName, OtherActor, Hit);
	}
}

void UMIPhysicsFingerComponent::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != GetOwner())
	{
		OverlapCount++;
		bIsInContact = true;
		ContactActor = OtherActor;
		OnFingerContactBegin.Broadcast(FingerName, OtherActor, SweepResult);
	}
}

void UMIPhysicsFingerComponent::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor != GetOwner())
	{
		OverlapCount = FMath::Max(0, OverlapCount - 1);
		if (OverlapCount == 0)
		{
			bIsInContact = false;
			ContactActor = nullptr;
		}
		OnFingerContactEnd.Broadcast(FingerName, OtherActor);
	}
}
