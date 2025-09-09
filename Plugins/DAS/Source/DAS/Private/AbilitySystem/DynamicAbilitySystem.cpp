
#include "AbilitySystem/DynamicAbilitySystem.h"
#include "TickerModules/AbilityUpdateTickerModule.h"
#include "TickerModules/FunHolderTickerModule.h"

DEFINE_LOG_CATEGORY(LogDynamicAbilitySystem);

void UDynamicAbilitySystem::BeginPlay()
{
	Super::BeginPlay();
	CreateRegisteredAttributes();
	SetUpTickerManager();

#if WITH_TOUCH
	if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		TouchManager = GameInstance->GetSubsystem<UTouchManager>();
	}
#endif
}

void UDynamicAbilitySystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	CurrentAbilities.Empty();
	Attributes.Empty();
}

void UDynamicAbilitySystem::SetUpTickerManager()
{
	AutoActivateTickerType = { ETickerStateType::GameUnPaused };
	AutoDisableTickerType = { ETickerStateType::GamePaused };

	AddTickerModule<FFunHolderTickerModule>();
	FAbilityUpdateTickerModule* AbilityUpdateTickerModule = AddTickerModule<FAbilityUpdateTickerModule>();
	AbilityUpdateTickerModule->AbilityUpdateInvoker.Bind(this, &UDynamicAbilitySystem::UpdateAbility);
	
	AbilityUpdateTickerModule->DisableAbilityInvoker.Bind([this](const FName& Key){
		const auto& AbilityStorage = CurrentAbilities.FindChecked(Key);
		if (const auto Ability = AbilityStorage.Get(); !Ability->CurrentSlideType.IsValid()) // если на базовом слайде, то полностью выключаем способность
		{
			OnAbilityDisabled(Ability, EDisableType::End, this, FGameplayTag::EmptyTag);
		}
		else // если слайд кастомный, то сбрасываемся к базовому
		{
			ChangeAbilitySlide(Ability, FGameplayTag::EmptyTag);
		}
	});
}
	
bool UDynamicAbilitySystem::AddAbility(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder)
{
	if (!ValidateAbilityAddition(Key, AbilityClass, Adder)) return false;
	if (auto Ability = NewObject<UDynamicAbility>(this, AbilityClass))
	{
		CurrentAbilities.Add(Key, TStrongObjectPtr(MoveTemp(Ability)));
		OnAbilityAdded(Key, Ability, Adder);
		
		if (Ability->AbilitySettings.ActivateAbilityOnGranted) ActivateAbility(Key, Adder);
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Failed to create ability object of name '%s'."), *Key.ToString());
	return false;
}
bool UDynamicAbilitySystem::ValidateAbilityAddition(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder) const
{
	if (!Adder) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to add ability class '%s', but adder was invalid"), *AbilityClass->GetName());
	if (Key == NAME_None) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to add ability class '%s', but this ability has invalid name 'None'."), *AbilityClass->GetName());
	if (CurrentAbilities.Contains(Key))
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Attempted to add ability '%s', but this ability has already been added."), *Key.ToString());
		return false;
	}
	return true;
}
void UDynamicAbilitySystem::OnAbilityAdded(const FName Key, UDynamicAbility* Ability, const UObject* Adder)
{
	Ability->AbilitySystem = this;
	Ability->Owner = GetOwner();
	FindAndSetAbilitySettings(Key, Ability);
	Ability->OnAbilityAdded(Adder);
	OnAddedAbility.Broadcast(Key);
}

