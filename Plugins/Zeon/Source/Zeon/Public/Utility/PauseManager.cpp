
#include "PauseManager.h"

TUniquePtr<FPauseManager> FPauseManager::Instance;
FPauseManager::FOnGamePause FPauseManager::OnGamePause;