
#pragma once

#include "Modules/WindowModuleBase.h"
#include "Utility/ZeonUtilits.h"
#include "AbilitySystem/DynamicAbilitySystem.h" 
#include "TickerModules/AbilityUpdateTickerModule.h"
#include "Widgets/Layout/SWrapBox.h"

class SWrapBox;

template<typename T, typename AbilityT>
class FAbilityInfoWindowModule : public FWindowModuleBase
{
protected:
	
	virtual void OnPostPIEStarted(bool bArg) override
	{
		FWindowModuleBase::OnPostPIEStarted(bArg);
		if (const auto World = FZeonUtil::FindWorld({EWorldType::PIE}))
		{
			if (!World->GetFirstPlayerController()) return;
			if (const auto Pawn = World->GetFirstPlayerController()->GetPawn())
			{
				AbilitySystem = Pawn->GetComponentByClass<T>();
			}
		}
	}
	FORCEINLINE virtual bool UpdateWindowInformation(float DeltaTime) override
	{
		if (AbilitySystem.IsValid())
		{
			UpdateOwnedTags();
			UpdateAbilitiesInfo();	
			return true;
		}
		return false;
	}
	virtual TSharedRef<SDockTab> RegisterWindow(const FSpawnTabArgs& SpawnTabArgs) override;

	virtual void UpdateAbilityInfo(const FName& Key, const AbilityT* Ability) const;
	virtual void UpdateAbilityInfo_Internal(const FName& Key, const AbilityT* Ability,
		TSharedPtr<SVerticalBox>& LeftAbilityBox, TSharedPtr<SVerticalBox>& RightAbilityBox) const {}

	
	void UpdateOwnedTags() const;
	void UpdateAbilitiesInfo() const;

	TSharedPtr<SWrapBox> AbilitiesInfoBox;
	TSharedPtr<SVerticalBox> OwnedTagsBox;
	TWeakObjectPtr<T> AbilitySystem;
public:
	FAbilityInfoWindowModule()
	{
		// Base module set up:
		bSetUpButton = true;
		bUpdateWindow = true;
		bAutoManageTicker = true;
		UpdateWindowRate = 0.01f;
	}
};

template <typename T, typename AbilityT>
TSharedRef<SDockTab> FAbilityInfoWindowModule<T, AbilityT>::RegisterWindow(const FSpawnTabArgs& SpawnTabArgs)
{
	const auto Window = FWindowModuleBase::RegisterWindow(SpawnTabArgs);

	Window->SetContent(
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SBox)
			.Padding(10, 10)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Owner tags:"))))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SAssignNew(OwnedTagsBox, SVerticalBox)
				]
			]
		]
		+ SScrollBox::Slot()
		[
			SNew(SBox)
			.Padding(10, 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("CurrentAbilities info:"))))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 10)
				[
					SAssignNew(AbilitiesInfoBox, SWrapBox)
					.UseAllottedSize(true)
				]
			]
		]);

	return Window;
}

template <typename T, typename AbilityT>
void FAbilityInfoWindowModule<T, AbilityT>::UpdateOwnedTags() const
{
	const auto& OwnerTags = AbilitySystem->GetOwnedTags();
	OwnedTagsBox->ClearChildren();
	if (!OwnerTags.IsEmpty())
	{
		for (const auto& Tag : OwnerTags)
		{
			OwnedTagsBox->AddSlot()
			.Padding(0, 0)
			[
				SNew(STextBlock)	
				.Text(FText::FromString(Tag.ToString()))
			];
		}
	}
	else
	{
		OwnedTagsBox->AddSlot()
		[
			SNew(STextBlock)	
			.Text(FText::FromString(TEXT("No tags found")))
		];
	}
}

template <typename T, typename AbilityT>
void FAbilityInfoWindowModule<T, AbilityT>::UpdateAbilitiesInfo() const
{
	AbilitiesInfoBox->ClearChildren();
	if (!AbilitySystem->GetAbilities().IsEmpty())
	{
		for (const auto& AbilityData : AbilitySystem->GetAbilities())
		{
			if constexpr (std::is_same_v<AbilityT, UDynamicAbility>) UpdateAbilityInfo(AbilityData.Key, AbilityData.Value.Get());
			else UpdateAbilityInfo(AbilityData.Key, CastChecked<AbilityT>(AbilityData.Value.Get()));
		}
	}
	else
	{
		AbilitiesInfoBox->AddSlot()
		[
			SNew(STextBlock)	
			.Text(FText::FromString(TEXT("No abilities found")))
		];
	}
}

template <typename T, typename AbilityT>
void FAbilityInfoWindowModule<T, AbilityT>::UpdateAbilityInfo(const FName& Key, const AbilityT* Ability) const
{
	TSharedPtr<SVerticalBox> LeftAbilityBox;
	TSharedPtr<SVerticalBox> RightAbilityBox;
	const auto& CurrentAbilitySettings =  UDynamicAbilitySystem::FindSlideData(Ability, ESlideSettingsType::Current, true);
	
	// Base Data:
	AbilitiesInfoBox->AddSlot()
	.Padding(5, 10)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.141176f, 0.141176f, 0.141176f, 0.1))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			[
				SAssignNew(LeftAbilityBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Class: %s"), *Ability->GetClass()->GetName())))
				]
				+ SVerticalBox::Slot() // это должна быть доп инфа
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Max active time: %.2f"), CurrentAbilitySettings->MaxActiveTime)))
				]
				+ SVerticalBox::Slot() // это должна быть доп инфа
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Logic delay: %.2f"), CurrentAbilitySettings->ActivationDelay)))
				]
				+ SVerticalBox::Slot() // это должна быть доп инфа
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Update rate: %.2f"), CurrentAbilitySettings->UpdateAbilityRate)))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.AutoWidth()
			[
				SAssignNew(RightAbilityBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromName(Ability->AbilityName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Slide: %s"), *Ability->CurrentSlideType.ToString())))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()	
				[
					SNew(STextBlock)
					.Text(StaticEnum<EAbilityState>()->GetDisplayNameTextByValue(static_cast<uint8>(Ability->AbilityState)))
				]
			]
		]
	];

	// Additional Data:
	if (Ability->AbilityFlags.Contains(EAbilityFlag::Updating) && CurrentAbilitySettings->MaxActiveTime != 0.f)  // AbilityRemainingTime
	{
		const auto* UpdateModule = AbilitySystem->template GetTickerModule<FAbilityUpdateTickerModule>();
		const FUpdateAbilityTickerData* Task = UpdateModule ? UpdateModule->GetUpdateTask(Key) : nullptr;
		check(Task)
		LeftAbilityBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Remaining time: %.2f"), Task->MaxActiveTime - Task->RemainingTime)))
		];
	}

	// Additional Data:
	if (!CurrentAbilitySettings->SlideTags.IsEmpty())
	{
		RightAbilityBox->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Tags:"))))
			];
		
		for (auto Tag : CurrentAbilitySettings->SlideTags)
		{
			RightAbilityBox->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(*Tag.ToString()))
			];
		}
	}
	UpdateAbilityInfo_Internal(Key, Ability, LeftAbilityBox, RightAbilityBox);
}