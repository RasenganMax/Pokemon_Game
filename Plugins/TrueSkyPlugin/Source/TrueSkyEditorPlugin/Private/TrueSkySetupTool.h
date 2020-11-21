#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "TrueSkyLight.h"
#include "TrueSkySequenceActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SComboBox.h"

class SEditableTextBox;
class SAssetPicker;
class SWizard;
struct FSequenceAssetItem;

DECLARE_DELEGATE_EightParams( FOnTrueSkySetup, bool, ADirectionalLight* ,bool, UTrueSkySequenceAsset *,bool ,bool, bool, bool);


#define S_DECLARE_CHECKBOX(name) \
	bool name; \
	ECheckBoxState Is##name##Checked() const { return name ? ECheckBoxState::Checked:ECheckBoxState::Unchecked;} \
	void On##name##Changed(ECheckBoxState InCheckedState) {name=(InCheckedState==ECheckBoxState::Checked);}

/**
 * A dialog to add trueSKY to the scene.
 **/
class STrueSkySetupTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STrueSkySetupTool )
		:_CreateTrueSkyLight(false)
		,_DirectionalLight(nullptr)
		, _OverridePostProcessVolume(true)
		, _AltitudeLight(false)
		,_Sequence(nullptr)
	{}
	/** A TrueSkyLight actor performs real-time ambient lighting.*/
	SLATE_ARGUMENT(bool,CreateTrueSkyLight)
	/** TrueSKY can drive a directional light to provide sunlight and moonlight.*/
	SLATE_ARGUMENT(ADirectionalLight*,DirectionalLight)
	/** If we are overriding a volume in the current scene*/
	SLATE_ARGUMENT(bool, OverridePostProcessVolume)
	/** If we want the directional light to have altitude-dependent colour/brightness - e.g. for flight sims.*/
	SLATE_ARGUMENT(bool, AltitudeLight)
	/** TrueSKY needs to apply a post process material for effects that use transparency*/
	SLATE_ARGUMENT(APostProcessVolume*, PostProcessVolume)
	/** The TrueSKY Sequence provides the weather state to render.*/
	SLATE_ARGUMENT(UTrueSkySequenceAsset *,Sequence)
	/** Event called when code is successfully added to the project */
	SLATE_EVENT( FOnTrueSkySetup, OnTrueSkySetup )
	SLATE_END_ARGS()
	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );
	
	/** Handler for when cancel is clicked */
	void CancelClicked();

	/** Returns true if Finish is allowed */
	bool CanFinish() const;

	/** Handler for when finish is clicked */
	void FinishClicked();
	bool IsDirectionalLight( const AActor* const Actor ) const;
	void OnDirectionalLightSelected( AActor*  Actor ) ;
	void DirectionalLightCloseComboButton();
	void DirectionalLightOnUse();

	EVisibility GetGlobalErrorLabelVisibility() const;
	FText GetGlobalErrorLabelText() const;
	void OnSequenceAssetItemDoubleClicked( TSharedPtr<FSequenceAssetItem> TemplateItem );
	void OnSequenceAssetSelected(TSharedPtr<FSequenceAssetItem> Item, ESelectInfo::Type SelectInfo);
	void OnSequenceAssetSelected2(const FAssetData& AssetData);

	EVisibility GetTemplateSequenceVisibility() const;
	EVisibility GetAllSequenceVisibility() const;
	
	S_DECLARE_CHECKBOX(CreateTrueSkyLight)
	S_DECLARE_CHECKBOX(OverridePostProcessVolume)
	S_DECLARE_CHECKBOX(AltitudeLight)
	S_DECLARE_CHECKBOX(ShowAllSequences)
	S_DECLARE_CHECKBOX(DeleteFogs)
	S_DECLARE_CHECKBOX(DeleteBPSkySphere)

	void SetupSequenceAssetItems();
	
	void CloseContainingWindow();
private:
	/** The wizard widget */
	TSharedPtr<SWizard> MainWizard;
	/** A pointer to a class viewer **/
	TSharedPtr<class SAssetPicker> AssetPicker;
	TSharedPtr<class SSimpleActorPicker> DirectionalLightPicker;

	bool bNoExtantDirectionalLight;
	bool bNoExtantTrueSkyLight;
	
	/** Sequence Asset items */
	TSharedPtr< SListView< TSharedPtr<FSequenceAssetItem> > > SequenceAssetListView;
	TArray< TSharedPtr<FSequenceAssetItem> > SequenceAssetItemsSource;
	/** A pointer to a class viewer **/
	//TSharedPtr<class SClassViewer> ClassViewer;
	
	TSharedRef<ITableRow> MakeSequenceAssetListViewWidget(TSharedPtr<FSequenceAssetItem> ParentClassItem, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** The selected parent class */
	UTrueSkySequenceAsset *Sequence;

	ADirectionalLight* DirectionalLight;
	FOnTrueSkySetup OnTrueSkySetup;
	APostProcessVolume* PostProcessVolume;
};
