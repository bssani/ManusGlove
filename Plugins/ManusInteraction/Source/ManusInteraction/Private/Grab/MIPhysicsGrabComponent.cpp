// Copyright 2024-2025 ManusInteraction Plugin

#include "Grab/MIPhysicsGrabComponent.h"
#include "Grab/MIGrabbableComponent.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusInteraction.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/PrimitiveComponent.h"

UMIPhysicsGrabComponent::UMIPhysicsGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UMIPhysicsGrabComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMIPhysicsGrabComponent::SetFingerComponents(const TArray<UMIPhysicsFingerComponent*>& InFingers)
{
	FingerComponents = InFingers;
}

void UMIPhysicsGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsGrabbing)
	{
		// Check if grab should be maintained
		if (!ShouldMaintainGrab())
		{
			EndGrab();
		}
	}
	else
	{
		// Try to detect a new grab
		AActor* Candidate = DetectGrabCandidate();
		if (Candidate)
		{
			StartGrab(Candidate);
		}
	}
}

AActor* UMIPhysicsGrabComponent::DetectGrabCandidate() const
{
	// Count how many fingers are contacting each actor
	TMap<AActor*, int32> ContactCounts;
	TMap<AActor*, bool> ThumbContacts;

	for (const UMIPhysicsFingerComponent* Finger : FingerComponents)
	{
		if (!Finger || !Finger->bIsInContact || !Finger->ContactActor.IsValid())
		{
			continue;
		}

		AActor* ContactedActor = Finger->ContactActor.Get();

		// Check if the actor has a grabbable component
		UMIGrabbableComponent* Grabbable = ContactedActor->FindComponentByClass<UMIGrabbableComponent>();
		if (!Grabbable || !Grabbable->bIsGrabbable || Grabbable->bIsHeld)
		{
			continue;
		}

		int32& Count = ContactCounts.FindOrAdd(ContactedActor, 0);
		Count++;

		if (Finger->FingerName == EMIFingerName::Thumb)
		{
			ThumbContacts.Add(ContactedActor, true);
		}
	}

	// Find the best candidate (most finger contacts)
	AActor* BestCandidate = nullptr;
	int32 BestCount = 0;

	for (auto& Pair : ContactCounts)
	{
		const bool bHasThumb = ThumbContacts.Contains(Pair.Key);
		if (Pair.Value >= MinContactFingers && (!bRequireThumb || bHasThumb))
		{
			if (Pair.Value > BestCount)
			{
				BestCount = Pair.Value;
				BestCandidate = Pair.Key;
			}
		}
	}

	return BestCandidate;
}

