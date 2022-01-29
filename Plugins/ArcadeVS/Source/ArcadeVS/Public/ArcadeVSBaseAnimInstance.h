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
#include "Animation/AnimInstance.h"
#include "ArcadeVSBaseAnimInstance.generated.h"


/** This is a base class for ArcadeVSVehicle's animation blueprints to inherit from. 

    If assigned to the vehicle's skeletal mesh, the ArcadeVSVehicle will compute the wheel rotation
    and wheel direction values based on it's settings. 

    - Wheel rotation will use the actual vehicle speed and specified wheel radius setting.
    - Wheel direction will use the turning axis scale and the min/max angles specified
    - Drift is only used by ArcadeVS_Drift_VehicleAnim for BP_ArcadeVS_Kart to simulate old school racing game drifting animations.

    Please check ArcadeVS_Basic_VehicleAnim and ArcadeVS_Drift_VehicleAnim for the actual blueprint code applying those values.
 */
UCLASS()
class ARCADEVS_API UArcadeVSBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

    /** Rotation of the wheel, computed in ArcadeVSVehicle using the vehicle speed and specified wheel radius setting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        int wheelRotation;

    /** Direction of the front wheels, computed in ArcadeVSVehicle using the turning axis scale and the min/max angles specified */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        int wheelDirection;

    /** Offsets to be applied to the wheel bones to simulate the movement of the suspension */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        TArray<FVector> wheelsOffsets;

    /** Drift direction computed in ArcadeVSVehicle when starting a drift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        int driftDirection;

    /** Drift direction computed in ArcadeVSVehicle when starting a drift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        float tiltAngle;

    /** Drift direction computed in ArcadeVSVehicle when starting a drift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcadeVS | Animation")
        float rollAngle;
};
