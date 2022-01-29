/* Copyright (C) PoyoWorks 2019, Inc - All Rights Reserved
*
* ArcadeVS - Arcade Vehicle System
* poyoworks@gmail.com
*
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
*
*/


#include "ArcadeVSVehicle.h"
#include "ArcadeVSBaseAnimInstance.h"

#include <Engine/Engine.h>
#include <Engine/Canvas.h>
#include <Components/BoxComponent.h>
#include <Components/StaticMeshComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include <Math/UnrealMathUtility.h>

#include <algorithm>


//----------------------------------------------------------------------------------------------
//  Constructor
//
//  * Sets all the original default values
//  * Will be overriden in Blueprints inheriting this class
//----------------------------------------------------------------------------------------------
AArcadeVSVehicle::AArcadeVSVehicle()
    : APawn()
	, _drifting(false)
	, _jumping(false)
    , _rotationAngle(0.f)
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    VehicleSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Mesh"));
    SetRootComponent(VehicleSkeletalMesh);
    _root = Cast<UPrimitiveComponent>(VehicleSkeletalMesh);

    // Wheels visual direction change handling
    _wheelsMaxDirectionAngle = 35;
    _wheelsRotationToNeutralDuration = .5f;
    _wheelsDirectionTimer = 0.f;
    _wheelsDirectionChangeDuration = 0.f;
    _wheelsTargetDirection = 0.f;
    _wheelsPreviousDirection = 0.f;
    _wheelsDirection = 0.f;

    // Power
    _maxSpeed = 160.f;
    _acceleration = 4000.f;
    _brake = 2000.f;
    _accelerateBrakeOffset = FVector(0.f, 0.f, 0.f);
    _mass = 100;
    _gravityScale = 3.f;
    _delayBeforeReverse = .4f;
	_maxReverseSpeed = 80.f;
    _nbWheelsForOnGround = 1;

    // Turning
    _maxTurningAngleAtMinSpeed = 50.f;
    _maxTurningAngleAtFullSpeed = 40.f;
    _turningForce = 300.f;
    _adherenceForceVerticalOffset = -2.f;

    // Suspension
    _suspensionRayCastLength = 110.f;
    _targetSuspensionHeight = 80.f;
    _suspensionDamping = 300.f;
    _minMaxSuspensionOffsets = FVector2D(-2.f, 10.f);
    _adherence = 15000.f;
    _linearDamping = 0.2f;
    _angularDamping = 3.f;
    _comOffset = FVector(0.f, 0.f, -0.5f);

    // Drift
    _driftMinSpeed = 60.f;
    _driftMinTurningScale = .9f;
    _driftDelay = 0.f;
    _driftTurningAngleBoost = 35.f;
    _driftAccelerationCompensation = .8f;
	_driftRecoverDuration = 1.f;
	_driftTurningScaleOffset = .3f;
    _driftAdherenceNerf = 25.f;
	_driftRecoverTimer = -1.f;

    // Jump
    _jumpEnabled = true;
    _jump = 1000.f;
	_disableLinearDampingInAir = false;
    _stabilizeInAir = true;
    _stabilizationSpeed = 50.f;
    _jumpSuspensionDelay = .2f;

    // Data
    _driftDirection = 0.f;
    _driftTimer = -1.f;
    _canDrift = false;
    _drifting = false;

    _nbWheelsOnGround = 0;
    _onGround = true;
    _onReverse = false;

    // Inputs
    _accelerating = false;
    _braking = false;
    _drifting = false;

    _previousSpeedInCms = -1.f;
}

//----------------------------------------------------------------------------------------------
//  Utility functions
//----------------------------------------------------------------------------------------------
float speedToKmH(float speed)
{
    // / 100 -> m/s
    // / 1000 -> km/s
    // * 3600 -> km/h
    return speed * 0.036;
}

float speedToCmS(float speed)
{
    return speed * 27.7;
}


//----------------------------------------------------------------------------------------------
//  APawn overrides
//----------------------------------------------------------------------------------------------
void AArcadeVSVehicle::BeginPlay()
{
    Super::BeginPlay();

    RegisterWheelBones();

    // Make sure that the Arcade vehicle is ok
    if (!IsValid())
        return;

    VehicleSkeletalMesh->SetSimulatePhysics(true);
    VehicleSkeletalMesh->SetEnableGravity(false);

    // Ignore mass set in the Physics material, use the mass set in the BP
    FBodyInstance* BodyInstance = VehicleSkeletalMesh->GetBodyInstance(VehicleSkeletalMesh->GetBoneName(0));
    FBodyInstance* test = VehicleSkeletalMesh->GetBodyInstance();
    if (!BodyInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("ArcadeVS BodyInstance found on SkeletalMesh is null"));
    }
    else
    {
        BodyInstance->bOverrideMass = true;
        BodyInstance->SetMassOverride(_mass);
    }

    _maxTurningAngle = _maxTurningAngleAtMinSpeed;
}

