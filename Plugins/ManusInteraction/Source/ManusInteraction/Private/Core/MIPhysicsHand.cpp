// Copyright 2024-2025 ManusInteraction Plugin

#include "Core/MIPhysicsHand.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "Core/MIHandDriver.h"
#include "Haptics/MIHapticFeedbackManager.h"
#include "ManusComponent.h"
#include "ManusInteraction.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

AMIPhysicsHand::AMIPhysicsHand()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	HandDriver = CreateDefaultSubobject<UMIHandDriver>(TEXT("HandDriver"));
	HapticManager = CreateDefaultSubobject<UMIHapticFeedbackManager>(TEXT("HapticManager"));
}

void AMIPhysicsHand::BeginPlay()
{
	Super::BeginPlay();

	if (SourceManusComponent && !bIsInitialized)
	{
		InitializeHand();
	}
}

void AMIPhysicsHand::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
	if (bShowDebugVisualization && bIsInitialized)
	{
		for (auto& Pair : FingerComponents)
		{
			if (UMIPhysicsFingerComponent* Finger = Pair.Value)
			{
				const FColor Color = Finger->bIsInContact
					? FColor::Red
					: FLinearColor(DebugColor).ToFColor(true);
				DrawDebugSphere(
					GetWorld(),
					Finger->GetComponentLocation(),
					Finger->GetUnscaledSphereRadius(),
					8,
					Color,
					false,
					-1.0f,
					0,
					0.5f);

				// Draw line from physics body to kinematic target
				DrawDebugLine(
					GetWorld(),
					Finger->GetComponentLocation(),
					Finger->GetKinematicTargetLocation(),
					FColor::Yellow,
					false,
					-1.0f,
					0,
					0.3f);
			}
		}

		if (PalmBody)
		{
			DrawDebugSphere(
				GetWorld(),
				PalmBody->GetComponentLocation(),
				PalmBody->GetUnscaledSphereRadius(),
				8,
				FLinearColor(DebugColor).ToFColor(true),
				false,
				-1.0f,
				0,
				0.5f);
		}
	}
#endif
}

void AMIPhysicsHand::InitializeHand()
{
	if (!SourceManusComponent)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MIPhysicsHand: Cannot initialize — SourceManusComponent is null."));
		return;
	}

	if (bIsInitialized)
	{
		UE_LOG(LogManusInteraction, Warning, TEXT("MIPhysicsHand: Already initialized."));
		return;
	}

	// Set default bone names if not configured
	if (FingerTipBoneNames.Num() == 0)
	{
		SetDefaultBoneNames();
	}

	// Create physics bodies for each finger
	for (auto& Pair : FingerTipBoneNames)
	{
		UMIPhysicsFingerComponent* Finger = CreateFingerBody(Pair.Key, Pair.Value);
		if (Finger)
		{
			FingerComponents.Add(Pair.Key, Finger);
		}
	}

	// Create palm body
	CreatePalmBody();

	// Initialize the hand driver
	TArray<UMIPhysicsFingerComponent*> FingerArray;
	FingerComponents.GenerateValueArray(FingerArray);

	HandDriver->MaxLinearSpeed = Config.MaxLinearSpeed;
	HandDriver->MaxAngularSpeed = Config.MaxAngularSpeed;
	HandDriver->DriveMode = Config.DriveMode;
	HandDriver->Initialize(SourceManusComponent, FingerArray, PalmBody);

	// Initialize haptic manager
	if (HapticManager)
	{
		HapticManager->Initialize(SourceManusComponent, FingerArray, HandSide);
	}

	bIsInitialized = true;

	UE_LOG(LogManusInteraction, Log, TEXT("MIPhysicsHand: Initialized %s hand with %d fingers."),
		HandSide == EMIHandSide::Left ? TEXT("Left") : TEXT("Right"),
		FingerComponents.Num());
}

UMIPhysicsFingerComponent* AMIPhysicsHand::GetFingerComponent(EMIFingerName Finger) const
{
	const auto* Found = FingerComponents.Find(Finger);
	return Found ? *Found : nullptr;
}

