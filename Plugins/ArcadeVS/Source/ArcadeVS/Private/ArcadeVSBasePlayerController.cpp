/* Copyright (C) PoyoWorks 2019, Inc - All Rights Reserved
*
* ArcadeVS - Arcade Vehicle System
* poyoworks@gmail.com
*
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
*
*/

#include "ArcadeVSBasePlayerController.h"
#include "ArcadeVSVehicle.h"

#include "Net/UnrealNetwork.h"



AArcadeVSBasePlayerController::AArcadeVSBasePlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , _vehicle(nullptr)
	, _isAccelerating(false)
	, _isMovementEnabled(true)
    , _accelerateAxis(0.f)
	, _brakeAxis(0.f)
{
}

void AArcadeVSBasePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArcadeVSBasePlayerController, _vehicle);
}

bool AArcadeVSBasePlayerController::IsVehicleValid()
{
	if (IsValid(GetPawn()))
	{
		_vehicle = Cast<AArcadeVSVehicle>(GetPawn());
		if (IsValid(_vehicle))
		{
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AArcadeVSBasePlayerController: Error, possessed pawn is not based on ArcadeVSVehicle"));
		}
	}

	_vehicle = nullptr;
	return false;
}

void AArcadeVSBasePlayerController::SetEnableMovement(bool enable)
{
    _isMovementEnabled = enable;
    if (_isMovementEnabled)
        Accelerate(_accelerateAxis);
}


//----------------------------------------------------------------------------------------------
//  Control functions
//
//  * Only set the control flags on the vehicle when called
//    all movement are then calculated and applied during the Vehicle's Tick() function
//----------------------------------------------------------------------------------------------

void AArcadeVSBasePlayerController::Accelerate(float axisValue)
{
    _accelerateAxis = axisValue;
    if (IsVehicleValid() && _isMovementEnabled)
        _vehicle->SetAccelerate(axisValue);
}

void AArcadeVSBasePlayerController::Brake(float axisValue)
{
    _brakeAxis = axisValue;
    if (IsVehicleValid() && _isMovementEnabled)
        _vehicle->SetBrake(axisValue);
}

void AArcadeVSBasePlayerController::Drift(bool drift)
{
    if (IsVehicleValid() && _isMovementEnabled)
        _vehicle->SetDrift(drift);
}

void AArcadeVSBasePlayerController::Turn(float scale)
{
    if (IsVehicleValid() && _isMovementEnabled)
        _vehicle->SetTurn(scale);
}

void AArcadeVSBasePlayerController::Jump()
{
    if (IsVehicleValid() && _isMovementEnabled)
        _vehicle->Jump();
}
