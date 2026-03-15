// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/MITypes.h"
#include "MIWidgetSurface.generated.h"

class UWidgetComponent;
class UBoxComponent;

/// @brief Bridges physics finger interaction with UMG widget surfaces.
/// Creates a thin collision surface in front of a UWidgetComponent to prevent
/// finger penetration, and converts finger contact positions to widget coordinates
/// for synthetic pointer events.
///
/// Usage:
/// 1. Add to an Actor that has a UWidgetComponent
/// 2. Set TargetWidget reference
/// 3. The component auto-creates a collision surface matching the widget size
/// 4. Physics fingers will stop at the surface and trigger widget interactions
///
/// Limitation: Synthetic widget events use non-public UE5 APIs and may need
/// adjustment across engine versions. For maximum stability, consider using
/// 3D mesh buttons (MIButtonInteractable) instead of UMG widgets.
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIWidgetSurface : public UActorComponent
{
	GENERATED_BODY()

public:
	UMIWidgetSurface();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// The UWidgetComponent to protect and interact with.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|WidgetSurface")
	UWidgetComponent* TargetWidget = nullptr;

	/// Thickness of the collision surface (cm). Keep thin but not zero.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|WidgetSurface", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float CollisionThickness = 0.5f;

	/// Offset of the collision surface from the widget (cm). Positive = in front.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|WidgetSurface", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CollisionOffset = 0.2f;

	/// Whether to send synthetic pointer events to the widget on touch.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|WidgetSurface")
	bool bSendWidgetEvents = true;

	// ── Events ──

	/// Fired when a finger touches the widget surface.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnWidgetTouched,
		EMIFingerName, Finger,
		FVector2D, WidgetLocalPosition);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|WidgetSurface")
	FOnWidgetTouched OnWidgetTouched;

	/// Fired when a finger moves on the widget surface.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|WidgetSurface")
	FOnWidgetTouched OnWidgetTouchMoved;

	/// Fired when a finger lifts from the widget surface.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|WidgetSurface")
	FOnWidgetTouched OnWidgetTouchReleased;

	// ── Public API ──

	/// Convert a world position to widget local coordinates.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|WidgetSurface")
	FVector2D WorldToWidgetLocal(FVector WorldPosition) const;

	/// Refresh the collision surface size to match the widget.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|WidgetSurface")
	void RefreshCollisionSurface();

private:
	/// The auto-generated collision surface.
	UPROPERTY()
	UBoxComponent* CollisionSurface = nullptr;

	/// Create/update the collision surface.
	void CreateCollisionSurface();

	/// Track per-finger touch state for widget events.
	TMap<EMIFingerName, bool> FingerTouchStates;
};