TArray<FMIFingerState> AMIPhysicsHand::GetAllFingerStates() const
{
	TArray<FMIFingerState> States;
	for (const auto& Pair : FingerComponents)
	{
		if (UMIPhysicsFingerComponent* Finger = Pair.Value)
		{
			FMIFingerState State;
			State.FingerName = Finger->FingerName;
			State.PhysicsLocation = Finger->GetComponentLocation();
			State.KinematicLocation = Finger->GetKinematicTargetLocation();
			State.Divergence = Finger->CurrentDivergence;
			State.bIsInContact = Finger->bIsInContact;
			States.Add(State);
		}
	}
	return States;
}

UMIPhysicsFingerComponent* AMIPhysicsHand::CreateFingerBody(EMIFingerName Finger, FName BoneName)
{
	const FString FingerStr = StaticEnum<EMIFingerName>()->GetValueAsString(Finger);
	const FName ComponentName = FName(*FString::Printf(TEXT("Finger_%s"), *FingerStr));

	UMIPhysicsFingerComponent* FingerComp = NewObject<UMIPhysicsFingerComponent>(this, ComponentName);
	if (!FingerComp)
	{
		return nullptr;
	}

	FingerComp->SetupAttachment(RootSceneComponent);
	FingerComp->RegisterComponent();

	FingerComp->FingerName = Finger;
	FingerComp->TargetBoneName = BoneName;
	FingerComp->SetSphereRadius(Config.FingerTipRadius);
	FingerComp->SetMassOverrideInKg(NAME_None, Config.FingerTipMass);
	FingerComp->SetLinearDamping(Config.LinearDamping);
	FingerComp->SetAngularDamping(Config.AngularDamping);

	// Set initial position to bone location
	if (SourceManusComponent)
	{
		const FVector BoneLocation = SourceManusComponent->GetSocketLocation(BoneName);
		FingerComp->SetWorldLocation(BoneLocation);
	}

	return FingerComp;
}

void AMIPhysicsHand::CreatePalmBody()
{
	PalmBody = NewObject<USphereComponent>(this, TEXT("Palm"));
	if (!PalmBody)
	{
		return;
	}

	PalmBody->SetupAttachment(RootSceneComponent);
	PalmBody->RegisterComponent();
	PalmBody->SetSphereRadius(Config.PalmRadius);
	PalmBody->SetSimulatePhysics(true);
	PalmBody->SetEnableGravity(false);
	PalmBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PalmBody->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	PalmBody->SetGenerateOverlapEvents(true);
	PalmBody->BodyInstance.bNotifyRigidBodyCollision = true;
	PalmBody->BodyInstance.bUseCCD = true;
	PalmBody->SetMassOverrideInKg(NAME_None, Config.PalmMass);
	PalmBody->SetLinearDamping(Config.LinearDamping);
	PalmBody->SetAngularDamping(Config.AngularDamping);

	// Set initial position
	if (SourceManusComponent)
	{
		if (!PalmBoneName.IsNone())
		{
			PalmBody->SetWorldLocation(SourceManusComponent->GetSocketLocation(PalmBoneName));
		}
		else
		{
			PalmBody->SetWorldLocation(SourceManusComponent->GetComponentLocation());
		}
	}
}

void AMIPhysicsHand::SetDefaultBoneNames()
{
	// Default bone names for the standard Manus hand skeleton.
	// These may need adjustment depending on the actual skeleton used.
	const FString Side = (HandSide == EMIHandSide::Left) ? TEXT("l") : TEXT("r");

	FingerTipBoneNames.Add(EMIFingerName::Thumb, FName(*FString::Printf(TEXT("%s_finger_thumb_03_end"), *Side)));
	FingerTipBoneNames.Add(EMIFingerName::Index, FName(*FString::Printf(TEXT("%s_finger_index_03_end"), *Side)));
	FingerTipBoneNames.Add(EMIFingerName::Middle, FName(*FString::Printf(TEXT("%s_finger_middle_03_end"), *Side)));
	FingerTipBoneNames.Add(EMIFingerName::Ring, FName(*FString::Printf(TEXT("%s_finger_ring_03_end"), *Side)));
	FingerTipBoneNames.Add(EMIFingerName::Pinky, FName(*FString::Printf(TEXT("%s_finger_pinky_03_end"), *Side)));

	PalmBoneName = FName(*FString::Printf(TEXT("%s_hand_wrist"), *Side));
}