void UMIPhysicsGrabComponent::StartGrab(AActor* Target)
{
	if (!Target || bIsGrabbing)
	{
		return;
	}

	UPrimitiveComponent* TargetMesh = Cast<UPrimitiveComponent>(Target->GetRootComponent());
	if (!TargetMesh || !TargetMesh->IsSimulatingPhysics())
	{
		return;
	}

	// Find the palm component from the owning hand
	UPrimitiveComponent* PalmBody = nullptr;
	if (AActor* Owner = GetOwner())
	{
		// Look for a sphere component that could be the palm
		TArray<USphereComponent*> Spheres;
		Owner->GetComponents<USphereComponent>(Spheres);
		for (USphereComponent* Sphere : Spheres)
		{
			if (Sphere->GetName().Contains(TEXT("Palm")))
			{
				PalmBody = Sphere;
				break;
			}
		}
	}

	if (!PalmBody)
	{
		// Use the first finger as constraint anchor if no palm found
		for (UMIPhysicsFingerComponent* Finger : FingerComponents)
		{
			if (Finger && Finger->bIsInContact && Finger->ContactActor.Get() == Target)
			{
				PalmBody = Finger;
				break;
			}
		}
	}

	if (!PalmBody)
	{
		return;
	}

	// Create runtime physics constraint
	GrabConstraint = NewObject<UPhysicsConstraintComponent>(GetOwner(), TEXT("GrabConstraint"));
	GrabConstraint->SetupAttachment(GetOwner()->GetRootComponent());
	GrabConstraint->RegisterComponent();

	GrabConstraint->SetConstrainedComponents(PalmBody, NAME_None, TargetMesh, NAME_None);

	// Configure as a semi-rigid grab (6DOF with spring)
	GrabConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Free, 0.0f);
	GrabConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, 0.0f);
	GrabConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, 0.0f);

	GrabConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
	GrabConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
	GrabConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);

	// Set linear drive (spring-like behavior)
	GrabConstraint->SetLinearPositionDrive(true, true, true);
	GrabConstraint->SetLinearVelocityDrive(true, true, true);
	GrabConstraint->SetLinearDriveParams(GrabStiffness, GrabDamping, BreakForce);

	// Set angular drive
	GrabConstraint->SetAngularOrientationDrive(true, true);
	GrabConstraint->SetAngularVelocityDrive(true, true);
	GrabConstraint->SetAngularDriveParams(GrabStiffness, GrabDamping, BreakTorque);

	// Set break thresholds
	if (BreakForce > 0.0f)
	{
		GrabConstraint->ConstraintInstance.ProfileInstance.bLinearBreakable = true;
		GrabConstraint->ConstraintInstance.ProfileInstance.LinearBreakThreshold = BreakForce;
	}
	if (BreakTorque > 0.0f)
	{
		GrabConstraint->ConstraintInstance.ProfileInstance.bAngularBreakable = true;
		GrabConstraint->ConstraintInstance.ProfileInstance.AngularBreakThreshold = BreakTorque;
	}

	CurrentlyGrabbedActor = Target;
	bIsGrabbing = true;

	// Notify the grabbable component
	UMIGrabbableComponent* Grabbable = Target->FindComponentByClass<UMIGrabbableComponent>();
	if (Grabbable)
	{
		Grabbable->NotifyGrabbed(GetOwner());
	}

	OnGrabbed.Broadcast(Target);

	UE_LOG(LogManusInteraction, Log, TEXT("MIPhysicsGrabComponent: Grabbed %s"), *Target->GetName());
}

void UMIPhysicsGrabComponent::EndGrab()
{
	if (!bIsGrabbing)
	{
		return;
	}

	AActor* ReleasedActor = CurrentlyGrabbedActor;

	// Destroy the constraint
	if (GrabConstraint)
	{
		GrabConstraint->DestroyComponent();
		GrabConstraint = nullptr;
	}

	// Notify the grabbable component
	if (ReleasedActor)
	{
		UMIGrabbableComponent* Grabbable = ReleasedActor->FindComponentByClass<UMIGrabbableComponent>();
		if (Grabbable)
		{
			Grabbable->NotifyReleased(GetOwner());
		}
	}

	CurrentlyGrabbedActor = nullptr;
	bIsGrabbing = false;

	OnReleased.Broadcast(ReleasedActor);

	UE_LOG(LogManusInteraction, Log, TEXT("MIPhysicsGrabComponent: Released %s"),
		ReleasedActor ? *ReleasedActor->GetName() : TEXT("null"));
}

void UMIPhysicsGrabComponent::ForceRelease()
{
	EndGrab();
}

bool UMIPhysicsGrabComponent::ShouldMaintainGrab() const
{
	if (!CurrentlyGrabbedActor)
	{
		return false;
	}

	// Check if the constraint was broken by physics
	if (GrabConstraint && GrabConstraint->IsBroken())
	{
		return false;
	}

	// Count fingers still in contact with the grabbed actor
	int32 ContactCount = 0;
	bool bThumbInContact = false;

	for (const UMIPhysicsFingerComponent* Finger : FingerComponents)
	{
		if (Finger && Finger->bIsInContact && Finger->ContactActor.Get() == CurrentlyGrabbedActor)
		{
			ContactCount++;
			if (Finger->FingerName == EMIFingerName::Thumb)
			{
				bThumbInContact = true;
			}
		}
	}

	// Release if not enough fingers (use a lower threshold for release to avoid flickering)
	const int32 ReleaseThreshold = FMath::Max(1, MinContactFingers - 1);
	if (ContactCount < ReleaseThreshold)
	{
		return false;
	}

	return true;
}
