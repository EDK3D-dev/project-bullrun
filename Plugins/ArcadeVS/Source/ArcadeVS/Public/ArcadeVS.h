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
#include "Modules/ModuleManager.h"

class FArcadeVSModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
