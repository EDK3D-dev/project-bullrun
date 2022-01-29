// Fill out your copyright notice in the Description page of Project Settings.

#include "ArcadeVSAnimator.h"

#include <Components/SkeletalMeshComponent.h>

#include "ArcadeVSVehicle.h"
#include "ArcadeVSBaseAnimInstance.h"


extern float speedToKmH(float speed);
extern float speedToCmS(float speed);


// Sets default values for this component's properties
UArcadeVSAnimator::UArcadeVSAnimator()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetAutoActivate(true);

    // Default values
    _wheelsMaxDirectionAngle = 35;
    _wheelsDirection = 0.f;
    _wheelsTargetDirection = 0.f;
    _wheelsPreviousDirection = 0.f;
    _wheelsRotationToNeutralDuration = .5f;

    _accelerationTiltMaxAngle = 3.f;
    _brakingTiltMaxAngle = -3.f;
    _accelerationTiltDamping = 3.f;
    _accelerationTiltOscSpeed = 10.f;
    _brakingTiltDamping = 2.f;
    _brakingTiltOscSpeed = 10.f;
    _tiltOscSpeed = 0.f;
    _maxTiltAngle = 0.f;
    _tiltDamping = 0.f;
	_scaleTiltWithSpeed = false;

    _maxRollAngle = 10.f;
    _rollScale = 3.f;
    _resetTilt = false;
	_directionChanged = false;

    _isVehicleAccelerating = false;
    _isVehicleBraking = false;

    _isActive = true;
}


// Called when the game starts
void UArcadeVSAnimator::BeginPlay()
{
	Super::BeginPlay();

    _vehicle = Cast<AArcadeVSVehicle>(GetOwner());
    if (!_vehicle)
    {
        UE_LOG(LogTemp, Error, TEXT("This component can only be added to an Actor inheriting ArcadeVSVehicle Actor"));
        return;
    }

    USkeletalMeshComponent* rootComponent = Cast<USkeletalMeshComponent>(_vehicle->GetRootComponent());
    _animInstance = Cast<UArcadeVSBaseAnimInstance>(rootComponent->GetAnimInstance());
	
    if (!_animInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("The actor skeletalMesh does not have a ArcadeVSBaseAnimInstance Animation Blueprint set"));
        return;
    }
}

// Called every frame
void UArcadeVSAnimator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!_isActive)
	    return;

    if (!_vehicle || !_animInstance)
        return;

    if (_vehicle->_accelerating && (_vehicle->_accelerating != _isVehicleAccelerating) && _vehicle->_onGround)
    {
        _resetTilt = true;
    }
    if (_vehicle->_braking && (_vehicle->_braking != _isVehicleBraking) && _vehicle->_onGround)
    {
        _resetTilt = true;
    }

    _isVehicleAccelerating = _vehicle->_accelerating;
    _isVehicleBraking = _vehicle->_braking;

    UpdateWheelsDirection();
    UpdateWheelsRotation();
    UpdateWheelsSuspensions();
    UpdateDriftDirection();
    UpdateTiltAndRoll();
}

void UArcadeVSAnimator::UpdateWheelsDirection()
{
    float newTargetDirection = _vehicle->_turningScale * _wheelsMaxDirectionAngle;
    if (newTargetDirection != _wheelsTargetDirection)
    {
        _wheelsDirectionTimer = 0.f;
        _wheelsPreviousDirection = _wheelsDirection;
        _wheelsTargetDirection = newTargetDirection;
        _wheelsDirectionChangeDuration = _wheelsRotationToNeutralDuration * (_wheelsTargetDirection - _wheelsPreviousDirection) / (_wheelsMaxDirectionAngle);
        _wheelsDirectionChangeDuration = FMath::Abs(_wheelsDirectionChangeDuration);
    }

    // We want to direction to be smoothed out

    // Linear easing equation c*t/d + b
    // t = currentTime / b = start value / c = change in value / d = duration
    if (_wheelsDirectionTimer < _wheelsDirectionChangeDuration) // Random epsilon
    {
        float changeInValue = (_wheelsTargetDirection - _wheelsPreviousDirection);
        _wheelsDirectionTimer += _vehicle->_deltaTime;
        _wheelsDirection = (changeInValue * _wheelsDirectionTimer) / _wheelsDirectionChangeDuration + _wheelsPreviousDirection;
    }
    else
    {
        _wheelsDirectionChangeDuration = 0.f;
        _wheelsDirectionTimer = 0.f;
        _wheelsDirection = _wheelsTargetDirection;
    }
    _animInstance->wheelDirection = _wheelsDirection;
}

