/* Copyright (C) PoyoWorks 2019, Inc - All Rights Reserved
*
* ArcadeVS - Arcade Vehicle System
* poyoworks@gmail.com
*
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
*
*/


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArcadeVSAnimator.generated.h"


class AArcadeVSVehicle;
class UArcadeVSBaseAnimInstance;

/**
	This component will compute values for the VehicleAnimationBlueprint based on its parent vehicle state.
	This means that all visible animations (tilting / Rolling / wheels Turning and moving) are all purely visual effects and have no relation to the 
	actual vehicle physics body. 
	This is very handy as it means that changing physics setting on the vehicle will not change its animation. 
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARCADEVS_API UArcadeVSAnimator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UArcadeVSAnimator();

    // Properties

    /** The time it will take the wheels to go back to neutral from their maximum turning angle
    (This is visual only and will not affect gameplay) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Wheels Direction", meta = (DisplayName = "Wheels Max Rotation Angle", ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "180"))
        int _wheelsMaxDirectionAngle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Wheels Direction", meta = (DisplayName = "Wheels Rotation back to neutral duration"))
        float _wheelsRotationToNeutralDuration;


    /** The max tilt angle (Y-Axis) the car will reach when accelerating (parameter A of the equation https://www.geogebra.org/m/zXU3HUpm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Acceleration Tilt Max Angle", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
        float _accelerationTiltMaxAngle;

	/** The max tilt angle (Y-Axis) the car will reach when braking (parameter A of the equation https://www.geogebra.org/m/zXU3HUpm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Braking Tilt Max Angle", ClampMin = "-50", ClampMax = "0", UIMin = "-50", UIMax = "0"))
        float _brakingTiltMaxAngle;

	/** How fast the acceleration oscillations will stabilize (parameter c of the equation https://www.geogebra.org/m/zXU3HUpm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Acceleration Tilt Damping", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
        float _accelerationTiltDamping;

	/** How fast the braking oscillations will will stabilize (parameter c of the equation https://www.geogebra.org/m/zXU3HUpm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Braking Tilt Damping", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
        float _brakingTiltDamping;

	/** How fast the vehicle will oscillate when accelerating (parameter w of the equation https://www.geogebra.org/m/zXU3HUpm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Acceleration Tilt Oscillation Speed", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
        float _accelerationTiltOscSpeed;

	/** How fast the vehicle will oscillate when braking (parameter w of the equation https://www.geogebra.org/m/zXU3HUpm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Braking Tilt Oscillation Speed", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
        float _brakingTiltOscSpeed;

	/** Max roll angle the car will reach when turning */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Max Roll Angle", ClampMin = "0", ClampMax = "45", UIMin = "0", UIMax = "45"))
        float _maxRollAngle;

	/** Scale applied to the computed roll angle, needed as it may be hard to get the 
	expected roll angle as the suspension will try to keep the vehicle on the ground */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Roll Scale", ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10"))
        float _rollScale;

	/** Will scale the tilt angle based on the current speed. Ie accelerating when already going fast will produce less tilt movement than accelerating when stopped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Frame Tilt and Roll", meta = (DisplayName = "Scale Tilt with speed"))
		float _scaleTiltWithSpeed;


protected:
    // Called when the game starts
    virtual void BeginPlay() override;
	 
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void Activate(bool bReset = false) { SetActive(true); }
	virtual void Deactivate() { SetActive(false); };
	virtual void SetActive(bool bNewActive, bool bReset = false) { _isActive = bNewActive; }

    virtual void UpdateWheelsDirection();
    virtual void UpdateWheelsRotation();
    virtual void UpdateWheelsSuspensions();
    virtual void UpdateDriftDirection();
    virtual void UpdateTiltAndRoll();


protected:

    UPROPERTY()
    AArcadeVSVehicle* _vehicle;

    UPROPERTY()
    UArcadeVSBaseAnimInstance* _animInstance;

    // Tilt and Roll
    float _maxTiltAngle;
    float _tiltOscSpeed;
    float _tiltDamping;
    float _elapsedTimeSinceReset;
    bool _resetTilt;
	bool _directionChanged;
    bool _isActive;

	float _axisScale;

    // Wheels rotation
    float _wheelRotationAngle;

    // Wheels direction
    float _wheelsDirection;
    float _wheelsTargetDirection;
    float _wheelsPreviousDirection;
    float _wheelsDirectionTimer;
    float _wheelsDirectionChangeDuration;

    bool _isVehicleAccelerating;
    bool _isVehicleBraking;
};