//----------------------------------------------------------------------------------------------
//  Tick
//
//  Whole logic goes here, looks at the movement flags set by the controller and updates
//  the Vehicle Physics model
//
//  * ---------- gravity and state phase
//  * 1. Apply gravity based on gravity scale setting
//  * 2. Calculate speed and direction vector
//  * 3. Calculate Max turning angle based on current speed, drift and settings
//  * 4. Calculate suspension by shooting raycasts at each registered wheel bone location
//  * 5. Determine if we're on the ground or airbone based on the raycast results
//  * 6. Check if we're stopped
//  * 7. Check if we're jumping, if that's the case deactivate suspension based on settings
//
//  * ---------- physics phase
//  * 1. Apply suspension forces based on raycast results and targetSuspensionHeight. Keep in mind that the suspension
//  *    behaves like a constraint instead of a simple spring so it will try to enforce this distance by bringing the vehicle to the ground
//  * 2. Apply adherence force, applied alongside the right vector of the vehicle based on the current linear velocity and the settings.
//  * 3. Brake or Accelerate
//  * 4. Apply drift, please check other functions to see all the impacts of the drift mechanics (turning update, speed etc...)
//  * 5. Turn, based on the current speed ratio and the min/max turning angles settings
//  * 6. Stabilize if in Air and stabilization enabled
//  * 7. Update properties of the animation blueprint (wheel direction/rotation and drift if needed)
//
//----------------------------------------------------------------------------------------------
void AArcadeVSVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    _deltaTime = DeltaTime;
	_driftRecoverTimer += _deltaTime;

    _accelerationForce = FVector::ZeroVector;

    if (!IsValid())
        return;

    VehicleSkeletalMesh->SetLinearDamping(_linearDamping);
    VehicleSkeletalMesh->SetAngularDamping(_angularDamping);
    VehicleSkeletalMesh->SetCenterOfMass(_comOffset);

    ApplyGravity();
    CalculateSpeedAndDirection();
    CalculateMaxTurningAngle();
    ShootSuspensionRayCasts();
    GroundCheck();

    if (_linearVelocityLocal.X < 1)
    {
        if (!_stopped)
        {
            OnStop();
            _stopped = true;
        }
    }
    else
    {
        _stopped = false;
    }

    bool applySuspension = true;
    if (_jumping)
    {
        _jumpSuspensionTimer += _deltaTime;
        if (_jumpSuspensionTimer < _jumpSuspensionDelay)
        {
            applySuspension = false;
        }
        else if (_onGround)
        {
            _jumping = false;
        }
    }

    if (applySuspension)
        ApplySuspension();

    ComputeSuspensionOffsets();
    ApplyAdherence();

    _tractionOffsetInWorldSpace = GetActorRotation().RotateVector(_accelerateBrakeOffset);
    if (_onGround)
    {
        if (_braking)
        {
            SetDrift(false);
            if (_reverseDelayTimer >= 0.0)
                _reverseDelayTimer += DeltaTime;

            Brake();
        }
        else if (_accelerating)
        {
            Accelerate();
        }
        else
        {
            SetDrift(false);
        }

        Drift();
        Turn();
    }
    else
    {
		if(_disableLinearDampingInAir)
			VehicleSkeletalMesh->SetLinearDamping(0.f);

        SetDrift(false);
        if (_stabilizeInAir)
            Stabilize();
    }
}