void UArcadeVSAnimator::UpdateWheelsRotation()
{
    // Update wheels rotation 
    int rotation = 0;
    int rotationDirection = _vehicle->Speed > 0 ? -1 : 1;
    
    if (FMath::Abs(_vehicle->Speed) > 0.0)
    {
        // We know the radius, we can get the perimeter of the wheel (in cm) P = 2 * Pi * R
        float p = 2 * PI * _vehicle->_targetSuspensionHeight / 100.f;	// perimeter in meters
																		// We know the speed so we can get the amount of movement for delta time
        _vehicle->_lastFrameTraveledDistance = _vehicle->Speed * 1000 / 3600.f * _vehicle->_deltaTime;
        _wheelRotationAngle += (_vehicle->_lastFrameTraveledDistance / p) * 360;
    }
    else
    {
        _wheelRotationAngle = 0.f;
    }
    
    rotation = static_cast<int>(_wheelRotationAngle) % 360;
    _animInstance->wheelRotation = rotationDirection * rotation;
}

void UArcadeVSAnimator::UpdateWheelsSuspensions()
{
    // Update wheels offsets
    _animInstance->wheelsOffsets = _vehicle->_suspensionOffsets;
}

void UArcadeVSAnimator::UpdateDriftDirection()
{
    // Update drift
    if (_vehicle->_drifting)
    {
        _animInstance->driftDirection = (_vehicle->_driftDirection > 0.f) ? 1.f : -1.f;
    }
    else
    {
        _animInstance->driftDirection = 0.f;
    }
}

void UArcadeVSAnimator::UpdateTiltAndRoll()
{
	// Compute the speed difference
	float speedDiffRatio = 1.f - (FMath::Abs((float)_vehicle->Speed) / (float)_vehicle->_maxSpeed);

	// Tilt
	if (_resetTilt && (FMath::Abs(_animInstance->tiltAngle) < 0.1f))  // Try to get the highest value so that the transition is as fast as possible but without having "jumps"
	{
		// Debug
		// UE_LOG(LogTemp, Warning, TEXT("resetTilt - %s - SpeedRatio: %.2f"), _isVehicleAccelerating ? TEXT("Accelerating") : TEXT("Braking"), speedDiffRatio);
		_resetTilt = false;
		_elapsedTimeSinceReset = .0f;

		_maxTiltAngle = _isVehicleAccelerating ? _accelerationTiltMaxAngle : _brakingTiltMaxAngle;
		_tiltOscSpeed = _isVehicleAccelerating ? _accelerationTiltOscSpeed : _brakingTiltOscSpeed;
		_tiltDamping = _isVehicleAccelerating ? _accelerationTiltDamping : _brakingTiltDamping;

		_tiltOscSpeed *= speedDiffRatio;
		_tiltOscSpeed = FMath::Clamp(_tiltOscSpeed, 5.f, 10.f); // Hardcoded, change the minimal value so that we still got the effect

		if (_isVehicleAccelerating)
		{
			_axisScale = _vehicle->_accelerationScale;
		}
		else if (_isVehicleBraking)
		{
			_axisScale = _vehicle->_brakingScale;
		}

		// Trigger Axis support (acceleration and braking scales set in the Controller)
		// Not perfect but will at least scale the animation so that it feels in sync with the axis pressure 
		_maxTiltAngle *= _axisScale;
	}

	_elapsedTimeSinceReset += _vehicle->_deltaTime;

	// Damped sine wave equation, check: https://www.geogebra.org/m/zXU3HUpm
	// f(t) = A * e^(-c*t) * sin(w*t)
	// A = Max Tilt Angle
	// c = Tilt Damping
	// w = Oscillation speed
	// t = Elapsed time_scale
	float tiltAngleValue = _maxTiltAngle * FMath::Exp(-1.f * _tiltDamping * _elapsedTimeSinceReset) * FMath::Sin(_tiltOscSpeed * _elapsedTimeSinceReset);

	if(_scaleTiltWithSpeed)
		tiltAngleValue *= speedDiffRatio;

	_animInstance->tiltAngle = tiltAngleValue;

	// Debug
	// UE_LOG(LogTemp, Warning, TEXT("maxTiltAngle:%.2f - OscillationSpeed:%.2f - Damping:%.2f"), _maxTiltAngle, _tiltOscSpeed, _tiltDamping);

	// Roll
	float wheelSlip = (speedToKmH(_vehicle->_linearVelocityLocal.Y) / _vehicle->_maxSpeed);
	wheelSlip = FMath::Clamp(wheelSlip, -1.f, 1.f);
	float rollAngleValue = _maxRollAngle * wheelSlip;

	_animInstance->rollAngle = FMath::Clamp(rollAngleValue, -_maxRollAngle, _maxRollAngle) * _rollScale;
}