bool UDynamicAbilitySystem::RemoveAbility(const FName Key, const UObject* Remover)
{
	if (!Remover) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to Remove ability, but Remover has invalid."));
	if (Key == NAME_None) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to Remove ability, but ability name has invalid 'None'."));
	if (const auto AbilityStorage = CurrentAbilities.Find(Key))
	{
		const auto Ability = AbilityStorage->Get();
		
		DisableAbility(Ability, EDisableType::Removed, Remover, FGameplayTag::EmptyTag);
		OnRemovedAbility.Broadcast(Key);
		Ability->OnAbilityRemoved(Remover);
		CurrentAbilities.Remove(Key);
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Cannot remove ability '%s' because it was not added."), *Key.ToString());
	return false;
}

bool UDynamicAbilitySystem::ActivateAbility(const FName Key, const UObject* Activator)
{
	if (Key == NAME_None) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to activate ability, but this ability has invalid name 'None'."));
	if (!Activator) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to activate ability, but adder was invalid"));
	if (const auto AbilityContainer = CurrentAbilities.Find(Key))
	{
		auto Ability = AbilityContainer->Get();
		if (Ability->AbilityState == EAbilityState::Inactive)
		{
			const auto Settings = FindSlideData(Ability, ESlideSettingsType::Auto);
			if (!ValidateSlideChange(*Settings)) return false;
			if (!Ability->ValidateAbilityActivation(Activator)) return false;
			
			if (Settings->ActivationDelay == 0.f) OnAbilityActivated(Ability, Activator);
			else
			{
				Ability->AbilityState = EAbilityState::Activating;
				GetTickerModuleMutable<FFunHolderTickerModule>()->AddDelayedFun(Key, Settings->ActivationDelay)->Bind([this, Ability, Activator]
				{
					OnAbilityActivated(Ability, Activator);
				});
			}
			return true;
		}
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Cannot activate ability '%s' because it is already active."), *Ability->GetName());
	}
	UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Attempted to activate ability '%s', but it could not be found"), *Key.ToString());
	return false;

}

void UDynamicAbilitySystem::OnAbilityActivated(UDynamicAbility* Ability, const UObject* Activator)
{
	const auto Settings = FindSlideData(Ability, ESlideSettingsType::Auto);
	Ability->AbilityState = EAbilityState::Active;
	OwnedTags.AppendTags(Settings->SlideTags);
	OverrideAbilities(Ability);
	Ability->OnAbilityActivated(Activator);
	
	if (Settings->UpdateAbilityRate != 0.f || Settings->bTickEveryFrame)
	{
		Ability->AbilityFlags.Add(EAbilityFlag::Updating);
		const auto UpdateRate = Settings->bTickEveryFrame ? 0.f : Settings->UpdateAbilityRate;
		GetTickerModuleMutable<FAbilityUpdateTickerModule>()->ReSetAbilityUpdate(Ability->AbilityName, UpdateRate, Settings->MaxActiveTime);
	}
}

bool UDynamicAbilitySystem::DisableAbility(UDynamicAbility* Ability, const EDisableType& DisableType, const UObject* Disabler, const FGameplayTag& DisableReason)
{
	if (!Ability) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to disable ability, but ability was invalid"));
	if (!Disabler) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to disable ability, but disabler was invalid"));
	if (Ability->AbilityState != EAbilityState::Inactive)
	{
		if (Ability->AbilityState == EAbilityState::Activating) GetTickerModuleMutable<FFunHolderTickerModule>()->RemoveDelayedFun(Ability->AbilityName);
		else if (Ability->AbilityFlags.Contains(EAbilityFlag::Updating)) GetTickerModuleMutable<FAbilityUpdateTickerModule>()->EndUpdateAbility(Ability->AbilityName);

		OnAbilityDisabled(Ability, DisableType, Disabler, DisableReason);
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Cannot disable ability '%s', because it already disabled"), *Ability->GetName());
	return false;
}
void UDynamicAbilitySystem::OnAbilityDisabled(UDynamicAbility* Ability, const EDisableType& DisableType, const UObject* Disabler, const FGameplayTag& DisableReason)
{
	// сбрасываем настройки менеджера
	OwnedTags.RemoveTags(FindSlideData(Ability, ESlideSettingsType::Base, true)->SlideTags);
	if (Ability->CurrentSlideType.IsValid()) // дополнительно очищаем от тегов слайда
	{
		OwnedTags.RemoveTags(FindSlideData(Ability, ESlideSettingsType::Current)->SlideTags);
	}

	// сбрасываем настройки способности
	Ability->AbilityFlags.Remove(EAbilityFlag::Updating);
	Ability->CurrentSlideType = FGameplayTag::EmptyTag;
	Ability->AbilityState = EAbilityState::Inactive;
	Ability->OnAbilityDisabled(DisableType, DisableReason, Disabler);
}

bool UDynamicAbilitySystem::UpdateAbility(const FName& Key, const float DeltaTime)
{
	if (const auto AbilityStorage = CurrentAbilities.Find(Key))
	{
		if (const auto Ability = AbilityStorage->Get())
		{
			if (const TOptional<FGameplayTag>& Reason = Ability->UpdateAbility(DeltaTime); Reason.IsSet())
			{
				if (!Ability->CurrentSlideType.IsValid()) // если на базовом слайде, то полностью выключаем способность
				{
					OnAbilityDisabled(Ability, EDisableType::FromUpdate, this, Reason.GetValue());
				}
				else // если слайд кастомный, то сбрасываемся к базовому
				{
					ChangeAbilitySlide(Ability, FGameplayTag::EmptyTag);
				}
			}
		}
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Cannot update ability '%s'"), *Key.ToString());
	return false;
}

bool UDynamicAbilitySystem::ValidateSlideChange(const FAbilitySlideSettings& SlideSettings) const
{
	if (OwnedTags.HasAny(SlideSettings.SlideTags)) return false;
	if (OwnedTags.HasAny(SlideSettings.ActivationBlockedTags)) return false;
	if (!SlideSettings.ActivationNecessaryTags.IsEmpty() && !OwnedTags.HasAny(SlideSettings.ActivationNecessaryTags)) return false;
	return true;
}

bool UDynamicAbilitySystem::ChangeAbilitySlide(UDynamicAbility* Ability, const FGameplayTag& SlideName)
{
	if (!Ability) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to change ability slide, but ability was invalid"));
	if (Ability->AbilityState == EAbilityState::Inactive)
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Cannot change ability slide because the ability is not active"));
		return false;
	}
	if (Ability->AbilityState == EAbilityState::Activating)
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Cannot change ability slide while the ability is activating"));
		return false;
	}
	if (Ability->CurrentSlideType == SlideName)
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Cannot switch to slide '%s' because it is already active"), *SlideName.ToString());
		return false;
	}
	
	if (const auto NewSlideSettings = FindSlideData(Ability, ESlideSettingsType::Auto, false, SlideName))
	{
		if (SlideName.IsValid() && !ValidateSlideChange(*NewSlideSettings)) return false; // проверяем только если переключаемся не на базовый слайд
		if (SlideName.IsValid() && !Ability->ValidateSlideChange(SlideName)) return false; // проверяем только если переключаемся не на базовый слайд

		if (NewSlideSettings->ActivationDelay == 0.f) OnAbilitySlideChanged(Ability, SlideName);
		GetTickerModuleMutable<FFunHolderTickerModule>()->AddDelayedFun(Ability->AbilityName, NewSlideSettings->ActivationDelay)->Bind([this,  Ability, SlideName]
		{
			OnAbilitySlideChanged(Ability, SlideName);
		});

		Ability->AbilityState = EAbilityState::Activating;
		if (Ability->AbilityFlags.Contains(EAbilityFlag::Updating))  // нужно если мы меняем слайд, который был в update
		{
			Ability->AbilityFlags.Remove(EAbilityFlag::Updating);
			GetTickerModuleMutable<FAbilityUpdateTickerModule>()->EndUpdateAbility(Ability->AbilityName);
		}
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Settings for slide '%s' could not be found"), *SlideName.ToString());
	return false;
}
bool UDynamicAbilitySystem::OnAbilitySlideChanged(UDynamicAbility* Ability, const FGameplayTag& SlideName)
{
	if (const auto* NewSlideSettings = FindSlideData(Ability, ESlideSettingsType::Auto, false, SlideName))
	{
		if (!Ability->ValidateSlideChange(SlideName)) return false;
		
		if (!Ability->CurrentSlideType.IsValid() && SlideName.IsValid()) // переключение с базового на кастомный
		{
			OwnedTags.AppendTags(NewSlideSettings->SlideTags);
		}
		else if (Ability->CurrentSlideType.IsValid() && !SlideName.IsValid()) // переключение с кастомного на базовый
		{
			OwnedTags.RemoveTags(FindSlideData(Ability, ESlideSettingsType::Current, true)->SlideTags);
		}
		else if (Ability->CurrentSlideType.IsValid() && SlideName.IsValid()) // переключение с кастомного на кастомный
		{
			OwnedTags.RemoveTags(FindSlideData(Ability, ESlideSettingsType::Current, true)->SlideTags);
			OwnedTags.AppendTags(NewSlideSettings->SlideTags);
		}

		if (NewSlideSettings->UpdateAbilityRate != 0.f || NewSlideSettings->bTickEveryFrame)
		{
			Ability->AbilityFlags.Add(EAbilityFlag::Updating);
			const auto UpdateRate = NewSlideSettings->bTickEveryFrame ? 0.f : NewSlideSettings->UpdateAbilityRate;
			GetTickerModuleMutable<FAbilityUpdateTickerModule>()->ReSetAbilityUpdate(Ability->AbilityName, UpdateRate, NewSlideSettings->MaxActiveTime);
		}
				
		Ability->CurrentSlideType = SlideName;
		Ability->AbilityState = EAbilityState::Active;
		Ability->OnSlideChanged(SlideName);
		return true;
	}
	UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Settings for slide '%s' could not be found"), *SlideName.ToString());
	return false;
}

const FAbilitySlideSettings* UDynamicAbilitySystem::FindSlideData(const UDynamicAbility* Ability, const ESlideSettingsType& SettingsType,
	const bool bEnsureResult, const FGameplayTag& SlideName)
{
	if (!Ability) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to find ability slide data, but ability was invalid"));
	const FAbilitySlideSettings* Result = nullptr;
	switch (SettingsType)
	{
		case ESlideSettingsType::Auto: Result = !SlideName.IsValid() ? &Ability->AbilitySettings.BaseSlideSettings : Ability->AbilitySettings.SlidesSettings.Find(SlideName); break;
		case ESlideSettingsType::Base: Result = &Ability->AbilitySettings.BaseSlideSettings; break;
		case ESlideSettingsType::Custom: Result = Ability->AbilitySettings.SlidesSettings.Find(SlideName); break;
		case ESlideSettingsType::Current: Result = !Ability->CurrentSlideType.IsValid() ? &Ability->AbilitySettings.BaseSlideSettings : Ability->AbilitySettings.SlidesSettings.Find(Ability->CurrentSlideType); break;
	}
	if (bEnsureResult && !Result)
	{
		UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Settings for slide '%s' could not be found"), *SlideName.ToString());
		return nullptr;
	}
	if (!Result)
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("Settings for slide '%s' could not be found"), *SlideName.ToString());
		return nullptr;
	}
	return Result;
}

