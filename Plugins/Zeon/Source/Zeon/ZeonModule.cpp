
#include "ZeonModule.h"
#include "Public/Utility/PauseManager.h"
#include "Public/Utility/ZeonUtilits.h"

IMPLEMENT_MODULE(FZeonModule, Zeon)

void FZeonModule::StartupModule()
{
	FZeonUtil::Initialize();
	FPauseManager::Initialize();
}

void FZeonModule::ShutdownModule()
{	
	FZeonUtil::Shutdown();
	FPauseManager::Shutdown();
}