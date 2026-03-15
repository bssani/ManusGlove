// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "Interaction/MIInteractableBase.h"
#include "MIButtonInteractable.generated.h"

/// @brief Push-button interactable — linear press along one axis.
/// Covers: AC buttons, window switches, start buttons, all push buttons.
///
/// Usage:
/// 1. Create Actor Blueprint with a StaticMesh (the button cap)
/// 2. Add MIButtonInteractable component
/// 3. Set PressAxis, PressDepth, optional bIsToggle
/// 4. Bind OnButtonPressed/Released
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class MANUSINTERACTION_API UMIButtonInteractable : public UMIInteractableBase
{
	GENERATED_BODY()

public:
	UMIButtonInteractable();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Button Configuration ──

	/// The axis along which the button presses (in local space).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button")
	EMIAxis PressAxis = EMIAxis::X;

	/// Maximum press depth (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float PressDepth = 1.0f;

	/// Spring return force — the button springs back when released.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float ReturnForce = 100.0f;

	/// Activation threshold (normalized 0-1). The button is "pressed" when depth exceeds this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ActivationThreshold = 0.7f;

	/// Whether this is a toggle button (stays pressed/released on each push).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button")
	bool bIsToggle = false;

	/// Mass of the button body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction|Button", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float ButtonMass = 0.02f;

	// ── State ──

	/// Whether the button is currently in the "pressed" state.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Button")
	bool bIsPressed = false;

	/// For toggle buttons, the current toggle state.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Button")
	bool bToggleState = false;

	/// Current press depth (0.0 to 1.0 normalized).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction|Button")
	float CurrentDepth = 0.0f;

	// ── Events ──

	/// Fired when the button is pressed past the activation threshold.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonPressed);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Button")
	FOnButtonPressed OnButtonPressed;

	/// Fired when the button returns above the activation threshold.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonReleased);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Button")
	FOnButtonReleased OnButtonReleased;

	/// Fired when toggle state changes (only for toggle buttons).
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonToggled, bool, bNewState);

	UPROPERTY(BlueprintAssignable, Category = "ManusInteraction|Button")
	FOnButtonToggled OnButtonToggled;

private:
	/// The physics constraint for linear movement.
	UPROPERTY()
	class UPhysicsConstraintComponent* ButtonConstraint = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> ButtonMesh;

	FVector InitialLocalPosition = FVector::ZeroVector;

	void SetupPhysicsConstraint();
	void UpdateDepthFromPhysics();

	bool bWasPressedLastFrame = false;
};
