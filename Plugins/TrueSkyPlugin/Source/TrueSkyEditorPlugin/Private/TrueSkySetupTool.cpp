#include "TrueSkySetupTool.h"
#include "TrueSkyEditorPluginPrivatePCH.h"
#include "ScopedTransaction.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/MessageDialog.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SButton.h"


#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Workflow/SWizard.h"
#include "AssetRegistryModule.h"
#include "TrueSkySequenceAsset.h"
#include "IDocumentation.h"
#include "TutorialMetaData.h"
#include "SAssetPicker.h"
#include "SSimpleActorPicker.h"
#include "PropertyCustomizationHelpers.h"
#include "ContentBrowserModule.h"
#include "Engine/Light.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Atmosphere/AtmosphericFog.h"
#define LOCTEXT_NAMESPACE "TrueSkySetup"

struct FSequenceAssetItem
{
	UTrueSkySequenceAsset *SequenceAsset;

	FSequenceAssetItem(UTrueSkySequenceAsset *a)
		: SequenceAsset(a)
	{}
};

bool STrueSkySetupTool::IsDirectionalLight( const AActor* const Actor ) const
{
	return Actor->IsA( ADirectionalLight::StaticClass() ) && !Actor->IsChildActor();
}
void STrueSkySetupTool::OnDirectionalLightSelected( AActor*  Actor ) 
{
	DirectionalLight=static_cast<ADirectionalLight*>(Actor);
}

void STrueSkySetupTool::DirectionalLightCloseComboButton()
{
	//AssetComboButton->SetIsOpen(false);
}