//----------------------------------------------------------------------------------------------
//  Movement flags
//
//  * Functions called by the ArcadeVehicleController
//----------------------------------------------------------------------------------------------
void AArcadeVSVehicle::SetAccelerate(float scale)
{
	_accelerationScale = scale;
	bool value = _accelerationScale > 0.05f;
    
	if (_accelerating == value)
        return;

    _accelerating = value;
    _accelerating ? OnStartAccelerate() : OnStopAccelerate();

    if (_accelerating)
        SetBrake(false);
}
void AArcadeVSVehicle::SetBrake(float scale)
{
	_brakingScale = scale;
	bool value = _brakingScale > 0.05f;

    if (_braking == value)
        return;

    _braking = value;
    _braking ? OnStartBrake() : OnStopBrake();

    if (_braking)
        SetAccelerate(false);
}
void AArcadeVSVehicle::SetDrift(bool value)
{
    if (_drifting == value)
        return;

    _drifting = value && _canDrift;

    // Set drift direction to be able to properly drive the animation blueprint later
    if (_drifting)
    {
        _driftDirection = (_turningScale > 0.f) ? 1.f : -1.f;
		_driftRecoverTimer = -1.f;
    }
    else
    {
        _driftDirection = 0.f;
		_driftRecoverTimer = 0.f;
    }

    _drifting ? OnStartDrift() : OnStopDrift();
}
void AArcadeVSVehicle::SetTurn(float scale)
{
    _turningScale = scale;

    if (_drifting)
    {
        // Drift direction -1 = left, 1 = right
        if (_driftDirection == -1.f)
        {
            _turningScale -= _driftTurningScaleOffset;
            _turningScale = FMath::Clamp(_turningScale, -1.f, 0.f);
        }
        else
        {
            _turningScale += _driftTurningScaleOffset;
            _turningScale = FMath::Clamp(_turningScale, 0.f, 1.f);
        }
    }

    float newTargetDirection = _turningScale * _wheelsMaxDirectionAngle;
    if (newTargetDirection != _wheelsTargetDirection)
    {
        _wheelsDirectionTimer = 0.f;
        _wheelsTargetDirection = newTargetDirection;
        _wheelsPreviousDirection = _wheelsDirection;
        _wheelsDirectionChangeDuration = _wheelsRotationToNeutralDuration * (_wheelsTargetDirection - _wheelsPreviousDirection) / (_wheelsMaxDirectionAngle);
        _wheelsDirectionChangeDuration = FMath::Abs(_wheelsDirectionChangeDuration);
    }
}
void AArcadeVSVehicle::ApplyDriftTurnChanges(float scale)
{
    // Drift direction -1 = left, 1 = right
    if (_driftDirection == -1.f)
    {
        _turningScale -= _driftTurningScaleOffset;
        _turningScale = FMath::Clamp(_turningScale, -1.f, 0.f);
    }
    else
    {
        _turningScale += _driftTurningScaleOffset;
        _turningScale = FMath::Clamp(_turningScale, 0.f, 1.f);
    }
}

//----------------------------------------------------------------------------------------------
//  Internal stuff
//----------------------------------------------------------------------------------------------
bool AArcadeVSVehicle::IsValid()
{
    if (!VehicleSkeletalMesh->SkeletalMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("ArcadeVS Error: no skeletal mesh assigned"));
        return false;
    }

    _animInstance = Cast<UArcadeVSBaseAnimInstance>(VehicleSkeletalMesh->GetAnimInstance());

    return true;
}
void AArcadeVSVehicle::RegisterWheelBones()
{
    int32 numBones = VehicleSkeletalMesh->GetNumBones();
    for (auto& name : _suspensionPointsBones)
    {
        int32 boneIndex = VehicleSkeletalMesh->GetBoneIndex(name);
        if (boneIndex == INDEX_NONE)
        {
            UE_LOG(LogTemp, Error, TEXT("ArcadeVS Error: bone name %s not found!"), *name.ToString());
        }
        else
        {
            FVector location = VehicleSkeletalMesh->GetBoneLocation(name, EBoneSpaces::ComponentSpace);
            _suspensionPts.Add(location);
        }
    }
}