void UDynamicAbilitySystem::AddAbilityInput(const FGameplayTag& InputKey, const ETriggerEvent& Event)
{
	bool bInputCalled = false;
	for (const auto& AbilityData : CurrentAbilities)
	{
		if (const auto Ability = AbilityData.Value.Get(); Ability && Ability->AbilityState == EAbilityState::Active)
		{
			if (!Ability->AbilitySettings.InputsKeys.Contains(InputKey)) continue;
			if (Ability->AbilityState == EAbilityState::Inactive) continue;
			Ability->OnAddInput(InputKey, Event);
			bInputCalled = true;
		}
	}
	if (!bInputCalled) UE_LOG(LogDynamicAbilitySystem, Error, TEXT("No ability found that can handle input with InputKey: %s"), *InputKey.ToString());
}

void UDynamicAbilitySystem::AddAbilityInputVector(const FVector& WorldVector, const FGameplayTag& InputKey, const ETriggerEvent& Event)
{
	bool bInputCalled = false;
	for (const auto& AbilityData : CurrentAbilities)
	{
		if (const auto Ability = AbilityData.Value.Get(); Ability && Ability->AbilityState == EAbilityState::Active)
		{
			if (!Ability->AbilitySettings.InputsKeys.Contains(InputKey)) continue;
			if (Ability->AbilityState == EAbilityState::Inactive) continue;
			Ability->OnAbilityInputVector(WorldVector, InputKey, Event);
			bInputCalled = true;
		}
	}
	if (!bInputCalled) UE_LOG(LogDynamicAbilitySystem, Error, TEXT("No ability found that can handle input with InputKey: %s"), *InputKey.ToString());
}

