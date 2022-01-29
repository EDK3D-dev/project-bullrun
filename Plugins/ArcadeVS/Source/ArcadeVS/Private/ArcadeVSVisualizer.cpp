/* Copyright (C) PoyoWorks 2019, Inc - All Rights Reserved
*
* ArcadeVS - Arcade Vehicle System
* poyoworks@gmail.com
*
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
*
*/


#include "ArcadeVSVisualizer.h"
#include "ArcadeVSVehicle.h"

#include <Components/LineBatchComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include <Components/ArrowComponent.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <DrawDebugHelpers.h>



static const FColor Color_LinearVelocity        = FColor::Yellow;
static const FColor Color_Acceleration          = FColor::Green;
static const FColor Color_Adherence             = FColor::Purple;



// Sets default values for this component's properties
UArcadeVSVisualizer::UArcadeVSVisualizer()
    : UActorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;

    // Default values
    _enabled = true;
    _useDebugMaterial = true;
    _lifeTime = .016f; // 1 Frame at 60fps 
    _forceScalingFactor = 0.002f;
    _arrowLength = 200.f;
    _arrowThickness = 3.f;
    _suspensionLineWidth = 1.f;
    _drawCenterOfMass = true;
    _drawTractionOffset = true;
    _drawSuspensionRayCasts = true;
    _drawLinearVelocity = true;
    _drawAcceleration = true;
    _drawAdherence = true;

    _rayCastImpactSphereRadius = 4.0;
}


// Called when the game starts
void UArcadeVSVisualizer::BeginPlay()
{
    Super::BeginPlay();

    _vehicle = Cast<AArcadeVSVehicle>(GetOwner());
    if (!_vehicle)
    {
        UE_LOG(LogTemp, Error, TEXT("This component can only be added to an Actor inheriting ArcadeVSVehicle Actor"));
        return;
    }

    _rootCpt = Cast<UPrimitiveComponent>(_vehicle->GetRootComponent());
    if (_useDebugMaterial)
    {
        if (!IsValid(_debugMaterial))
        {
            UE_LOG(LogTemp, Error, TEXT("Set to use debug material but no debug material specified"));
            return;
        }
        else
        {
            for (int i = 0; i<_rootCpt->GetNumMaterials(); ++i)
                _rootCpt->SetMaterial(i, _debugMaterial);
        }
    }
}

// Called every frame
void UArcadeVSVisualizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!_vehicle || !_rootCpt || !_enabled)
        return;

    // Draw debug infos
    static float deltaAccum = 0.0;
    static int speed = 0.0;
    deltaAccum += DeltaTime;

    _transform = _vehicle->GetTransform();

    // GetCenterOfMass returns the center of mass in World Space
    _com = _rootCpt->GetCenterOfMass();

    if (_drawCenterOfMass)
        DrawDebugSphere(GetWorld(), _com, 8, 16, FColor(255, 0, 0), false, _lifeTime, SDPG_MAX);

    if (_drawTractionOffset)
    {
        DrawDebugSphere(GetWorld(), _com + _vehicle->_tractionOffsetInWorldSpace, 8, 16, FColor(100, 255, 0), false, _lifeTime, SDPG_MAX);
    }

    DrawLinearVelocity();
    DrawAcceleration();
    DrawSuspensionRayCasts();
    DrawAdherence();
}

void UArcadeVSVisualizer::DrawAcceleration()
{
    if (!_drawAcceleration)
        return;

    DrawDebugDirectionalArrow(GetWorld(), _com, _com + (_vehicle->_accelerationForce * _forceScalingFactor),
        _arrowLength, Color_Acceleration, false, _lifeTime, 0.0, _arrowThickness);
}

void UArcadeVSVisualizer::DrawLinearVelocity()
{
    if (!_drawLinearVelocity)
        return;

    FVector vel = _vehicle->_linearVelocityWorld * _vehicle->_mass;
    DrawDebugDirectionalArrow(GetWorld(), _com, _com + (vel * _forceScalingFactor)
        , _arrowLength, Color_LinearVelocity, false, _lifeTime, 0.0, _arrowThickness);
}

void UArcadeVSVisualizer::DrawAdherence()
{
    if (!_drawAdherence)
        return;

    DrawDebugDirectionalArrow(GetWorld(), _com, _com + (_vehicle->_adherenceForce * _forceScalingFactor)
        , _arrowLength, Color_Adherence, false, _lifeTime, 0.0, _arrowThickness);
}

void UArcadeVSVisualizer::DrawSuspensionRayCasts()
{
    if (!_drawSuspensionRayCasts)
        return;

    float susCompRatio = 0.0;
    FColor color(FColor::White);

    FVector rayDir = _rootCpt->GetUpVector() * _vehicle->_suspensionRayCastLength * -1.0;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(GetOwner());

    FHitResult hr[4];
    FVector location;
    for (int i = 0; i < _vehicle->_suspensionPts.Num(); ++i)
    {
        location = _vehicle->GetTransform().TransformPositionNoScale(_vehicle->_suspensionPts[i]);
        GetWorld()->LineTraceSingleByObjectType(hr[i], location, location + rayDir, FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic)), CollisionParams);

        susCompRatio = 1.f - _vehicle->_suspensionHitResults[i].Distance / _vehicle->_suspensionRayCastLength;
        color = FColor((uint8)(susCompRatio * 255), (uint8)(1.f - susCompRatio * 255), 0.0);
        FColor impactNormalColor(255, 255, 0);
        FColor normalColor(0, 255, 255);
        if (hr[i].GetActor() != nullptr)
        {
            DrawDebugLine(GetWorld(), hr[i].TraceStart, hr[i].ImpactPoint, color, false, _lifeTime, 0.0, _suspensionLineWidth);
            DrawDebugLine(GetWorld(), hr[i].TraceStart, hr[i].TraceStart - hr[i].Normal * 60.f, normalColor, false, _lifeTime, 0.0, _suspensionLineWidth);
            DrawDebugLine(GetWorld(), hr[i].TraceEnd, hr[i].TraceEnd + hr[i].ImpactNormal * 60.f, impactNormalColor, false, _lifeTime, 0.0, _suspensionLineWidth);
            DrawDebugSphere(GetWorld(), hr[i].ImpactPoint, _rayCastImpactSphereRadius, 16, color, false);
        }
        else
        {
            // No hit
            DrawDebugLine(GetWorld(), hr[i].TraceStart, hr[i].TraceEnd, FColor::White, false, _lifeTime, 0.0, _suspensionLineWidth);
        }
    }
}