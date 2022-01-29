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
#include "GameFramework/Pawn.h"
#include "ArcadeVSVehicle.generated.h"


class UCanvas;
class UArcadeVSBaseAnimInstance;

/**
    Vehicle class, contains all the vehicle system logic.
    Please see the functions documentation and system flow in the .cpp comments.
*/
UCLASS()
class ARCADEVS_API AArcadeVSVehicle : public APawn
{
    GENERATED_BODY()

        friend class UArcadeVSAnimator;
        friend class UArcadeVSVisualizer;

public:

    virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

    // Blueprint implementable events
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStartAccelerate();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStopAccelerate();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStartBrake();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStopBrake();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStartDrift();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStopDrift();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnCanDriftStart();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnCanDriftEnd();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnJump();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnStop();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnOffGroundStart();
    UFUNCTION(BlueprintImplementableEvent, Category = "Arcade Vehicle")
        void OnOffGroundEnd();


    // Constructor, sets default values for this pawn's properties
    AArcadeVSVehicle();

    // APawn overrides
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;


    // Control functions - sets the movement flags, should be called by the PlayerController
    // We still keep them blueprintCallable to allow implementation of custom behavior / AI
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Control Override")
        virtual void SetAccelerate(float scale);
	UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Control Override")
		virtual void SetBrake(float scale);
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Control Override")
        virtual void SetDrift(bool value);
    virtual void ApplyDriftTurnChanges(float scale);
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Control Override")
        virtual void SetTurn(float scale);
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Control Override", meta = (Tooltip = "Reset all action/axis bindings, call this function before disabling input to ensure that no input gets stuck"))
        virtual void Stop();

    UFUNCTION(BlueprintCallable, Category = "ArcadeVS / Physics")
        FVector GetLocalLinearVelocity() const { return _linearVelocityLocal; };


    // Almost all functions are marked virtual to allow C++ override to keep the system super flexible

    // Movement processing and physics update (add force / torque)
    virtual void Accelerate();
    virtual void Brake();
    virtual void Drift();
    virtual void Turn();
    virtual void Jump();

    virtual bool IsValid();


protected:

    virtual void RegisterWheelBones();

    virtual void UpdateAccelerationDirection();
    virtual void CalculateMaxTurningAngle();
    virtual void CalculateSpeedAndDirection();
    virtual void ShootSuspensionRayCasts();
    virtual void GroundCheck();
    virtual void ApplyGravity();
    virtual void ApplySuspension();
    virtual void ComputeSuspensionOffsets();
    virtual void ApplyAdherence();
    virtual void Stabilize();


public:

    //
    // Properties
    //
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "Skeletal Mesh Component"))
        class USkeletalMeshComponent* VehicleSkeletalMesh;

    //
    // SUSPENSION Settings
    //

    /** The name of the bones that will give the positions from where the suspension ray casts will be shoot and the suspension
    forces will be applied. Those should usually be located at the perfect center of the wheels. Note that all those positions should have
    almost the same Z value otherwise the vehicle may not be stable when stopped */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Suspension Points Bone Names"))
        TArray<FName> _suspensionPointsBones;

    /** The height of the suspension. That's the distance of the suspension ray cast. You usually want to set this
    bigger than the wheel radius to get a feeling of the vehicle sticking to the road */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Ray Cast Length"))
        float _suspensionRayCastLength;

    /** Super important! This is the distance that the suspension system will try to enforce. This means that it's going
    to be the distance between your suspension point bone and the ground. On a regular car, this should be close to the radius of the wheel. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Target Suspension Height"))
        float _targetSuspensionHeight;

    /** Those parameters will be set on the Animation Blueprint to move the wheel bones along their Z axis to simulate suspension compression
    based on the current ray cast suspension height. Use those values to give a more realistic feel to the suspension.
    */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Min and Max Suspension Offsets"))
        FVector2D _minMaxSuspensionOffsets;

    /** Damping applied to the suspension force. The higher the value the fastest the suspension will come to a stable position.
    The lower the value, the more it will feel like a spring.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Suspension Damping"))
        float _suspensionDamping;

    /** Adherence force. This force will scale based on how much the vehicle is sliding and be applied alongside the vehicle's right vector.
    It's one of the most important setting to get right to get the feeling that the vehicle sticks to the road.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Adherence"))
        float _adherence;

    /** Offset applied to the physic body center of mass. Based on the wheels position, the precalculated center mass may not
    be properly position between the wheels, meaning that the vehicle will slightly move by default. Use this value to make
    it stable. Note that this is meters and not cm and its related to Physx!
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Suspension", meta = (DisplayName = "Center Of Mass Offset", ClampMin = "-5.0", ClampMax = "5.0", UIMin = "-5.0", UIMax = "5.0"))
        FVector _comOffset;


    //
    // POWER Settings
    //

    /** Physics Linear damping, this setting will override the value set in the Physics material */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Linear Damping"))
        float _linearDamping;

    /** Physics mass value, this setting will override the value set in the Physics material
    If you want to make the vehicle feel heavier, try to play with the gravity scale.
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Mass"))
        float _mass;

    /** Instead of changing the mass and having to re-tweak other settings, just adjust the gravity scale, it will give the feeling that the
    object is heavier*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Gravity Scale"))
        float _gravityScale;

    /** Maximum speed that the vehicle can reach, once reach acceleration is ignored */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Max Speed (km/h)"))
        int _maxSpeed;

    /** Acceleration force value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Acceleration"))
        float _acceleration;

    /** Brake force value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Brake"))
        float _brake;

    /** Offset in worldspace from the center of mass to where the acceleration and brake force will be applied */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Traction Offset"))
        FVector _accelerateBrakeOffset;

    /** When stopped, delay that the player have to keep the brake button pressed to start going in reverse */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Delay before going reverse (s)"))
        float _delayBeforeReverse;

	/** Maximum speed that the vehicle can reach while in reverse, once reach braking is ignored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Max Reverse Speed (km/h)"))
		float _maxReverseSpeed;

    /** This is the number of wheels that must be contacting with the ground (ie have their suspension raycast hitting an actor)
    for the vehicle to be considered "On Ground". Being "On ground" means that the vehicle can accelerate / brake and jump. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Power", meta = (DisplayName = "Ground Check Wheels Count"))
        int _nbWheelsForOnGround;

    //
    // TURNING Settings
    //

    /** Physics Angular damping, this setting will override the value set in the Physics material */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Angular Damping"))
        float _angularDamping;

    /** The maximum angle that the wheels can reach when turning.
    (This is visual only and will not affect gameplay. Gameplay turning is changed by adjusting the turningSpeed property) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Wheels Max Rotation Angle", ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "180"))
        int _wheelsMaxDirectionAngle;

    /** The time it will take the wheels to go back to neutral from their maximum turning angle
    (This is visual only and will not affect gameplay) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Wheels Rotation back to neutral duration"))
        float _wheelsRotationToNeutralDuration;

    /** How fast the car can turn when at min speed (maximum angular velocity in degrees/seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Max Angular Velocity At Min Speed"))
        float _maxTurningAngleAtMinSpeed;

    /** How fast the car can turn when at max speed (maximum angular velocity in degrees/seconds)
    You usually want this value to be smaller at max speed to give the feeling of an assisted direction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Max Angular Velocity at Full Speed"))
        float _maxTurningAngleAtFullSpeed;

    /** The Torque force */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Turning Force"))
        float _turningForce;

    /** World space (cm) offset applied to the location where the adherence force is applied. Use this to make the vehicle slightly roll
    when taking sharp turns */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Turning", meta = (DisplayName = "Adherence Force Vertical Offset"))
        float _adherenceForceVerticalOffset;


    //
    // DRIFT Settings 
    //

    /** Minimum speed (in percent) to be at to allow the player to start drifting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Min Speed (%)", ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
        float _driftMinSpeed;

    /** Minimum angle (in degrees) to be at to allow the player to start drifting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Min Turning Scale (0-1)", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
        float _driftMinTurningScale;

    /** Minimum time to be at the minimum speed AND angle before being able to drift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Start Delay", ClampMin = "0", ClampMax = "60", UIMin = "0", UIMax = "60"))
        float _driftDelay;

    /** This value will be added to the max turning angle value when drifting to give the feeling that you can take sharper turns */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Max Turning Angle Boost"))
        float _driftTurningAngleBoost;

    /** For infinite drifting scenarios, compensate for the loss of speed (due to the adherence nerf) to keep the player drifting indefinitely.
    This will act as a multiplactor on the acceleration force when drifting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Acceleration Compensation"))
        float _driftAccelerationCompensation;

	/** How long will it take for the vehicle to fully recover adherence after a drift */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Recover Duration"))
		float _driftRecoverDuration;

    /** For infinite drifting scenarios, to avoid having the player be able to go straight while drifting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Turning Scale Offset", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
        float _driftTurningScaleOffset;

    /** Loss of adherence when drifting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Drift", meta = (DisplayName = "Drift Adherence Nerf (%)", ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
        float _driftAdherenceNerf;


    //
    // JUMP Settings
    //

    /** Allows the player to jump */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Enable Jump"))
        bool _jumpEnabled;

    /** Jump force value (note thath Jump is applied as an Impulse and not a Force) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Jump Force"))
        float _jump;

	/** Will apply forces to bring the vehicle back to neutral position while airborne */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Disable Linear Damping In Air"))
		bool _disableLinearDampingInAir;

    /** Will apply forces to bring the vehicle back to neutral position while airborne */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Stabilize when Jumping"))
        bool _stabilizeInAir;

    /** Speed at which the car will go back to neutral position while airborne (only if stabilize in air is enabled) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Stabilization Speed", ClampMin = "10", ClampMax = "100", UIMin = "10", UIMax = "100"))
        float _stabilizationSpeed;

    /** To be able to jump, the suspension must be disabled shortly. This is the time that the suspension will stay disabled while jumping. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS / Jump", meta = (DisplayName = "Jump Suspension Delay", ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2"))
        float _jumpSuspensionDelay;


    //
    // State related properties
    // * Mostly here for debugging
    // * Readonly as all changes to the movement flags should be done through a Controller
    //
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        int Speed;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        int SpeedPercent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _accelerating;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _turning;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _braking;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _drifting;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _jumping;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _stopped;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        float _turningScale;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _onGround;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        bool _onReverse;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / State")
        float _wheelsDirection;
        

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Number of Wheels On Ground"))
        int _nbWheelsOnGround;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Linear Velocity in Local Space"))
        FVector _linearVelocityLocal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Angular Velocity in Local Space"))
        FVector _angularVelocityLocal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Suspension Points Locations"))
        TArray<FVector> _suspensionPts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Calculated Gravity Force"))
        FVector _gravityForce;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Suspensions Forces"))
        TArray<FVector> _suspensionForces;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Suspensions Hit Results"))
        TArray<FHitResult> _suspensionHitResults;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Suspension Offsets"))
        TArray<FVector> _suspensionOffsets;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcadeVS / Data", meta = (DisplayName = "Traveled Distance Last Frame (m)"))
        float _lastFrameTraveledDistance;


protected:

    UArcadeVSBaseAnimInstance* _animInstance;
    UPrimitiveComponent* _root;

    float _deltaTime;
    float _rotationAngle;
    float _maxTurningAngle;
    float _jumpSuspensionTimer;

    // Acceleration / Braking
    float _accelerationScale;
    float _brakingScale;

    // Wheels direction movement
    float _wheelsDirectionTimer;
    float _wheelsDirectionChangeDuration;
    float _wheelsTargetDirection;
    float _wheelsPreviousDirection;

    // Drift
    float _driftTimer;
    int _driftDirection;
    bool _canDrift;
	float _driftRecoverTimer;

    FVector _accDir;
    FVector _linearVelocityWorld;
    FVector _adherenceForce;
    FVector _accelerationForce;
    FVector _tractionOffsetInWorldSpace;

    float _speedInCms;
    float _previousSpeedInCms;
    float _reverseDelayTimer;
};
