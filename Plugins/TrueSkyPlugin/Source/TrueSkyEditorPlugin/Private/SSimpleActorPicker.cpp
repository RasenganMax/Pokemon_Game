// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SSimpleActorPicker.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "GameFramework/Actor.h"
#include "AssetData.h"
#include "Editor.h"
#include "Editor/SceneOutliner/Public/SceneOutlinerPublicTypes.h"
#include "Editor/SceneOutliner/Public/SceneOutlinerModule.h"
#include "AssetRegistryModule.h"
//#include "Private/UserInterface/PropertyEditor/PropertyEditorAssetConstants.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "SimpleActorPicker"

void SSimpleActorPicker::Construct( const FArguments& InArgs )
{
	CurrentActor = InArgs._InitialActor;
	bAllowClear = InArgs._AllowClear;
	ActorFilter = InArgs._ActorFilter;
	OnSet = InArgs._OnSet;
	OnClose = InArgs._OnClose;
	OnUseSelected = InArgs._OnUseSelected;

	FMenuBuilder MenuBuilder(true, NULL);


	//MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		TSharedPtr<SWidget> MenuContent;

		FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::Get().LoadModuleChecked<FSceneOutlinerModule>(TEXT("SceneOutliner"));

		SceneOutliner::FInitializationOptions InitOptions;
		InitOptions.Mode = ESceneOutlinerMode::ActorPicker;
		InitOptions.Filters->AddFilterPredicate(ActorFilter);
		InitOptions.bFocusSearchBoxWhenOpened = true;

		InitOptions.ColumnMap.Add(SceneOutliner::FBuiltInColumnTypes::Label(), SceneOutliner::FColumnInfo(SceneOutliner::EColumnVisibility::Visible, 0) );
		InitOptions.ColumnMap.Add(SceneOutliner::FBuiltInColumnTypes::ActorInfo(), SceneOutliner::FColumnInfo(SceneOutliner::EColumnVisibility::Visible, 10) );
		
		TSharedRef< ISceneOutliner > Outliner=SceneOutlinerModule.CreateSceneOutliner(InitOptions, FOnActorPicked::CreateSP(this, &SSimpleActorPicker::OnActorSelected));
	

		MenuContent =
			SNew(SBox)
			.WidthOverride(350.F)
			.HeightOverride(260.F)
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush("Menu.Background") )
				[
					Outliner
				]
			];

		MenuBuilder.AddWidget(MenuContent.ToSharedRef(), FText::GetEmpty(), true);
	}
	//MenuBuilder.EndSection();

	ChildSlot
	[
		MenuBuilder.MakeWidget()
	];
}

void SSimpleActorPicker::HandleUseSelected()
{
	OnUseSelected.ExecuteIfBound();
}

void SSimpleActorPicker::OnClear()
{
	SetValue(NULL);
	OnClose.ExecuteIfBound();
}

void SSimpleActorPicker::OnActorSelected( AActor* InActor )
{
	SetValue(InActor);
	OnClose.ExecuteIfBound();
}

void SSimpleActorPicker::SetValue( AActor* InActor )
{
	OnSet.ExecuteIfBound(InActor);
}

#undef LOCTEXT_NAMESPACE