//----------------------------------------------------------------------------------------------
//  State update functions
//----------------------------------------------------------------------------------------------
void AArcadeVSVehicle::CalculateSpeedAndDirection()
{
    FTransform worldTransform(GetTransform());
    FTransform worldToLocal = worldTransform.Inverse();

    _linearVelocityWorld = _root->GetPhysicsLinearVelocity();
    _linearVelocityLocal = worldToLocal.TransformVector(_linearVelocityWorld);
    _angularVelocityLocal = worldToLocal.TransformVector(_root->GetPhysicsAngularVelocityInDegrees());
    _speedInCms = _linearVelocityLocal.X;

    Speed = speedToKmH(_linearVelocityLocal.X);
    SpeedPercent = FMath::Abs((int)((float)Speed / (float)_maxSpeed * 100));

    _onReverse = Speed < 0.f;
    Speed *= _onReverse ? -1.0 : 1.0;

    _accDir = _root->GetForwardVector();
}
void AArcadeVSVehicle::CalculateMaxTurningAngle()
{
    float angleDiff = _maxTurningAngleAtMinSpeed - _maxTurningAngleAtFullSpeed;
    _maxTurningAngle = (1.f - (Speed / _maxSpeed)) * angleDiff + _maxTurningAngleAtFullSpeed;

    if (_drifting)
        _maxTurningAngle += _driftTurningAngleBoost;
}
void AArcadeVSVehicle::ShootSuspensionRayCasts()
{
    FVector rayDir = _root->GetUpVector() * _suspensionRayCastLength * -1.0;

    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    FTransform worldTransform(GetTransform());

    FVector location;
    FHitResult hitResult;
    _suspensionHitResults.Empty();
    for (int i = 0; i != _suspensionPts.Num(); ++i)
    {
        location = worldTransform.TransformPositionNoScale(_suspensionPts[i]);
        GetWorld()->LineTraceSingleByObjectType(hitResult, location, location + rayDir, FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic)), CollisionParams);
        _suspensionHitResults.Add(hitResult);
    }
}
void AArcadeVSVehicle::GroundCheck()
{
    bool onGround = true;
    _nbWheelsOnGround = 0;

    // Are we on ground?
    for (int i = 0; i < _suspensionPts.Num(); ++i)
    {
        if (_suspensionHitResults[i].GetActor() != nullptr)
            _nbWheelsOnGround++;
    }

    // TODO Add this as a property!
    onGround = _nbWheelsOnGround >= _nbWheelsForOnGround;
    if (!_onGround && onGround)
    {
        // Back on the ground!
        OnOffGroundEnd();
    }
    else if (_onGround && !onGround)
    {
        // Jumpiiiiinnnng
        OnOffGroundStart();
    }
    _onGround = onGround;
}
void AArcadeVSVehicle::UpdateAccelerationDirection()
{
    // Update Acceleration Vector
    _accDir = _root->GetForwardVector();
}

//----------------------------------------------------------------------------------------------
//  Physics update functions
//----------------------------------------------------------------------------------------------
void AArcadeVSVehicle::ApplyGravity()
{
    _gravityForce = FVector(0.f, 0.f, GetWorld()->GetGravityZ() * _gravityScale * _mass);
    _root->AddForceAtLocation(_gravityForce, _root->GetCenterOfMass());
}
void AArcadeVSVehicle::ApplySuspension()
{
    float suspensionSpringForce = -_gravityForce.Z;

    // Dampening factor, gives good results, keep hardcoded for now
    suspensionSpringForce /= 20.f;

    _suspensionForces.Empty();
    for (int i = 0; i < _suspensionPts.Num(); ++i)
    {
        if (_suspensionHitResults[i].GetActor() != nullptr)
        {
            // Check https://gafferongames.com/post/spring_physics/ it's great
            // F = -k(|x|-d)(x/|x|) - bv
            // |x|   : current distance between the points -> _hitResults[i].Distance
            //  d    : desired distance -> _suspensionHeight
            // x/|x| : unit direction length between the two points -> impactNormal
            // bv    : dampening factor * velocity

            FVector velocityAtPt = _root->GetPhysicsLinearVelocityAtPoint(_suspensionHitResults[i].TraceStart);
            float relVelocity = FVector::DotProduct(velocityAtPt, _suspensionHitResults[i].ImpactNormal);

            float suspensionForce = -(suspensionSpringForce * (_suspensionHitResults[i].Distance - _targetSuspensionHeight)) - (_suspensionDamping * relVelocity);                        
            FVector force = suspensionForce * _root->GetUpVector();
            _root->AddForceAtLocation(force, FVector(_suspensionHitResults[i].TraceStart));
            
            _suspensionForces.Add(force);            
        }
    }
}

void AArcadeVSVehicle::ComputeSuspensionOffsets()
{
    _suspensionOffsets.Empty();

    for (int i = 0; i < _suspensionPts.Num(); ++i)
    {
        if (_suspensionHitResults[i].GetActor() != nullptr)
        {
            float wheelOffset = FMath::Clamp(-1.f * (_suspensionHitResults[i].Distance - _targetSuspensionHeight), _minMaxSuspensionOffsets[0], _minMaxSuspensionOffsets[1]);
            _suspensionOffsets.Add(FVector(0.f, 0.f, wheelOffset));
        }
        else
        {
            // This wheel is not touching ground, simulate a fully extended suspension
            _suspensionOffsets.Add(FVector(0.f, 0.f, _minMaxSuspensionOffsets[0]));
        }
    }
}

