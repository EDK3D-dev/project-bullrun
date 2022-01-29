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
#include "ArcadeVSVisualizer.generated.h"


class AArcadeVSVehicle;
class UMaterialInterface;

/**
    Visualizer component class
    
    The visualizer is a very handy visual debugger actor component that can be added to any ArcadeVSVehicle class.
    It will draw the actual forces applied to the vehicle as well as its center of mass and 
    suspension ray casts.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARCADEVS_API UArcadeVSVisualizer : public UActorComponent
{
	GENERATED_BODY()

public:

    UArcadeVSVisualizer();

    // Properties

    /* Debug material to use one the vehicle when useDebugMaterial is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Enabled"))
        bool _enabled;

    /* Debug material to use one the vehicle when useDebugMaterial is enabled */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Debug Material"))
        UMaterialInterface* _debugMaterial;

    /* The vehicle's material will be overriden with the specified debug material */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Use Debug Material"))
        bool _useDebugMaterial;

    /* Time to keep the arrows displayed. Increasing this value means that you'll be able to see the previous frames 
    arrows, very usefull for debugging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
        meta = (DisplayName = "Life Time (s)", ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "3.0"))
        float _lifeTime;

    /* All forces will be scaled according to this factor (should be very small ~0.001). 
    Tweak that value until the arrows length are comfortable to see on your vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Force Scaling factor"))
        float _forceScalingFactor;

    /* Length of the sharp part of the arrows */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
        meta = (DisplayName = "Arrow Length", ClampMin = "0", ClampMax = "500", UIMin = "0", UIMax = "500"))
        int _arrowLength;

    /* Thickness of the arrows */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
        meta = (DisplayName = "Arrow Thickness", ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
        int _arrowThickness;

    /* Draws the center of mass, taking into account the center of mass offset specified in the vehicle's settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
        meta = (DisplayName = "Suspension Line Width", ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
        int _suspensionLineWidth;

    /* Draws the center of mass, taking into account the center of mass offset specified in the vehicle's settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Center Of Mass"))
        bool _drawCenterOfMass;

    /* Draws the center of mass, taking into account the center of mass offset specified in the vehicle's settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Traction Offset"))
        bool _drawTractionOffset;

    /* Draws the suspensions ray casts as well as the suspensions normals and impact normals */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Suspension Ray Casts"))
        bool _drawSuspensionRayCasts;

    /* Draws the acceleration force vector as an arrow */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Acceleration"))
        bool _drawAcceleration;

    /* Draws the linear velocity force vector as an arrow */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Velocity"))
        bool _drawLinearVelocity;

    /* Draws the adherence force vector as an arrow */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Draw Adherence"))
        bool _drawAdherence;


protected:

    // Called when the game starts
    virtual void BeginPlay() override;

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void DrawLinearVelocity();
    void DrawAcceleration();
    void DrawSuspensionRayCasts();
    void DrawAdherence();


protected:

    UPROPERTY()
    AArcadeVSVehicle* _vehicle;
    
    UPROPERTY()
    UPrimitiveComponent* _rootCpt;

    FTransform _transform;
    FVector _com;

    float _rayCastWidth;
    float _rayCastImpactSphereRadius;
};