void STrueSkySetupTool::DirectionalLightOnUse()
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void STrueSkySetupTool::Construct( const FArguments& InArgs )
{
	OnTrueSkySetup = InArgs._OnTrueSkySetup;
	CreateTrueSkyLight=InArgs._CreateTrueSkyLight;
	OverridePostProcessVolume = InArgs._OverridePostProcessVolume;
	AltitudeLight = InArgs._AltitudeLight;
	DirectionalLight=InArgs._DirectionalLight;

	DeleteFogs=true;
	DeleteBPSkySphere=true;
	Sequence=InArgs._Sequence;

	TArray<AActor*> DirectionalLights;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ADirectionalLight::StaticClass(), DirectionalLights);

	DirectionalLight=(ADirectionalLight*)(DirectionalLights.Num()?DirectionalLights[0]:nullptr);

	TArray<AActor*> TrueSkyLights;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ATrueSkyLight::StaticClass(), TrueSkyLights);

	CreateTrueSkyLight=(TrueSkyLights.Num()==0);
	ShowAllSequences=false;
	check(OnTrueSkySetup.IsBound());
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(*UTrueSkySequenceAsset::StaticClass()->GetName());
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.bCanShowFolders=true;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &STrueSkySetupTool::OnSequenceAssetSelected2);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetPicker = StaticCastSharedRef<SAssetPicker>(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));

	{
		DirectionalLightPicker=
			SNew( SSimpleActorPicker )
			.InitialActor(DirectionalLight)
			.AllowClear(true)
			.ActorFilter(FOnShouldFilterActor::CreateSP( this, &STrueSkySetupTool::IsDirectionalLight ))
			.OnSet(FOnActorSelected::CreateSP( this, &STrueSkySetupTool::OnDirectionalLightSelected))
			.OnClose(FSimpleDelegate::CreateSP( this, &STrueSkySetupTool::DirectionalLightCloseComboButton ))
			.OnUseSelected(FSimpleDelegate::CreateSP( this, &STrueSkySetupTool::DirectionalLightOnUse ));
			;
	}
	SetupSequenceAssetItems();
	ChildSlot
	[
		SNew(SBorder)
		.Padding(18)
		.BorderImage( FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush") )
		[
			SNew(SVerticalBox)
			.AddMetaData<FTutorialMetaData>(TEXT("AddTrueSkyMajorAnchor"))

			+SVerticalBox::Slot()
			[
				SAssignNew( MainWizard, SWizard)
				.ShowPageList(false)

				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
				.CancelButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
				.FinishButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
				.ButtonTextStyle(FEditorStyle::Get(), "LargeText")
				.ForegroundColor(FEditorStyle::Get().GetSlateColor("WhiteBrush"))

				.CanFinish(this, &STrueSkySetupTool::CanFinish)
				.FinishButtonText(  LOCTEXT("TrueSkyFinishButtonText", "Initialize") )
				.FinishButtonToolTip (
					LOCTEXT("TrueSkyFinishButtonToolTip", "Adds the trueSKY actors to your scene.") 
					)
				.OnCanceled(this, &STrueSkySetupTool::CancelClicked)
				.OnFinished(this, &STrueSkySetupTool::FinishClicked)
				.InitialPageIndex( 0)
				.PageFooter()
				[
					SNew(SBorder)
					.Visibility( this, &STrueSkySetupTool::GetGlobalErrorLabelVisibility )
					.BorderImage( FEditorStyle::GetBrush("NewClassDialog.ErrorLabelBorder") )
					.Padding(FMargin(0, 5))
					.Content()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("MessageLog.Warning"))
						]

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text( LOCTEXT( "TrueSkySetupToolErrorLabel", "" ) )
							.TextStyle( FEditorStyle::Get(), "NewClassDialog.ErrorLabelFont" )
						]
					]
				]



	
				+SWizard::Page()
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Center)
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle( FEditorStyle::Get(), "NewClassDialog.PageTitle" )
						.Text( LOCTEXT( "InitializeTrueSkyTitle", "Initialize trueSKY" ) )
					]

					// Title
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(EHorizontalAlignment::HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TrueSkySetupToolDesc", "This wizard will initialize trueSKY in the current Level.") )
						.AutoWrapText(true)
						.TextStyle(FEditorStyle::Get(), "NewClassDialog.ParentClassItemTitle")
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4, 5, 0, 10)
					[
						SNew(SCheckBox)
						.IsChecked( this, &STrueSkySetupTool::IsDeleteFogsChecked )
						.OnCheckStateChanged( this, &STrueSkySetupTool::OnDeleteFogsChanged )
						.ToolTipText(LOCTEXT("TrueSkySetupDeleteFogs", "trueSKY produces its own fog effects"))
						[
							SNew(STextBlock)
							.Text( LOCTEXT( "DeleteFogs", "Delete Atmospheric and Height Fog actors" ) )
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 10)
					[
						SNew(SCheckBox)
						.IsChecked( this, &STrueSkySetupTool::IsDeleteBPSkySphereChecked )
						.OnCheckStateChanged( this, &STrueSkySetupTool::OnDeleteBPSkySphereChanged )
						.ToolTipText(LOCTEXT("TrueSkySetupDeleteSkyEffects", "trueSKY will be hidden behind the Sky Effects such as Sky Spheres if they are not deleted"))
						[
							SNew(STextBlock)
							.Text( LOCTEXT( "Delete Sky Spheres and Effects", "Delete Unreal's Sky effects that will hide trueSKY such as the BP_Sky_Sphere or Sky Atmosphere" ) )
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 10)
					[
							SNew(SCheckBox)
							.IsChecked(this, &STrueSkySetupTool::IsOverridePostProcessVolumeChecked)
							.OnCheckStateChanged(this, &STrueSkySetupTool::OnOverridePostProcessVolumeChanged)
							.ToolTipText(LOCTEXT("TrueSkySetupOverridePostProcess", "We need to apply our transparency material function to a global post process, so weather effects such as rain are visible."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("OverridePostProcessVolume", "Modify Unbound Post Process in scene. If not found or unticked, one will be created."))
						]
					]
#if SIMUL_ALTITUDE_DIRECTIONAL_LIGHT
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 10)
					[
							SNew(SCheckBox)
							.IsChecked(this, &STrueSkySetupTool::IsAltitudeLightChecked)
							.OnCheckStateChanged(this, &STrueSkySetupTool::OnAltitudeLightChanged)
							.ToolTipText(LOCTEXT("TrueSkySetupAltitudeLight", "If checked, the directional (sun/moon) light will have altitude-dependent colour and brightness, useful for large-scale environments such as flight simulators."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AltitudeLight", "Altitude-dependent directional light."))
						]
					]
#endif
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 10)
					[
						SNew(SCheckBox)
						.IsChecked(this, &STrueSkySetupTool::IsCreateTrueSkyLightChecked)
						.OnCheckStateChanged(this, &STrueSkySetupTool::OnCreateTrueSkyLightChanged)
						.ToolTipText(LOCTEXT("TrueSkySetupCreateTrueSkyLight", "A TrueSKYLight provides real-time ambient lighting. Creating a trueSKYLight will delete and replace the current Skylights in the scene."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CreateTrueSkyLight", "Create a trueSKYLight and delete other Skylights"))
						]
					]
																			
				]

				+SWizard::Page()
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle( FEditorStyle::Get(), "NewClassDialog.PageTitle" )
						.Text( LOCTEXT( "WeatherStateTitle", "Choose a Sequence Asset" ) )
					]

					// Title
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0)
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TrueSkySetupToolDesc", "Choose which weather sequence to use initially.") )
							.AutoWrapText(true)
							.TextStyle(FEditorStyle::Get(), "NewClassDialog.ParentClassItemTitle")
						]

						// Full tree checkbox
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew(SCheckBox)
							.IsChecked( this, &STrueSkySetupTool::IsShowAllSequencesChecked )
							.OnCheckStateChanged( this, &STrueSkySetupTool::OnShowAllSequencesChanged )
							[
								SNew(STextBlock)
								.Text( LOCTEXT( "ShowAllSequences", "Show All Sequences" ) )
							]
						]
					]
					// Title spacer
					+SVerticalBox::Slot()
					.FillHeight(1.f)
					.Padding(0, 12, 0, 8)
					[
						SNew(SBorder)
						.AddMetaData<FTutorialMetaData>(TEXT("ChooseSequence"))
						.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							[
								// Basic view
								SAssignNew(SequenceAssetListView, SListView< TSharedPtr<FSequenceAssetItem> >)
								.ListItemsSource(&SequenceAssetItemsSource)
								.SelectionMode(ESelectionMode::Single)
								.ClearSelectionOnClick(false)
								.OnGenerateRow(this, &STrueSkySetupTool::MakeSequenceAssetListViewWidget)
								.OnMouseButtonDoubleClick( this, &STrueSkySetupTool::OnSequenceAssetItemDoubleClicked )
								.OnSelectionChanged(this, &STrueSkySetupTool::OnSequenceAssetSelected)
								.Visibility(this, &STrueSkySetupTool::GetTemplateSequenceVisibility)
							]
							+SVerticalBox::Slot()
							[
								// Advanced view
								SNew(SBox)
								.Visibility(this, &STrueSkySetupTool::GetAllSequenceVisibility)
								[
									AssetPicker.ToSharedRef()
								]
							]
						]
					]
				]
				+SWizard::Page()
				[
					SNew(SVerticalBox)
					
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 32, 12, 8)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectDirectionalLightDesc", "TrueSky can take control of a directional light to provide real-time sun and moon lighting. If none is selected, a new directional light will be created.") )
						.AutoWrapText(true)
						.TextStyle(FEditorStyle::Get(), "NewClassDialog.ParentClassItemTitle")
					]
					+SVerticalBox::Slot()
					.FillHeight(.5f)
					[
						DirectionalLightPicker.ToSharedRef()
					]
				]
			]
		]
	];
	// Select the first item
	if (  SequenceAssetItemsSource.Num() > 0 )
	{
		SequenceAssetListView->SetSelection(SequenceAssetItemsSource[0], ESelectInfo::Direct);
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


EVisibility STrueSkySetupTool::GetTemplateSequenceVisibility() const
{
	return ShowAllSequences ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility STrueSkySetupTool::GetAllSequenceVisibility() const
{
	return ShowAllSequences ? EVisibility::Visible : EVisibility::Collapsed;
}

void STrueSkySetupTool::SetupSequenceAssetItems()
{
	SequenceAssetItemsSource.Reserve(50);
	SequenceAssetItemsSource.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	const UClass* C = UTrueSkySequenceAsset::StaticClass();
	AssetRegistryModule.Get().GetAssetsByClass(C->GetFName(), AssetData);
	for(auto &I:AssetData)
	{
		FString path=I.ObjectPath.ToString();
		// This is just for sequences in the TrueSKY folder. Templates.
		if(path.Find(TEXT("/TrueSkyPlugin/SequenceSamples"), ESearchCase::IgnoreCase) < 0)
			if (path.Find(TEXT("/TrueSkyPlugin/SequenceSamples"), ESearchCase::IgnoreCase) < 0)
			continue;
		UTrueSkySequenceAsset * asset= static_cast<UTrueSkySequenceAsset*>(I.GetAsset());
		if(asset)
		{
			SequenceAssetItemsSource.Add(MakeShareable( new FSequenceAssetItem(asset) ));
		}
	}
}

void STrueSkySetupTool::OnSequenceAssetItemDoubleClicked( TSharedPtr<FSequenceAssetItem> TemplateItem )
{
	// Advance to the name page
	const int32 NextPageIdx = 1;
	if ( MainWizard->CanShowPage(NextPageIdx) )
	{
		MainWizard->ShowPage(NextPageIdx);
	}
}

void STrueSkySetupTool::OnSequenceAssetSelected(TSharedPtr<FSequenceAssetItem> Item, ESelectInfo::Type SelectInfo)
{
	if ( Item.IsValid() )
	{
		//ClassViewer->ClearSelection();
		Sequence = Item->SequenceAsset;
	}
	else
	{
		Sequence= nullptr;
	}
}

void STrueSkySetupTool::OnSequenceAssetSelected2(const FAssetData& AssetData)
{
	if ( AssetData.IsValid() )
	{
		Sequence = static_cast<UTrueSkySequenceAsset*>(AssetData.GetAsset());
	}
	else
	{
		Sequence = nullptr;
	}
}

TSharedRef<ITableRow> STrueSkySetupTool::MakeSequenceAssetListViewWidget(TSharedPtr<FSequenceAssetItem> SequenceAssetItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(SequenceAssetItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FSequenceAssetItem>>, OwnerTable );
	}

	if ( !SequenceAssetItem->SequenceAsset )
	{
		return SNew( STableRow<TSharedPtr<FSequenceAssetItem>>, OwnerTable );
	}
	//const FSlateBrush* const ClassBrush = UTrueSkySequenceAsset::StaticClass()->GetClassIcon();
	const int32 ItemHeight = 64;
	const int32 DescriptionIndent = 32;
	const FText ItemName = FText::FromString(SequenceAssetItem->SequenceAsset->GetName());
	const FString Description=SequenceAssetItem->SequenceAsset->Description;
	return
		SNew( STableRow<TSharedPtr<FSequenceAssetItem>>, OwnerTable )
		.Style(FEditorStyle::Get(), "NewClassDialog.ParentClassListView.TableRow")
		[
			SNew(SBox).HeightOverride(ItemHeight)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(8)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle( FEditorStyle::Get(), "NewClassDialog.ParentClassItemTitle" )
						.Text(ItemName)
					]
				]
				+SVerticalBox::Slot()
				.Padding(8)
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Description))
				]
			]
		];
}

EVisibility STrueSkySetupTool::GetGlobalErrorLabelVisibility() const
{
	return GetGlobalErrorLabelText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText STrueSkySetupTool::GetGlobalErrorLabelText() const
{
	return FText::GetEmpty();
}

void STrueSkySetupTool::CancelClicked()
{
	CloseContainingWindow();
}

bool STrueSkySetupTool::CanFinish() const
{
	check(OnTrueSkySetup.IsBound());
	return true;
}

void STrueSkySetupTool::FinishClicked()
{
	OnTrueSkySetup.ExecuteIfBound(DirectionalLight==nullptr, DirectionalLight ,CreateTrueSkyLight ,Sequence,DeleteFogs,DeleteBPSkySphere, OverridePostProcessVolume, AltitudeLight);
	CloseContainingWindow();
}

void STrueSkySetupTool::CloseContainingWindow()
{
	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow( AsShared());

	if ( ContainingWindow.IsValid() )
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE