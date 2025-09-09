
#include "ZeonUtilits.h"

TUniquePtr<FZeonUtil> FZeonUtil::Instance;
FDelegateHandle FZeonUtil::PostWorldInitDelegateHandle;
FZeonUtil::FOnWorldBeginPlay FZeonUtil::OnWorldBeginPlay;