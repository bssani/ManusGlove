// Copyright 2024-2025 ManusInteraction Plugin

#include "Widget/MIWidgetSurface.h"
#include "Core/MIPhysicsHand.h"
#include "Core/MIPhysicsFingerComponent.h"
#include "ManusInteraction.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

UMIWidgetSurface::UMIWidgetSurface()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Initialize finger states
	for (int32 i = 0; i < static_cast<int32>(EMIFingerName::Max); ++i)
	{
		FingerTouchStates.Add(static_cast<EMIFingerName>(i), false);
	}
}

void UMIWidgetSurface::BeginPlay()
{
	Super::BeginPlay();

	if (TargetWidget)
	{
		CreateCollisionSurface();
	}
	else
	{
		// Try to find a widget component on the owning actor
		if (AActor* Owner = GetOwner())
		{
			TargetWidget = Owner->FindComponentByClass<UWidgetComponent>();
			if (TargetWidget)
			{
				CreateCollisionSurface();
			}
			else
			{
				UE_LOG(LogManusInteraction, Warning, TEXT("MIWidgetSurface: No UWidgetComponent found on %s"), *Owner->GetName());
			}
		}
	}
}

void UMIWidgetSurface::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TargetWidget || !CollisionSurface)
	{
		return;
	}

	// Find all physics hands and check finger proximity to the collision surface
	TArray<AActor*> PhysicsHands;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMIPhysicsHand::StaticClass(), PhysicsHands);

	for (AActor* HandActor : PhysicsHands)
	{
		AMIPhysicsHand* Hand = Cast<AMIPhysicsHand>(HandActor);
		if (!Hand || !Hand->IsHandInitialized())
		{
			continue;
		}

		for (int32 i = 0; i < static_cast<int32>(EMIFingerName::Max); ++i)
		{
			const EMIFingerName FingerName = static_cast<EMIFingerName>(i);
			UMIPhysicsFingerComponent* Finger = Hand->GetFingerComponent(FingerName);
			if (!Finger)
			{
				continue;
			}

			// Check if finger is overlapping with our collision surface
			const bool bIsTouching = Finger->IsOverlappingComponent(CollisionSurface);
			bool& bWasTouching = FingerTouchStates.FindOrAdd(FingerName);

			if (bIsTouching)
			{
				const FVector2D WidgetLocal = WorldToWidgetLocal(Finger->GetComponentLocation());

				if (!bWasTouching)
				{
					// Touch began
					bWasTouching = true;
					OnWidgetTouched.Broadcast(FingerName, WidgetLocal);
				}
				else
				{
					// Touch moved
					OnWidgetTouchMoved.Broadcast(FingerName, WidgetLocal);
				}
			}
			else if (bWasTouching)
			{
				// Touch ended
				bWasTouching = false;
				const FVector2D WidgetLocal = WorldToWidgetLocal(Finger->GetComponentLocation());
				OnWidgetTouchReleased.Broadcast(FingerName, WidgetLocal);
			}
		}
	}
}

FVector2D UMIWidgetSurface::WorldToWidgetLocal(FVector WorldPosition) const
{
	if (!TargetWidget)
	{
		return FVector2D::ZeroVector;
	}

	// Transform world position to widget component local space
	const FTransform WidgetTransform = TargetWidget->GetComponentTransform();
	const FVector LocalPos = WidgetTransform.InverseTransformPosition(WorldPosition);

	// Widget's draw size gives us the pixel dimensions
	const FVector2D DrawSize = TargetWidget->GetDrawSize();

	// Map from local 3D coords to 2D widget coords
	// Widget typically lies in the YZ plane with X as normal
	FVector2D WidgetLocal;
	WidgetLocal.X = (LocalPos.Y + DrawSize.X * 0.5f);
	WidgetLocal.Y = (-LocalPos.Z + DrawSize.Y * 0.5f);

	return WidgetLocal;
}

void UMIWidgetSurface::RefreshCollisionSurface()
{
	if (CollisionSurface && TargetWidget)
	{
		const FVector2D DrawSize = TargetWidget->GetDrawSize();
		const float ScaleW = DrawSize.X * 0.5f;
		const float ScaleH = DrawSize.Y * 0.5f;

		CollisionSurface->SetBoxExtent(FVector(CollisionThickness * 0.5f, ScaleW, ScaleH));

		// Position in front of the widget
		const FVector WidgetForward = TargetWidget->GetForwardVector();
		CollisionSurface->SetWorldLocation(TargetWidget->GetComponentLocation() + WidgetForward * CollisionOffset);
		CollisionSurface->SetWorldRotation(TargetWidget->GetComponentRotation());
	}
}

void UMIWidgetSurface::CreateCollisionSurface()
{
	if (!TargetWidget || !GetOwner())
	{
		return;
	}

	CollisionSurface = NewObject<UBoxComponent>(GetOwner(), TEXT("WidgetCollisionSurface"));
	CollisionSurface->SetupAttachment(TargetWidget);
	CollisionSurface->RegisterComponent();

	// Configure collision
	CollisionSurface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSurface->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	CollisionSurface->SetGenerateOverlapEvents(true);

	// Size to match widget
	RefreshCollisionSurface();

	// Make invisible
	CollisionSurface->SetVisibility(false);
	CollisionSurface->SetHiddenInGame(true);

	UE_LOG(LogManusInteraction, Log, TEXT("MIWidgetSurface: Created collision surface for widget on %s"),
		*GetOwner()->GetName());
}