void AArcadeVSVehicle::ApplyAdherence()
{
    if (!_onGround)
    {
        _adherenceForce = FVector::ZeroVector;
        return;
    }

    float speedRatio = -(speedToKmH(_linearVelocityLocal.Y) / _maxSpeed);
    float force = (speedRatio * _mass * _adherence);

    if (_drifting)
    {
        // Reduce adherence force by the amount specified in the settings
        force -= force * (_driftAdherenceNerf / 100.f);
    }
	else
	{
		float driftRecoverRatio = FMath::Clamp((_driftRecoverTimer / _driftRecoverDuration), 0.f, 1.f);
		force = force * driftRecoverRatio;
	}

    _adherenceForce = force * _root->GetRightVector();
    
    FVector forceLocation(_root->GetCenterOfMass());
    forceLocation += _root->GetUpVector() * _adherenceForceVerticalOffset;
    
    _root->AddForceAtLocation(_adherenceForce, forceLocation);
}

void AArcadeVSVehicle::Stabilize()
{
    FRotator rotation(GetActorRotation());
    FVector eulerAngles = rotation.Euler();

    float force = (_turningForce * FMath::DegreesToRadians(eulerAngles.Y) * _stabilizationSpeed) - _angularVelocityLocal.Y * 50.f;
    _root->AddTorqueInDegrees(_root->GetRightVector() * force, VehicleSkeletalMesh->GetBoneName(0), true);

    force = (_turningForce * FMath::DegreesToRadians(eulerAngles.X) * 10.f) - _angularVelocityLocal.X * 50.f;
    _root->AddTorqueInDegrees(_root->GetForwardVector() * force, VehicleSkeletalMesh->GetBoneName(0), true);
}

void AArcadeVSVehicle::Accelerate()
{
    _reverseDelayTimer = -1.0;

    if (Speed >= _maxSpeed)
        return;

    // F = M * a
    float speedRatio = 1 - (Speed / _maxSpeed);
    FMath::Pow(speedRatio, 10.f); // Harcoded curve to avoid having something too linear, will be moved to a Curve parameter
    
	float force = _mass * _acceleration * speedRatio;
	_accelerationForce = _accDir * force * _accelerationScale;

    if (!_drifting)
        _root->AddForceAtLocation(_accelerationForce, _root->GetCenterOfMass() + _tractionOffsetInWorldSpace);
    else
        _root->AddForceAtLocation(_accelerationForce * _driftAccelerationCompensation, _root->GetCenterOfMass() + _tractionOffsetInWorldSpace);
}
void AArcadeVSVehicle::Brake()
{
    if (Speed <= 0.0)
    {
        if (_reverseDelayTimer == -1.0)
        {
            _reverseDelayTimer = 0.0;
        }
        else
        {
            if (_reverseDelayTimer >= _delayBeforeReverse)
            {
                // go in reverse
				if(FMath::Abs(Speed) >= _maxReverseSpeed)
					return;

                float force = _mass * _brake;
                _root->AddForceAtLocation(-_accDir * force, _root->GetCenterOfMass() + _tractionOffsetInWorldSpace);
            }
        }
        return;
    }

    float force = _mass * _brake;
	force *= _brakingScale;

    _root->AddForceAtLocation(-_accDir * force, _root->GetCenterOfMass() + _tractionOffsetInWorldSpace);
}
void AArcadeVSVehicle::Drift()
{
    bool newCanDrift = _canDrift;
    if (!_drifting)
    {
        // Reset timer if not already drifting and not turning to the minimum specified
        if (SpeedPercent >= _driftMinSpeed)
        {
            // Speed ok but not turning enough
            if (FMath::Abs(_turningScale) < _driftMinTurningScale)
            {
                _driftTimer = -1.f;
                newCanDrift = false;
            }
            else if (FMath::Abs(_turningScale) >= _driftMinTurningScale)
            {
                // If the timer is already started, increase the time, otherwise start it!
                if (_driftTimer != -1.f)
                    _driftTimer += _deltaTime;
                else
                    _driftTimer = 0.f;
            }

            if (!_drifting && (_driftTimer >= _driftDelay))
            {
                newCanDrift = true;
            }
        }
        else
        {
            // Too slow!
            _driftTimer = -1.f;
            newCanDrift = false;
        }

        if (!_canDrift && newCanDrift)
            OnCanDriftStart();
        else if (_canDrift && !newCanDrift)
            OnCanDriftEnd();

        _canDrift = newCanDrift;
    }
}

