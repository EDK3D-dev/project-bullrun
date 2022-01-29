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
#include "GameFramework/PlayerController.h"
#include "ArcadeVSBasePlayerController.generated.h"


class AArcadeVSVehicle;

/**
 * Default Controller class for a ArcadeVSVehicle
 *
 * This class is just a C++ template that you can inherit from to start doing your own controller.
 * This class only provides the functions that you can call to control the vehicle but not the actual mappings. 
 *
 * To make the plugin easier to understand and tweak, the mappings are all done in BP_ArcadeVS_PlayerController
 */
UCLASS()
class ARCADEVS_API AArcadeVSBasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:

    AArcadeVSBasePlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray <class FLifetimeProperty>& OutLifetimeProps) const override;
	bool IsVehicleValid();

    /** Will make the car accelerate */
    UFUNCTION(BlueprintCallable, Category="ArcadeVS Controller")
    virtual void Accelerate(float axisValue);

    /** Will make the car brake */
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS Controller")
    virtual void Brake(float axisValue);

    /** Will make the car start drifting, this act as a toggle */
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS Controller")
    virtual void Drift(bool drift);

    /** Will make the car jump */
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS Controller")
    virtual void Jump();

    /** Will make the car turn */
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS Controller")
    virtual void Turn(float scale);

    /** Will enable or disable the movement on the car */
    UFUNCTION(BlueprintCallable, Category = "ArcadeVS Controller")
    void SetEnableMovement(bool enable);


protected:

    /** The ArcadeVSVehicle that is currently possessed by the controller */
    UPROPERTY(replicated, BlueprintReadOnly)
    AArcadeVSVehicle* _vehicle;

    /** Is the vehicle currently accelerating */
    UPROPERTY(BlueprintReadOnly)
    bool _isAccelerating;

    /** Is movement enabled */
    UPROPERTY(BlueprintReadOnly)
    bool _isMovementEnabled;

    /** Axis value when using a trigger to control acceleration */
    UPROPERTY(BlueprintReadOnly)
    float _accelerateAxis;

    /** Axis value when using a trigger to control braking */
    UPROPERTY(BlueprintReadOnly)
    float _brakeAxis;
};
