/* Copyright (C) PoyoWorks 2019, Inc - All Rights Reserved
*
* ArcadeVS - Arcade Vehicle System
* poyoworks@gmail.com
*
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
*
*/


#include "ArcadeVS.h"

#define LOCTEXT_NAMESPACE "FArcadeVSModule"

void FArcadeVSModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FArcadeVSModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FArcadeVSModule, ArcadeVS)