void AArcadeVSVehicle::Turn()
{
    _turning = false;

    // Can't turn if you're (almost) not moving
    if (FMath::Abs(Speed) < 5.f)
        return;

    // Regular torque turn. Real turning is also due to the adherence force. Without it the 
    // vehicle simply turns around it's center of mass Z-Axis while keeping the same direction
    FVector angularVelocity = _root->GetPhysicsAngularVelocityInDegrees();

    if (FMath::Abs(angularVelocity.Z) <= _maxTurningAngle && FMath::Abs(_turningScale) > 0.f)
    {
        _turningScale *= _onReverse ? -1.0 : 1.0;
        _root->AddTorqueInDegrees(_root->GetUpVector() * _turningForce * _turningScale, VehicleSkeletalMesh->GetBoneName(0), true);
        _turning = true;
    }
}
void AArcadeVSVehicle::Jump()
{
    if (_jumpEnabled)
    {
        if (!_jumping)
        {
            _root->AddImpulseAtLocation(_root->GetUpVector() * _jump * _mass, _root->GetCenterOfMass());
            _jumping = true;
            _jumpSuspensionTimer = 0.f;
        }
    }
}
void AArcadeVSVehicle::Stop()
{
    // Stop all current action, the Controller input may have been disabled 
    // so we don't want actions/axis bindings registered to stay stuck on their current values
    SetDrift(false);
    SetAccelerate(false);
    SetBrake(false);

    _turningScale = 0.f;
}


void AArcadeVSVehicle::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
    Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

    UFont* Font = GEngine->GetSmallFont();

    FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
    DisplayDebugManager.SetFont(Font);

    DisplayDebugManager.SetDrawColor(FColor(255, 255, 0));
    DisplayDebugManager.DrawString("----------- [ Arcade Vehicle ] -----------");
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Speed:%d (%d%) / Drifting:%s / Can Drift:%s"), Speed, SpeedPercent, _drifting ? TEXT("YES") : TEXT("NO"), _canDrift ? TEXT("YES") : TEXT("NO")));
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Turning:%s / Max Turning Angle:%.1f / Turning Scale:%.1f"), _turning ? TEXT("YES") : TEXT("NO"), _maxTurningAngle, _turningScale));
    DisplayDebugManager.DrawString(FString::Printf(TEXT("On Ground:%s (%d wheels)"), _onGround ? TEXT("YES") : TEXT("NO"), _nbWheelsOnGround));

    for (int i = 0; i<_suspensionForces.Num(); ++i)
        DisplayDebugManager.DrawString(FString::Printf(TEXT("Suspension Force %d : [%.1f / %.1f / %.1f]"), i, _suspensionForces[i].X, _suspensionForces[i].Y, _suspensionForces[i].Z));

    DisplayDebugManager.SetDrawColor(FColor(255, 255, 20));
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Local Linear Velocity (m/s):  [%.1f, %.1f, %.1f]"), _linearVelocityLocal.X / 100.f, _linearVelocityLocal.Y / 100.f, _linearVelocityLocal.Z / 100.f));
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Local Angular Velocity (deg): [%.1f, %.1f, %.1f]"), _angularVelocityLocal.X, _angularVelocityLocal.Y, _angularVelocityLocal.Z));

    FRotator rotation(GetActorRotation());
    FVector eulerAngles = rotation.Euler();
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Rotation Angles:              [%.1f, %.1f, %.1f]"), eulerAngles.X, eulerAngles.Y, eulerAngles.Z));
    DisplayDebugManager.DrawString(FString::Printf(TEXT("Adherence Force:              [%.1f, %.1f, %.1f]"), _adherenceForce.X, _adherenceForce.Y, _adherenceForce.Z));

    if (_animInstance)
    {
        DisplayDebugManager.DrawString(FString::Printf(TEXT("Last Frame Traveled Distance      : %.1f"), _lastFrameTraveledDistance));
        DisplayDebugManager.DrawString(FString::Printf(TEXT("Anim Wheel Rotation      : %d"), _animInstance->wheelRotation));
        DisplayDebugManager.DrawString(FString::Printf(TEXT("Anim Wheel Direction     : %d"), _animInstance->wheelDirection));
    }
}