UAttribute* UDynamicAbilitySystem::GetAttribute(const UDynamicAbility* Ability, const TSubclassOf<UAttribute>& AttributeClass)
{
	if (IsAbilityHasAllAllows(Ability))
	{
		if (const auto NewAttribute = Attributes.Find(AttributeClass)) return NewAttribute->Get();
		return nullptr;
	}

	const auto AbilityClass = Ability->GetClass();
	if (!AttributesSecuritySettings.Contains(AbilityClass))
	{
		UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Failed to find a registered attribute class '%s'"), *AttributeClass->GetName());
		return nullptr;
	}
	for (auto& Attribute : AttributesSecuritySettings[AbilityClass].Attributes)
	{
		if (Attribute != AttributeClass) continue;
		if (const auto NewAttribute = Attributes.Find(AttributeClass)) return NewAttribute->Get();
	}
	UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Ability '%s' is not permitted to access attribute '%s'"), *AbilityClass->GetName(), *AttributeClass->GetName());
	return nullptr;
}

UObject* UDynamicAbilitySystem::GetContextObject(const FName Key, const UDynamicAbility* Ability, const bool bConst)
{
	if (IsAbilityHasAllAllows(Ability))
	{
		if (const auto Object = ContextObjects.Find(Key)) return Object->Get();
		return nullptr;
	}
	
	if (!ContextObjects.Contains(Key))
	{
		UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("No context object found for key '%s'"), *Key.ToString());
		return nullptr;
	}
	if (bConst)
	{
		if (const auto Settings = ContextObjectsConstAllows.Find(Key))
		{
			if (!Settings->ConstAllows.Contains(Ability->GetClass()))
			{
				UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Ability '%s' is not permitted to access context object for key '%s' by non-const reference"), *Ability->GetClass()->GetName(), *Key.ToString());
				return nullptr;
			}
		}
		else
		{
			UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("No const access settings found for key '%s'"), *Key.ToString());
			return nullptr;
		}
	}
	if (const auto Object = ContextObjects.Find(Key)) return Object->Get();
	return nullptr;
}

void UDynamicAbilitySystem::OverrideAbilities(const UDynamicAbility* Overrider)
{
	if (!Overrider) UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to override abilities, but overrider was invalid"));
	const auto& OverriderTags = Overrider->AbilitySettings.OverrideTags;
	for (const auto& AbilityData : CurrentAbilities)
	{
		const auto& Ability = AbilityData.Value.Get();
		\
		if (Ability == Overrider) continue;
		if (Ability->AbilityState == EAbilityState::Inactive) continue;
		
		if (FindSlideData(Ability, ESlideSettingsType::Base, true)->SlideTags.HasAny(OverriderTags)) // Проверяем теги базового слайда
		{
			DisableAbility(Ability, EDisableType::Overridden, Overrider, FGameplayTag::EmptyTag);
		}
		if (Ability->CurrentSlideType.IsValid()) // Проверяем если ли кастомный активный слайд, и если да то тоже проверяем его теги тоже	
		{
			if (FindSlideData(Ability, ESlideSettingsType::Custom, true)->SlideTags.HasAny(OverriderTags))
			{
				DisableAbility(Ability, EDisableType::Overridden, Overrider, FGameplayTag::EmptyTag);
			}
		}
	}
}