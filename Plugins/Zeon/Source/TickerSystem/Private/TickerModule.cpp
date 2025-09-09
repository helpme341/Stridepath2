
#include "TickerModule.h"
#include "StaticTickerManager.h"

void FTickerModule::TryStartTicker() const
{
	if (OwnerManager) OwnerManager->TryStartTicker();
}

void FTickerModule::TryEndTicker() const
{
	if (OwnerManager) OwnerManager->TryEndTicker(this);
}

void FTickerModule::TryEndTickerSave() const
{
	if (!NeedUpdate()) TryEndTicker();
}
