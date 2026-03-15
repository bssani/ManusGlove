// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MIInteractableBase.h"
#include "MISurfaceInteractable.generated.h"

/// @brief Touch surface interactable — for infotainment screens, touch panels, etc.
/// Detects finger contact on a flat surface and reports 2D touch coordinates.
/// Physics fingers stop at the surface (no penetration).
///
/// Usage:
/// 1. Add to an Actor with a StaticMesh (the touch panel)
/// 2. Set SurfaceExtent to match the panel size
/// 3. Bind OnSurfaceTouched/Moved/Released for input handling
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMISurfaceInteractable : public UMIInteractableBase
{
	GENERATED_BODY()

public:
	UMISurfaceInteractable();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ──

	/// The surface extent in local space (width, height in cm).
	/// Used for normalizing touch coordinates to 0-1 UV range.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Surface")
	FVector2D SurfaceExtent = FVector2D(50.0f, 30.0f);

	/// Local-space offset of the surface center from the actor origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Surface")
	FVector SurfaceOffset = FVector::ZeroVector;

	/// The local axis that represents the surface normal (default: +X forward).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Surface")
	EMIAxis SurfaceNormalAxis = EMIAxis::X;

	/// Maximum distance (cm) from surface for touch detection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Surface", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float TouchDetectionDistance = 2.0f;

	// ── Touch Events ──

	/// Fired when a finger touches the surface.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnSurfaceTouch,
		EMIFingerName, Finger,
		FVector2D, TouchUV,
		float, Pressure);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Surface")
	FOnSurfaceTouch OnSurfaceTouched;

	/// Fired when a touching finger moves on the surface.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Surface")
	FOnSurfaceTouch OnSurfaceMoved;

	/// Fired when a finger lifts off the surface.
	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Surface")
	FOnSurfaceTouch OnSurfaceReleased;

	// ── Public API ──

	/// Get the current touch UV coordinate for a specific finger.
	/// Returns (0,0) to (1,1) if touching, (-1,-1) if not touching.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Surface")
	FVector2D GetTouchLocationUV(EMIFingerName Finger) const;

	/// Convert a world position to surface UV coordinates.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Surface")
	FVector2D WorldToSurfaceUV(FVector WorldLocation) const;

	/// Get the number of fingers currently touching.
	UFUNCTION(BlueprintCallable, Category = "ManusInteraction|Surface")
	int32 GetActiveTouchCount() const;

private:
	/// Track per-finger touch state.
	struct FFingerTouchState
	{
		bool bIsTouching = false;
		FVector2D LastUV = FVector2D(-1.0f, -1.0f);
	};

	TMap<EMIFingerName, FFingerTouchState> FingerTouchStates;
};
