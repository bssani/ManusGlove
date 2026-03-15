// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "CoreMinimal.h"
#include "MITypes.generated.h"

/// @brief Drive mode for the physics hand bodies.
UENUM(BlueprintType)
enum class EMIHandDriveMode : uint8
{
	/// Drive physics bodies by setting linear/angular velocity each tick.
	Velocity	UMETA(DisplayName = "Velocity"),
	/// Drive physics bodies by applying forces each tick.
	Force		UMETA(DisplayName = "Force")
};

/// @brief Identifies a finger by name (mirrors EManusFingerName for decoupling).
UENUM(BlueprintType)
enum class EMIFingerName : uint8
{
	Thumb	UMETA(DisplayName = "Thumb"),
	Index	UMETA(DisplayName = "Index"),
	Middle	UMETA(DisplayName = "Middle"),
	Ring	UMETA(DisplayName = "Ring"),
	Pinky	UMETA(DisplayName = "Pinky"),
	Max		UMETA(Hidden)
};

/// @brief Hand side.
UENUM(BlueprintType)
enum class EMIHandSide : uint8
{
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right")
};

/// @brief Touch event types for surface interaction.
UENUM(BlueprintType)
enum class EMITouchEventType : uint8
{
	Began		UMETA(DisplayName = "Began"),
	Moved		UMETA(DisplayName = "Moved"),
	Ended		UMETA(DisplayName = "Ended")
};

/// @brief Hinge axis for interactables.
UENUM(BlueprintType)
enum class EMIAxis : uint8
{
	X	UMETA(DisplayName = "X"),
	Y	UMETA(DisplayName = "Y"),
	Z	UMETA(DisplayName = "Z")
};

/// @brief Data about a single finger's physics state.
USTRUCT(BlueprintType)
struct MANUSINTERACTION_API FMIFingerState
{
	GENERATED_BODY()

	/// Which finger this state belongs to.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction")
	EMIFingerName FingerName = EMIFingerName::Index;

	/// Current world location of the physics finger body.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction")
	FVector PhysicsLocation = FVector::ZeroVector;

	/// Current world location of the kinematic (Manus) finger target.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction")
	FVector KinematicLocation = FVector::ZeroVector;

	/// Distance between physics body and kinematic target (cm).
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction")
	float Divergence = 0.0f;

	/// Whether this finger is currently in contact with any surface.
	UPROPERTY(BlueprintReadOnly, Category = "ManusInteraction")
	bool bIsInContact = false;
};

/// @brief Configuration for physics hand behavior.
USTRUCT(BlueprintType)
struct MANUSINTERACTION_API FMIPhysicsHandConfig
{
	GENERATED_BODY()

	/// Maximum linear speed for finger physics bodies (cm/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float MaxLinearSpeed = 800.0f;

	/// Maximum angular speed for finger physics bodies (deg/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float MaxAngularSpeed = 1080.0f;

	/// Radius of fingertip physics sphere bodies (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "0.3", ClampMax = "5.0"))
	float FingerTipRadius = 1.0f;

	/// Radius of palm physics sphere body (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float PalmRadius = 3.0f;

	/// Drive mode for the physics bodies.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction")
	EMIHandDriveMode DriveMode = EMIHandDriveMode::Velocity;

	/// Mass of each fingertip physics body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float FingerTipMass = 0.05f;

	/// Mass of palm physics body (kg).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float PalmMass = 0.2f;

	/// Linear damping on physics bodies.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float LinearDamping = 1.0f;

	/// Angular damping on physics bodies.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ManusInteraction", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float AngularDamping = 1.0f;
};

/// @brief Collision profile names used by ManusInteraction.
namespace MICollision
{
	/// Default collision profile for physics finger bodies.
	static const FName PhysicsFingerProfile = TEXT("PhysicsFinger");

	/// Default collision channel name for interaction queries.
	static const FName InteractionChannel = TEXT("ManusInteraction");
}
