// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyEditorPluginPrivatePCH.h"

#include "TrueSkySequenceActor.h"
#include "TrueSkyLight.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "IMainFrameModule.h"
#include "IDocumentation.h"
#include "AssetToolsModule.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsWindow.h"
#include <windows.h>
#endif
#if PLATFORM_LINUX
#include "Linux/LinuxWindow.h"
#endif
#include "RendererInterface.h"
#include "UnrealClient.h"
#include "Widgets/Docking/SDockTab.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/Commands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "GPUProfiler.h"
#include "CoreMinimal.h"
#include "Internationalization/CulturePointer.h"
#include "Internationalization/Culture.h"
#include "UObject/ObjectMacros.h"
#include "HAL/PlatformFilemanager.h"
#include "Containers/StaticArray.h"
#include "ActorCrossThreadProperties.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Tickable.h"
#include "AssetTypeActions_TrueSkySequence.h"
#include "PropertyEditorModule.h"
#include "EngineModule.h"
#include "TrueSkySetupTool.h"
#include "DesktopPlatformModule.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Materials/Material.h"
#include "Components/LightComponent.h"
#include "Atmosphere/AtmosphericFog.h"
#include "Engine/ExponentialHeightFog.h"
#include <string>
#include <algorithm>
#include <wchar.h>

#include "TrueSkyEditorStyle.h"
#include "AssetTypeActions_TrueSkySequence.h"
#include "TrueSkySetupTool.h"
#include "EngineUtils.h"

#if PLATFORM_LINUX
typedef void* HWND;
typedef void* WNDPROC;
#define __stdcall
#define CALLBACK
typedef int LRESULT;
typedef long* WPARAM;
typedef long* LPARAM;
typedef unsigned int UINT;
#endif

class ADirectionalLight;

#define LOCTEXT_NAMESPACE "TrueSkyEditorPlugin"
DEFINE_LOG_CATEGORY_STATIC( TrueSkyEditor, Log, All );

using namespace simul;
using namespace unreal;


//! A simple variant for passing small data
struct Variant
{
	union
	{
		float Float;
		int32 Int;
		double Double;
		int64 Int64;
		vec3 Vec3;
	};
};
#define ENABLE_AUTO_SAVING 1

#define DECLARE_TOGGLE(name)\
	void					OnToggle##name();\
	bool					IsToggled##name();

#define IMPLEMENT_TOGGLE(name)\
	void FTrueSkyEditorPlugin::OnToggle##name()\
{\
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();\
	bool c=trueSkyPlugin.GetRenderBool(#name);\
	trueSkyPlugin.SetRenderBool(#name,!c);\
	GEditor->RedrawLevelEditingViewports();\
}\
\
bool FTrueSkyEditorPlugin::IsToggled##name()\
{\
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();\
	return trueSkyPlugin.GetRenderBool(#name);\
}

#define DECLARE_ACTION(name)\
	void					OnTrigger##name()\
	{\
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();\
		trueSkyPlugin.TriggerAction(#name);\
		GEditor->RedrawLevelEditingViewports();\
	}


class FTrueSkyTickable : public  FTickableGameObject
{
public:
	/** Tick interface */
	void					Tick( float DeltaTime );
	bool					IsTickable() const;
	TStatId					GetStatId() const;
};

class FTrueSkyEditorPlugin : public ITrueSkyEditorPlugin
{
public:
	FTrueSkyEditorPlugin();
	virtual ~FTrueSkyEditorPlugin();

	static FTrueSkyEditorPlugin*	Instance;
	
	void					OnDebugTrueSky(class UCanvas* Canvas, APlayerController*);

	/** IModuleInterface implementation */
	virtual void			StartupModule() override;
	virtual void			ShutdownModule() override;
	virtual bool			SupportsDynamicReloading() override;
	
#if UE_EDITOR
	/** TrueSKY menu */
	void					FillMenu( FMenuBuilder& MenuBuilder );
	/** Overlay sub-menu */
	void					FillOverlayMenu(FMenuBuilder& MenuBuilder);
#endif
	/** Open editor */
	virtual void			OpenEditor(UTrueSkySequenceAsset*const TrueSkySequence);

	struct SEditorInstance
	{
		/** Editor window */
		TSharedPtr<SWindow>		EditorWindow;
		/** Dockable tab */
		TSharedPtr< SDockableTab > DockableTab;
		/** Editor widow HWND */
		HWND					EditorWindowHWND;
		/** Original window message procedure */
		WNDPROC					OrigEditorWindowWndProc;
		/** Asset in editing */
		TWeakObjectPtr<UTrueSkySequenceAsset> Asset;
		/** ctor */
//		SEditorInstance() : EditorWindow(nullptr), EditorWindowHWND(0), OrigEditorWindowWndProc(nullptr), Asset(nullptr) {}
		/** Saves current sequence data into Asset */
		void					SaveSequenceData();
		/** Loads current Asset data into Environment */
		void					LoadSequenceData();
	};

	SEditorInstance*		FindEditorInstance(const TSharedRef<SWindow>& EditorWindow);
	SEditorInstance*		FindEditorInstance(HWND const EditorWindowHWND);
	SEditorInstance*		FindEditorInstance(UTrueSkySequenceAsset* const Asset);
	int						FindEditorInstance(SEditorInstance* const Instance);
	/** Saves all Environments */
	void					SaveAllEditorInstances();
	void					Tick( float DeltaTime );

    virtual void			SetUIString(SEditorInstance* const EditorInstance,const char* name, const char*  value);
	virtual void			SetCloudShadowRenderTarget(FRenderTarget *t);
	static ATrueSkySequenceActor *GetSequenceActor();
    void OnTrueSkySetup( bool , ADirectionalLight* ,bool ,UTrueSkySequenceAsset *,bool DeleteFogs,bool DeleteBPSkySphere, bool OverridePostProcessVolume,bool AltitudeLightFunction);
protected:
	
	FOnTrueSkySetup OnTrueSkySetupDelegate;
	void					OnMainWindowClosed(const TSharedRef<SWindow>& Window);

	/** Called when Toggle rendering button is pressed */
	void					OnToggleRendering();
	/** Returns true if Toggle rendering button should be enabled */
	bool					IsToggleRenderingEnabled();
	/** Returns true if Toggle rendering button should be checked */
	bool					IsToggleRenderingChecked();
	/** Always returns true */
	bool					IsMenuEnabled();

	/** Called when the Toggle Fades Overlay button is pressed*/
	DECLARE_TOGGLE(ShowFades)
	DECLARE_TOGGLE(ShowCubemaps)
	DECLARE_TOGGLE(ShowWaterTextures)
	DECLARE_TOGGLE(ShowFlowRays)
	DECLARE_TOGGLE(ShowLightningOverlay)
	DECLARE_TOGGLE(ShowCompositing)
	DECLARE_TOGGLE(ShowRainOverlay)
	DECLARE_TOGGLE(ShowCelestialDisplay)
	DECLARE_TOGGLE(OnScreenProfiling)
	DECLARE_TOGGLE(Show3DCloudTextures)
	DECLARE_TOGGLE(Show2DCloudTextures)
	void OnTriggerRecompileShaders()
	{
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
		if(!trueSkyPlugin.TriggerAction("RecompileShaders"))
		{
			FText txt=FText::FromString(TEXT("Cannot recompile trueSKY shaders"));
			FMessageDialog::Open(EAppMsgType::Ok,txt,nullptr);
		}
		GEditor->RedrawLevelEditingViewports();
	}
	DECLARE_ACTION(CycleCompositingView)
	void ShowDocumentation()
	{
		FString DocumentationUrl="http://docs.simul.co";
		FPlatformProcess::LaunchURL(*DocumentationUrl, nullptr, nullptr);
	}
	void ExportCloudLayer();
	
	/** Adds a TrueSkySequenceActor to the current scene */
	void					OnAddSequence();
	void					DeployDefaultContent();
	void					OnSequenceDestroyed(UTrueSkySequenceAsset*const DestroyedAsset);
	/** Returns true if user can add a sequence actor */
	bool					IsAddSequenceEnabled();

	/** Initializes all necessary paths */
	void					InitPaths();
	
	/** Creates new instance of UI */
	SEditorInstance*		CreateEditorInstance(  void* Env );
	TSharedPtr< FExtender > MenuExtender;
	TSharedPtr<FAssetTypeActions_TrueSkySequence> SequenceAssetTypeActions;
	
	TSharedPtr<SWindow> TrueSkySetupWindow;

	typedef void (*FOpenUI)(HWND, RECT*, RECT*, void*,PluginStyle style,const char *skin);
	typedef void (*FCloseUI)(HWND);
	
	typedef void (*FSetView)(int view_id,const float *view,const float *proj);
	typedef void (*FStaticSetUIString)(HWND,const char* name,const char* value );
	typedef int (*FStaticGetUIString)(HWND,const char* name,char* value,int max_len );

	typedef void (*FPushStyleSheetPath)(const char*);
	typedef void (*FSetSequence)(HWND, const char*,int length_hint);
	typedef char* (*FAlloc)(int size);
	typedef char* (*FGetSequence)(HWND, FAlloc);
	typedef void (*FOnTimeChangedCallback)(HWND,float);
	typedef void (*FSetOnTimeChangedInUICallback)(FOnTimeChangedCallback);
	typedef void (*FTrueSkyUILogCallback)(const char *);
	typedef void (*FSetTrueSkyUILogCallback)(FTrueSkyUILogCallback);
	typedef void (*FOnSequenceChangeCallback)(HWND,const char *);
	typedef void (*FSetOnPropertiesChangedCallback)( FOnSequenceChangeCallback);
	typedef void(__stdcall *FGenericDataCallback)(HWND, const char *, size_t, const void *);
	typedef void(*FSetGenericDataCallback)(FGenericDataCallback);

	typedef int (*FStaticInitInterface)(  );
	typedef int (*FStaticPushPath)(const char*,const char*);
	typedef int (*FStaticGetOrAddView)( void *);

	typedef int (*FStaticOnDeviceChanged)( void * device );
	typedef class UE4PluginRenderInterface* (*FStaticGetRenderInterfaceInstance)();
	typedef void (*FStaticTriggerAction)( const char* name );
	typedef void (*FStaticEnableUILogging)( const char* name );

	
	typedef bool (__stdcall *FGetColourTableCallback)(unsigned,int,int,int,float *);
	typedef void (*FSetGetColourTableCallback)(FGetColourTableCallback);
	
	typedef void (*FSetHighDPIAware)(bool);

	typedef int (*FSetRenderingInterface)(HWND ,RenderingInterface *);
	
	typedef void (__stdcall *FDeferredUpdateCallback)();
	typedef void (*FSetDeferredRenderCallback)(FDeferredUpdateCallback);
	typedef int(*FStaticGet)(HWND,const char *, int count, Variant *variant);
	typedef int(*FStaticSet)(HWND,const char *, int count, const Variant *variant);

	FOpenUI								OpenUI;
	FCloseUI							CloseUI;
	FSetView							SetView;
	FStaticSetUIString					StaticSetUIString;
	FStaticGetUIString					StaticGetUIString;
	FPushStyleSheetPath					PushStyleSheetPath;
	FSetSequence						SetSequence;
	FGetSequence						GetSequence;
	FSetOnPropertiesChangedCallback		SetOnPropertiesChangedCallback;
	FSetGenericDataCallback				SetGenericDataCallback;
	FSetOnTimeChangedInUICallback		SetOnTimeChangedInUICallback;
	FStaticInitInterface				StaticInitInterface;
	FStaticPushPath						StaticPushPath;
	FStaticGetOrAddView					StaticGetOrAddView;
	FStaticOnDeviceChanged				StaticOnDeviceChanged;
	FStaticTriggerAction				StaticTriggerAction;
	FStaticEnableUILogging				StaticEnableUILogging;
	FSetTrueSkyUILogCallback			SetTrueSkyUILogCallback;
	FSetGetColourTableCallback			SetGetColourTableCallback;
	FSetHighDPIAware					SetHighDPIAware;
	FSetRenderingInterface				SetRenderingInterface;
	FSetDeferredRenderCallback			SetDeferredRenderCallback;


	FStaticGet							StaticGet;
	FStaticSet							StaticSet;
	const TCHAR*						PathEnv;

	bool					RenderingEnabled;
	bool					RendererInitialized;

	float					CachedDeltaSeconds;
	float					AutoSaveTimer;		// 0.0f == no auto-saving
	

	TArray<SEditorInstance>	EditorInstances;

	static LRESULT CALLBACK EditorWindowWndProc(HWND, unsigned int, WPARAM, LPARAM);
	static unsigned int MessageId;

	static void				OnSequenceChangeCallback(HWND OwnerHWND,const char *);
	static void				GenericDataCallback(HWND OwnerHWND, const char *, size_t, const void *);
	static void				TrueSkyUILogCallback(const char *);
	static void				OnTimeChangedCallback(HWND OwnerHWND,float t);
	static bool				GetColourTableCallback(unsigned,int,int,int,float *target);
	static void				RetrieveColourTables();
	static void				DeferredUpdateCallback();
	FRenderTarget			*cloudShadowRenderTarget;

	bool					haveEditor;
};

IMPLEMENT_MODULE( FTrueSkyEditorPlugin, TrueSkyEditorPlugin )

#if UE_EDITOR
class FTrueSkyCommands : public TCommands<FTrueSkyCommands>
{
public:
	//	TCommands( const FName InContextName, const FText& InContextDesc, const FName InContextParent, const FName InStyleSetName )
	FTrueSkyCommands()
		: TCommands<FTrueSkyCommands>(
		TEXT("TrueSky"), // Context name for fast lookup
		NSLOCTEXT("Contexts", "TrueSkyCmd", "Simul TrueSky"), // Localized context name for displaying
		NAME_None, // Parent context name. 
		FEditorStyle::GetStyleSetName()) // Parent
	{
	}
	virtual void RegisterCommands() override
	{
		UI_COMMAND(AddSequence					,"Initialize trueSKY..."		,"Launches a wizard to setup trueSKY in the current level.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(ToggleShowCelestialDisplay	,"Celestial Display"			,"Toggles the celestial display.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleOnScreenProfiling		,"Profiling"					,"Toggles the Profiling display.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShowFades				,"Atmospheric Tables"			,"Toggles the atmospheric tables overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShowCubemaps			,"Cubemaps"						,"Toggles the cubemaps overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShowWaterTextures		,"Water Textures"				,"Toggles the water textures overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShowFlowRays			,"Flow Rays"					,"Toggles the Flow Rays debuf visualiser.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShowLightningOverlay	,"Lightning"					,"Toggles the lightning overlay.", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Alt, EKeys::L));
		UI_COMMAND(ToggleShowCompositing		,"Compositing"					,"Toggles the compositing overlay.", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Alt, EKeys::O));
		UI_COMMAND(ToggleShowRainOverlay		,"Rain Overlay"					,"Toggles the rain overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(TriggerCycleCompositingView	,"Cycle Compositing View"		,"Changes which view is shown on the compositing overlay.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(ToggleShow3DCloudTextures	,"Show 3D Cloud Textures"		,"Toggles the 3D cloud overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ToggleShow2DCloudTextures	,"Show 2D Cloud Textures"		,"Toggles the 2D cloud overlay.", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(TriggerRecompileShaders		,"Recompile Shaders"			,"Recompiles the shaders."		, EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::R));
		UI_COMMAND(TriggerShowDocumentation		,"trueSKY Documentation..."		,"Shows the trueSKY help pages.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(TriggerExportCloudLayer		,"Export Sky As .FBX"			,"Exports the current sky as a .FBX file.", EUserInterfaceActionType::Button, FInputGesture());
	}
public:
	TSharedPtr<FUICommandInfo> AddSequence;
	TSharedPtr<FUICommandInfo> ToggleShowFades;
	TSharedPtr<FUICommandInfo> ToggleShowCubemaps;
	TSharedPtr<FUICommandInfo> ToggleShowWaterTextures;
	TSharedPtr<FUICommandInfo> ToggleShowFlowRays;
	TSharedPtr<FUICommandInfo> ToggleShowLightningOverlay;
	TSharedPtr<FUICommandInfo> ToggleOnScreenProfiling;
	TSharedPtr<FUICommandInfo> ToggleShowCelestialDisplay;
	TSharedPtr<FUICommandInfo> ToggleShowCompositing;
	TSharedPtr<FUICommandInfo> ToggleShowRainOverlay;
	TSharedPtr<FUICommandInfo> ToggleShow3DCloudTextures;
	TSharedPtr<FUICommandInfo> ToggleShow2DCloudTextures;
	TSharedPtr<FUICommandInfo> TriggerRecompileShaders;
	TSharedPtr<FUICommandInfo> TriggerShowDocumentation;
	TSharedPtr<FUICommandInfo> TriggerExportCloudLayer;
	TSharedPtr<FUICommandInfo> TriggerCycleCompositingView;
};
#endif
FTrueSkyEditorPlugin* FTrueSkyEditorPlugin::Instance = nullptr;

//TSharedRef<FTrueSkyEditorPlugin> staticSharedRef;
static std::string trueSkyPluginPath="../../Plugins/TrueSkyPlugin";
FTrueSkyEditorPlugin::FTrueSkyEditorPlugin()
	:PathEnv(nullptr)
	, cloudShadowRenderTarget(nullptr)
	, haveEditor(false)
{
	InitPaths();
	DeployDefaultContent();
	Instance = this;
	EditorInstances.Reset();
	AutoSaveTimer = 0.0f;
	//we need to pass through real DeltaSecond; from our scene Actor?
	CachedDeltaSeconds = 0.0333f;
}

FTrueSkyEditorPlugin::~FTrueSkyEditorPlugin()
{
	Instance = nullptr;
}


bool FTrueSkyEditorPlugin::SupportsDynamicReloading()
{
	return false;
}

void FTrueSkyEditorPlugin::Tick( float DeltaTime )
{
	if(IsInGameThread())
	{

	}
	CachedDeltaSeconds = DeltaTime;
#ifdef ENABLE_AUTO_SAVING
	if ( AutoSaveTimer > 0.0f )
	{
		if ( (AutoSaveTimer -= DeltaTime) <= 0.0f )
		{
			SaveAllEditorInstances();
			AutoSaveTimer = 4.0f;
		}
	}
#endif
}

void FTrueSkyEditorPlugin::SetCloudShadowRenderTarget(FRenderTarget *t)
{
	cloudShadowRenderTarget=t;
}

void FTrueSkyEditorPlugin::SetUIString(SEditorInstance* const EditorInstance,const char* name, const char*  value)
{
	if( StaticSetUIString != nullptr&&EditorInstance!=nullptr)
	{
		StaticSetUIString(EditorInstance->EditorWindowHWND,name, value );
	}
	else
	{
		UE_LOG(TrueSkyEditor, Warning, TEXT("Trying to set UI string before StaticSetUIString has been set"), TEXT(""));
	}
}

/** Tickable object interface */
void FTrueSkyTickable::Tick( float DeltaTime )
{
	if(FTrueSkyEditorPlugin::Instance)
		FTrueSkyEditorPlugin::Instance->Tick(DeltaTime);
}

bool FTrueSkyTickable::IsTickable() const
{
	return true;
}

TStatId FTrueSkyTickable::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTrueSkyTickable, STATGROUP_Tickables);
}

#define MAP_TOGGLE(U)	CommandList->MapAction( FTrueSkyCommands::Get().Toggle##U,\
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggle##U),\
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),\
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggled##U)\
									);

void FTrueSkyEditorPlugin::StartupModule()
{
#if UE_EDITOR
	FTrueSkyEditorStyle::Get( );

	FTrueSkyCommands::Register();
	if(FModuleManager::Get().IsModuleLoaded("MainFrame") )
	{
		haveEditor=true;
		IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
		const TSharedRef<FUICommandList>& CommandList = MainFrameModule.GetMainFrameCommandBindings();
		CommandList->MapAction( FTrueSkyCommands::Get().AddSequence,
								FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnAddSequence),
								FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsAddSequenceEnabled)
								);
			CommandList->MapAction( FTrueSkyCommands::Get().TriggerRecompileShaders,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnTriggerRecompileShaders),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().TriggerShowDocumentation,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::ShowDocumentation)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().TriggerExportCloudLayer,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::ExportCloudLayer)
									);
		{
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleOnScreenProfiling,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleOnScreenProfiling),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledOnScreenProfiling)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleShowCelestialDisplay,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShowCelestialDisplay),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShowCelestialDisplay)
									);
			MAP_TOGGLE(ShowFades)
			MAP_TOGGLE(ShowCubemaps)
			MAP_TOGGLE(ShowLightningOverlay)
			CommandList->MapAction(	FTrueSkyCommands::Get().ToggleShowWaterTextures,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShowWaterTextures),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShowWaterTextures)
			);
			CommandList->MapAction(FTrueSkyCommands::Get().ToggleShowFlowRays,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShowFlowRays),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShowFlowRays)
			);
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleShowCompositing,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShowCompositing),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShowCompositing)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleShowRainOverlay,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShowRainOverlay),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShowRainOverlay)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().TriggerCycleCompositingView,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnTriggerCycleCompositingView),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleShow3DCloudTextures,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShow3DCloudTextures),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShow3DCloudTextures)
									);
			CommandList->MapAction( FTrueSkyCommands::Get().ToggleShow2DCloudTextures,
									FExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::OnToggleShow2DCloudTextures),
									FCanExecuteAction::CreateRaw(this, &FTrueSkyEditorPlugin::IsMenuEnabled),
									FIsActionChecked::CreateRaw(this, &FTrueSkyEditorPlugin::IsToggledShow2DCloudTextures)
									);
		}
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuExtension("WindowGlobalTabSpawners", EExtensionHook::After, CommandList, FMenuExtensionDelegate::CreateRaw(this, &FTrueSkyEditorPlugin::FillMenu));
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

		SequenceAssetTypeActions = MakeShareable(new FAssetTypeActions_TrueSkySequence);
		FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().RegisterAssetTypeActions(SequenceAssetTypeActions.ToSharedRef());
	}
#endif
	
	const FName IconName(TEXT("../../Plugins/TrueSkyPlugin/Resources/icon_64x.png"));

	OpenUI							=nullptr;
	CloseUI							=nullptr;
	SetView							=nullptr;
	PushStyleSheetPath				=nullptr;
	SetSequence						=nullptr;
	GetSequence						=nullptr;
	SetOnPropertiesChangedCallback	=nullptr;
	SetOnTimeChangedInUICallback	=nullptr;
	SetTrueSkyUILogCallback			=nullptr;
	SetGetColourTableCallback		=nullptr;
	SetHighDPIAware					=nullptr;
	SetRenderingInterface			=nullptr;
	SetDeferredRenderCallback		=nullptr;

	StaticGet						= nullptr;
	StaticSet						= nullptr;

	RenderingEnabled				=false;
	RendererInitialized				=false;
	StaticInitInterface				=nullptr;
	StaticPushPath					=nullptr;
	StaticOnDeviceChanged			=nullptr;

	StaticTriggerAction				=nullptr;
	StaticEnableUILogging			=nullptr;
	PathEnv							= nullptr;
#if PLATFORM_WINDOWS
	MessageId = RegisterWindowMessage(L"RESIZE");
#endif
}

void FTrueSkyEditorPlugin::OnDebugTrueSky(class UCanvas* Canvas, APlayerController*)
{
	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(false, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->DrawText(RenderFont, FString("trueSKY Debug Display"), 0.3f, 0.3f, 1.f, 1.f, FontRenderInfo);
	/*
		
	float res=Canvas->DrawText
	(
    UFont * InFont,
    const FString & InText,
    float X,
    float Y,
    float XScale,
    float YScale,
    const FFontRenderInfo & RenderInfo
)
		*/
	Canvas->SetDrawColor(OldDrawColor);
}

void FTrueSkyEditorPlugin::ShutdownModule()
{
#if !UE_BUILD_SHIPPING && WITH_EDITOR
	// Unregister for debug drawing
	//UDebugDrawService::Unregister(FDebugDrawDelegate::CreateUObject(this, &FTrueSkyEditorPlugin::OnDebugTrueSky));
#endif
	if(SetTrueSkyUILogCallback)
		SetTrueSkyUILogCallback(nullptr);
	if(SetGetColourTableCallback)
		SetGetColourTableCallback(nullptr);
	if(SetDeferredRenderCallback)
		SetDeferredRenderCallback(nullptr);
#if UE_EDITOR
	FTrueSkyEditorStyle::Destroy( );

	FTrueSkyCommands::Unregister();
	if ( FModuleManager::Get().IsModuleLoaded("LevelEditor") )
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender( MenuExtender );
	}
	if ( FModuleManager::Get().IsModuleLoaded("AssetTools") )
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().UnregisterAssetTypeActions(SequenceAssetTypeActions.ToSharedRef());
	}
#endif
	delete PathEnv;
	PathEnv = nullptr;
}


#if UE_EDITOR
void FTrueSkyEditorPlugin::FillMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection( "TrueSky", FText::FromString(TEXT("TrueSky")) );
	{		
		MenuBuilder.AddMenuEntry( FTrueSkyCommands::Get().AddSequence );
		MenuBuilder.AddMenuEntry( FTrueSkyCommands::Get().TriggerRecompileShaders);
		MenuBuilder.AddMenuEntry( FTrueSkyCommands::Get().TriggerShowDocumentation);
		//Exporting to FBX currently doesn't work, and it is hard to determine when it was broken.
		//MenuBuilder.AddMenuEntry( FTrueSkyCommands::Get().TriggerExportCloudLayer);
		try
		{
			FNewMenuDelegate d;
			MenuBuilder.AddSubMenu(FText::FromString("Overlays"),FText::FromString("TrueSKY overlays"),FNewMenuDelegate::CreateRaw(this,&FTrueSkyEditorPlugin::FillOverlayMenu ));
		}
		catch(...)
		{
			UE_LOG(TrueSkyEditor, Warning, TEXT("Failed to add trueSKY submenu"), TEXT(""));
		}
	}
	MenuBuilder.EndSection();
	
}
	
void FTrueSkyEditorPlugin::FillOverlayMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowCelestialDisplay);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleOnScreenProfiling);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowFades);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowCubemaps);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowWaterTextures);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowFlowRays);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowLightningOverlay);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowCompositing);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShowRainOverlay);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().TriggerCycleCompositingView);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShow3DCloudTextures);
	MenuBuilder.AddMenuEntry(FTrueSkyCommands::Get().ToggleShow2DCloudTextures);
}
#endif
/** Returns environment variable value */
static FString GetEnvVariable( const wchar_t* const VariableName)
{
	FString fstr = FPlatformMisc::GetEnvironmentVariable((const TCHAR*)VariableName);
	return fstr;
}

/** Returns HWND for a given SWindow (if native!) */

static HWND GetSWindowHWND(const TSharedPtr<SWindow>& Window)
{
	if ( Window.IsValid() )
	{
#if PLATFORM_WINDOWS
        TSharedPtr<FWindowsWindow> window = StaticCastSharedPtr<FWindowsWindow>(Window->GetNativeWindow());
        if ( window.IsValid() )
		{
            return (HWND)window->GetOSWindowHandle();
		}
#endif
#if PLATFORM_LINUX
        TSharedPtr<FLinuxWindow> window = StaticCastSharedPtr<FLinuxWindow>(Window->GetNativeWindow());
        if ( window.IsValid() )
        {
            return (HWND)window->GetOSWindowHandle();
        }
#endif
	}
	return 0;
}

static std::wstring Utf8ToWString(const char *src_utf8)
{
	int src_length=(int)strlen(src_utf8);
#ifdef _MSC_VER
	int length = MultiByteToWideChar(CP_UTF8, 0, src_utf8,src_length, 0, 0);
#else
	int length=src_length;
#endif
	wchar_t *output_buffer = new wchar_t [length+1];
#ifdef _MSC_VER
	MultiByteToWideChar(CP_UTF8, 0, src_utf8, src_length, output_buffer, length);
#else
	mbstowcs(output_buffer, src_utf8, (size_t)length );
#endif
	output_buffer[length]=0;
	std::wstring wstr=std::wstring(output_buffer);
	delete [] output_buffer;
	return wstr;
}

static std::string WStringToUtf8(const wchar_t *src_w)
{
	int src_length=(int)wcslen(src_w);
#ifdef _MSC_VER
	int size_needed = WideCharToMultiByte(CP_UTF8, 0,src_w, (int)src_length, nullptr, 0, nullptr, nullptr);
#else
	int size_needed=2*src_length;
#endif
	char *output_buffer = new char [size_needed+1];
#ifdef _MSC_VER
	WideCharToMultiByte (CP_UTF8,0,src_w,(int)src_length,output_buffer, size_needed, nullptr, nullptr);
#else
	wcstombs(output_buffer, src_w, (size_t)size_needed );
#endif
	output_buffer[size_needed]=0;
	std::string str_utf8=std::string(output_buffer);
	delete [] output_buffer;
	return str_utf8;
}
static std::string FStringToUtf8(const FString &Source)
{
	const wchar_t *src_w = (const wchar_t*)(Source.GetCharArray().GetData());
	int src_length = (int)wcslen(src_w);
#ifdef _MSC_VER
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, src_w, (int)src_length, nullptr, 0, nullptr, nullptr);
#else
	int size_needed = 2 * src_length;
#endif
	char *output_buffer = new char[size_needed + 1];
#ifdef _MSC_VER
	WideCharToMultiByte(CP_UTF8, 0, src_w, (int)src_length, output_buffer, size_needed, nullptr, nullptr);
#else
	wcstombs(output_buffer, src_w, (size_t)size_needed);
#endif
	output_buffer[size_needed] = 0;
	std::string str_utf8 = std::string(output_buffer);
	delete[] output_buffer;
	return str_utf8;
}
static std::string UE4ToSimulLanguageName(const FString &Source)
{
	std::string ue4_name = FStringToUtf8(Source);
	if (ue4_name.find("English") < ue4_name.size())
		return "en";
	if (ue4_name.find("Japanese") < ue4_name.size())
		return "ja";
	if (ue4_name.find("French") < ue4_name.size())
		return "fr";
	if (ue4_name.find("German") < ue4_name.size())
		return "de";
	if (ue4_name.find("Korean") < ue4_name.size())
		return "ko";
	return "";
}

bool CheckDllFunction(void *fn,FString &str,const char *fnName)
{
	bool res=(fn!=nullptr);
	if(!res)
	{
		if(!str.IsEmpty())
			str+=", ";
		str+=fnName;
	}
	return res;
}

typedef void* moduleHandle;

static moduleHandle GetDllHandle( const TCHAR* Filename )
{
	check(Filename);
	return FPlatformProcess::GetDllHandle(Filename);
}

#define GET_FUNCTION(fnName) fnName= (F##fnName)FPlatformProcess::GetDllExport(DllHandle, TEXT(#fnName) );
#define MISSING_FUNCTION(fnName) (!CheckDllFunction((void*)fnName,failed_functions, #fnName))

#define warnf(expr, ...)				{ if(!(expr)) FDebug::AssertFailed( #expr, __FILE__, __LINE__, ##__VA_ARGS__ ); CA_ASSUME(expr); }
FTrueSkyEditorPlugin::SEditorInstance* FTrueSkyEditorPlugin::CreateEditorInstance(   void* Env )
{
	static bool failed_once = false;
	FString DllPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Simul/Win64")));
	FString DllFilename(FPaths::Combine(*DllPath, TEXT("TrueSkyUI_MD.dll")));
#ifdef DEBUG // UE_BUILD_DEBUG //doesn't work... why?
	DllFilename=FPaths::Combine(*DllPath, TEXT("TrueSkyUI_MDd.dll"));
#endif
	if( !FPaths::FileExists(DllFilename) )
	{
		if (!failed_once)
			UE_LOG(TrueSkyEditor, Warning, TEXT("TrueSkyUI_MD DLL not found."));
		failed_once = true;
		return nullptr;
	}
//	SetErrorMode(SEM_FAILCRITICALERRORS);
	void* DllHandle = FPlatformProcess::GetDllHandle((const TCHAR*)DllFilename.GetCharArray().GetData() );
	if(DllHandle==nullptr)
	{
		DllHandle=GetDllHandle((const TCHAR*)DllFilename.GetCharArray().GetData());
		if(DllHandle==nullptr)
		{
			if (!failed_once)
				UE_LOG(TrueSkyEditor, Warning, TEXT("Failed to load %s for truesky ui."), (const TCHAR*)DllFilename.GetCharArray().GetData());
			failed_once = true;
			InitPaths();
			return nullptr;
		}
	}
	else
	{
		GET_FUNCTION(OpenUI);
		GET_FUNCTION(CloseUI);

		GET_FUNCTION(SetView);
		GET_FUNCTION(StaticSetUIString);
		GET_FUNCTION(StaticGetUIString);// optional for now.
		
		PushStyleSheetPath = (FPushStyleSheetPath)FPlatformProcess::GetDllExport(DllHandle, TEXT("PushStyleSheetPath") );
		SetSequence = (FSetSequence)FPlatformProcess::GetDllExport(DllHandle, TEXT("StaticSetSequence") );
		GetSequence = (FGetSequence)FPlatformProcess::GetDllExport(DllHandle, TEXT("StaticGetSequence") );
		SetOnPropertiesChangedCallback = (FSetOnPropertiesChangedCallback)FPlatformProcess::GetDllExport(DllHandle, TEXT("SetOnPropertiesChangedCallback") );
		GET_FUNCTION(SetGenericDataCallback);
		SetOnTimeChangedInUICallback = (FSetOnTimeChangedInUICallback)FPlatformProcess::GetDllExport(DllHandle, TEXT("SetOnTimeChangedCallback") );
		GET_FUNCTION(SetTrueSkyUILogCallback);
		GET_FUNCTION(StaticEnableUILogging);
		GET_FUNCTION(SetGetColourTableCallback);
		GET_FUNCTION(SetHighDPIAware);		// optional
		GET_FUNCTION(SetRenderingInterface);	// optional
		GET_FUNCTION(SetDeferredRenderCallback);	// optional
		
		GET_FUNCTION(StaticGet);	// optional
		GET_FUNCTION(StaticSet);	// optional
		FString failed_functions;
		int num_fails=MISSING_FUNCTION(OpenUI) +MISSING_FUNCTION(CloseUI)
			+MISSING_FUNCTION(StaticSetUIString)
			+MISSING_FUNCTION(PushStyleSheetPath)
			+MISSING_FUNCTION(SetSequence)
			+MISSING_FUNCTION(GetSequence)
			+MISSING_FUNCTION(SetOnPropertiesChangedCallback)
			+MISSING_FUNCTION(SetOnTimeChangedInUICallback)
			+MISSING_FUNCTION(SetTrueSkyUILogCallback)
			+MISSING_FUNCTION(SetGetColourTableCallback);
		if(num_fails>0)
		{
			static bool reported=false;
			if(!reported)
			{
				UE_LOG(TrueSkyEditor, Error
					,TEXT("Can't initialize the trueSKY UI plugin dll because %d functions were not found - please update TrueSkyUI_MD.dll or TrueSkyEditorPlugin.cpp.\nThe missing functions are %s.")
					,num_fails
					,*failed_functions
					);
				reported=true;
			}
			return nullptr;
		}
		checkf( OpenUI, TEXT("OpenUI function not found!" ));
		checkf( CloseUI, TEXT("CloseUI function not found!" ));
		checkf( PushStyleSheetPath, TEXT("PushStyleSheetPath function not found!" ));
		checkf( SetSequence, TEXT("SetSequence function not found!" ));
		checkf( GetSequence, TEXT("GetSequence function not found!" ));
		checkf( SetOnPropertiesChangedCallback, TEXT("SetOnPropertiesChangedCallback function not found!" ));
		checkf( SetOnTimeChangedInUICallback, TEXT("SetOnTimeChangedInUICallback function not found!" ));
		checkf( SetTrueSkyUILogCallback, TEXT("SetTrueSkyUILogCallback function not found!" ));
		
		checkf( StaticSetUIString, TEXT("StaticSetUIString function not found!" ));

		PushStyleSheetPath((trueSkyPluginPath+"\\Resources\\qss\\").c_str());

		IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
		TSharedPtr<SWindow> ParentWindow = MainFrameModule.GetParentWindow();
		if ( ParentWindow.IsValid() )
		{
			SEditorInstance EditorInstance;
			memset(&EditorInstance, 0, sizeof(EditorInstance));

			TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);

			EditorInstance.EditorWindow = SNew(SWindow)
				.Title( FText::FromString(TEXT("TrueSky")) )
				.ClientSize( FVector2D(800.0f, 600.0f) )
				.AutoCenter( EAutoCenter::PrimaryWorkArea )
				.SizingRule( ESizingRule::UserSized )
				 .AdjustInitialSizeAndPositionForDPIScale(false)
				;
			EditorInstance.EditorWindow->SetOnWindowClosed( FOnWindowClosed::CreateRaw(this, &FTrueSkyEditorPlugin::OnMainWindowClosed) );
			FSlateApplication::Get().AddWindowAsNativeChild( EditorInstance.EditorWindow.ToSharedRef(), ParentWindow.ToSharedRef() );
		//	EditorInstance.EditorWindow->ReshapeWindow( EditorInstance.EditorWindow->GetPositionInScreen(), FVector2D(800.0f, 600.0f) );

			EditorInstance.EditorWindowHWND = GetSWindowHWND(EditorInstance.EditorWindow);
			if ( EditorInstance.EditorWindowHWND )
			{
#if PLATFORM_WINDOWS
				const LONG_PTR wndStyle = GetWindowLongPtr( EditorInstance.EditorWindowHWND, GWL_STYLE );
				SetWindowLongPtr( EditorInstance.EditorWindowHWND, GWL_STYLE, wndStyle | WS_CLIPCHILDREN );
#endif
				const FVector2D ClientSize = EditorInstance.EditorWindow->GetClientSizeInScreen();
				const FMargin Margin = EditorInstance.EditorWindow->GetWindowBorderSize();
			//	GetWindowRect(EditorInstance.EditorWindowHWND, &ParentRect);
				EditorInstance.EditorWindow->Restore();
				FSlateRect winRect=EditorInstance.EditorWindow->GetClientRectInScreen();
				RECT ParentRect;
				ParentRect.left = 0;
				ParentRect.top = 0;
				ParentRect.right = winRect.Right-winRect.Left;
				ParentRect.bottom = winRect.Bottom-winRect.Top;
				RECT ClientRect;
				ClientRect.left = Margin.Left;
				ClientRect.top = Margin.Top + 2*EditorInstance.EditorWindow->GetTitleBarSize().Get();
				ClientRect.right = ClientSize.X - Margin.Right;
				ClientRect.bottom = ClientSize.Y - Margin.Bottom;
				
				// Let's do this FIRST in case of errors in OpenUI!
				SetTrueSkyUILogCallback( TrueSkyUILogCallback );
				static bool hdpi_aware=false;
				// Disable the Sequencer's High-dpi awareness because it conflicts with UE 4's.
				if(SetHighDPIAware)
					SetHighDPIAware(hdpi_aware);

				OpenUI( EditorInstance.EditorWindowHWND, &ClientRect, &ParentRect, Env, UNREAL_STYLE,"ue4");
				ATrueSkySequenceActor* SequenceActor = GetSequenceActor();
				if (SequenceActor&&StaticSet)
				{
					Variant v;
					v.Vec3 = { SequenceActor->OriginLatitude, SequenceActor->OriginLongitude, 0.0f };
					StaticSet(EditorInstance.EditorWindowHWND, "originlatlongheadingdeg", 1, &v);
				}

				FInternationalization& I18N = FInternationalization::Get();
				FCultureRef culture = I18N.GetCurrentCulture();
				FString cult_name=culture->GetEnglishName();
				StaticSetUIString(EditorInstance.EditorWindowHWND, "Language", UE4ToSimulLanguageName(cult_name).c_str());

				// Overload main window's WndProc
#if PLATFORM_WINDOWS
				EditorInstance.OrigEditorWindowWndProc = (WNDPROC)GetWindowLongPtr( EditorInstance.EditorWindowHWND, GWLP_WNDPROC );
				SetWindowLongPtr( EditorInstance.EditorWindowHWND, GWLP_WNDPROC, (LONG_PTR)EditorWindowWndProc );
#endif
				// Setup notification callback
				SetOnPropertiesChangedCallback(  OnSequenceChangeCallback );
				if (SetGenericDataCallback)
					SetGenericDataCallback(GenericDataCallback);
				SetGetColourTableCallback(GetColourTableCallback);
				if(SetDeferredRenderCallback)
					SetDeferredRenderCallback(DeferredUpdateCallback);
				SetOnTimeChangedInUICallback(OnTimeChangedCallback);
				EditorInstance.EditorWindow->Restore();
				ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
				if(SetRenderingInterface)
					SetRenderingInterface(EditorInstance.EditorWindowHWND,trueSkyPlugin.GetRenderingInterface());
				return &EditorInstances[ EditorInstances.Add(EditorInstance) ];
			}
		}
	}

	return nullptr;
}


FTrueSkyEditorPlugin::SEditorInstance* FTrueSkyEditorPlugin::FindEditorInstance(const TSharedRef<SWindow>& EditorWindow)
{
	for (int i = 0; i < EditorInstances.Num(); ++i)
	{
		if ( EditorInstances[i].EditorWindow.ToSharedRef() == EditorWindow )
		{
			return &EditorInstances[i];
		}
	}
	return nullptr;
}

FTrueSkyEditorPlugin::SEditorInstance* FTrueSkyEditorPlugin::FindEditorInstance(HWND const EditorWindowHWND)
{
	for (int i = 0; i < EditorInstances.Num(); ++i)
	{
		if ( EditorInstances[i].EditorWindowHWND == EditorWindowHWND )
		{
			return &EditorInstances[i];
		}
	}
	return nullptr;
}

FTrueSkyEditorPlugin::SEditorInstance* FTrueSkyEditorPlugin::FindEditorInstance(UTrueSkySequenceAsset* const Asset)
{
	for (int i = 0; i < EditorInstances.Num(); ++i)
	{
		if ( EditorInstances[i].Asset == Asset )
		{
			return &EditorInstances[i];
		}
	}
	return nullptr;
}

int FTrueSkyEditorPlugin::FindEditorInstance(FTrueSkyEditorPlugin::SEditorInstance* const EditorInstance)
{
	for (int i = 0; i < EditorInstances.Num(); ++i)
	{
		if ( &EditorInstances[i] == EditorInstance )
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void FTrueSkyEditorPlugin::SaveAllEditorInstances()
{
	for (int i = 0; i < EditorInstances.Num(); ++i)
	{
		EditorInstances[i].SaveSequenceData();
	}
}

void FTrueSkyEditorPlugin::SEditorInstance::SaveSequenceData()
{
	if ( Asset.IsValid()&&Asset->IsValidLowLevel())
	{
		struct Local
		{
			static char* AllocString(int size)
			{
				check( size > 0 );
				return new char[size];
			}
		};
		check( FTrueSkyEditorPlugin::Instance );
		check( FTrueSkyEditorPlugin::Instance->GetSequence );
		if ( char* const OutputText = FTrueSkyEditorPlugin::Instance->GetSequence(EditorWindowHWND, Local::AllocString) )
		{
			if ( Asset->SequenceText.Num() > 0 )
			{
				if ( strcmp(OutputText, (const char*)Asset->SequenceText.GetData()) == 0 )
				{
					// No change -> quit
					delete OutputText;
					return;
				}
			}

			const int OutputTextLen = strlen( OutputText );
			Asset->SequenceText.Reset( OutputTextLen + 1 );

			for (char* ptr = OutputText; *ptr; ++ptr)
			{
				Asset->SequenceText.Add( *ptr );
			}
			Asset->SequenceText.Add( 0 );

			// Mark as dirty
			Asset->Modify( true );
			
			delete OutputText;
		}
		if(FTrueSkyEditorPlugin::Instance->StaticGetUIString)
		{
			int len=0;
			int actual_length=FTrueSkyEditorPlugin::Instance->StaticGetUIString(EditorWindowHWND,"Description",nullptr,0);
			if(actual_length>0)
			{
				actual_length=std::max(actual_length,1000);
				char *str=new char[actual_length+2];
				FTrueSkyEditorPlugin::Instance->StaticGetUIString(EditorWindowHWND,"Description",str,actual_length+1);
				Asset->Description=str;
				delete [] str;
			}
		}
	}
	else
	{
	}
}

void FTrueSkyEditorPlugin::SEditorInstance::LoadSequenceData()
{
	if ( Asset.IsValid()&&Asset->IsValidLowLevel() && Asset->SequenceText.Num() > 0 )
	{
		check( FTrueSkyEditorPlugin::Instance );
		check( FTrueSkyEditorPlugin::Instance->SetSequence );
		FTrueSkyEditorPlugin::Instance->SetSequence( EditorWindowHWND, (const char*)Asset->SequenceText.GetData(),Asset->SequenceText.Num());
	}
	else
	{
	}
}


void FTrueSkyEditorPlugin::OnMainWindowClosed(const TSharedRef<SWindow>& Window)
{
	if ( SEditorInstance* const EditorInstance = FindEditorInstance(Window) )
	{
		EditorInstance->SaveSequenceData();

		check( CloseUI );
		CloseUI( EditorInstance->EditorWindowHWND );

		EditorInstance->EditorWindow = nullptr;
		EditorInstances.RemoveAt( FindEditorInstance(EditorInstance) );
	}
}


/** Called when TrueSkyUI properties have changed */
void FTrueSkyEditorPlugin::OnSequenceChangeCallback(HWND OwnerHWND,const char *txt)
{
	check( Instance );
	if ( SEditorInstance* const EditorInstance = Instance->FindEditorInstance(OwnerHWND) )
	{
		if (EditorInstance->Asset.IsValid()&&EditorInstance->Asset->IsValidLowLevel())
			EditorInstance->SaveSequenceData();
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
		if (trueSkyPlugin.GetActiveSequence() == EditorInstance->Asset)
		{
			trueSkyPlugin.OnUIChangedSequence();
		}
	}
}

/** Called when TrueSkyUI properties have changed */
void FTrueSkyEditorPlugin::GenericDataCallback(HWND OwnerHWND, const char *txt, size_t sz, const void *data)
{
	check(Instance);
	SEditorInstance* const EditorInstance = Instance->FindEditorInstance(OwnerHWND);
	if(!EditorInstance)
		return;
	ITrueSkyPlugin &trueSkyPlugin = ITrueSkyPlugin::Get();
	if (trueSkyPlugin.GetActiveSequence() == EditorInstance->Asset)
	{
		if (FString(txt) == "CloudWindow:OriginLatLongHeadingDeg" && sz==sizeof(FVector))
		{
			TArray<FVariant> vals;
			const float *fval = (const float*)data;
			ATrueSkySequenceActor* SequenceActor = GetSequenceActor();
			if (SequenceActor)
			{
				SequenceActor->OriginLatitude = fval[0];
				SequenceActor->OriginLongitude= fval[1];
				SequenceActor->OriginHeading = fval[2];
				SequenceActor->initializeDefaults=true;
			}
		}
	}
}

void FTrueSkyEditorPlugin::TrueSkyUILogCallback(const char *txt)
{
	if(!Instance )
		return;
	static FString fstr;
	fstr+=txt;
	int max_len=0;
	for(int i=0;i<fstr.Len();i++)
	{
		if(fstr[i]==L'\n'||i>1000)
		{
			fstr[i]=L' ';
			max_len=i+1;
			break;
		}
	}
	if(max_len==0)
		return;
	FString substr=fstr.Left(max_len);
	fstr=fstr.RightChop(max_len);
	if(substr.Contains("error"))
	{
		UE_LOG(TrueSkyEditor,Error,TEXT("%s"), *substr);
	}
	else if(substr.Contains("warning"))
	{
		UE_LOG(TrueSkyEditor,Warning,TEXT("%s"), *substr);
	}
	else
	{
		UE_LOG(TrueSkyEditor,Display,TEXT("%s"), *substr);
	}
}

static void editorCallback()
{
	//FTrueSkyEditorPlugin::Instance->RetrieveColourTables();
}

void FTrueSkyEditorPlugin::RetrieveColourTables()
{
}

bool FTrueSkyEditorPlugin::GetColourTableCallback(unsigned kf_uid,int x,int y,int z,float *target)
{
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
	trueSkyPlugin.RequestColourTable(kf_uid,x,y,z);
	if(target)
	{
		const ITrueSkyPlugin::ColourTableRequest *req=nullptr;

		int count=0;

		while((req=trueSkyPlugin.GetColourTable(kf_uid))==nullptr&&count<10)
		{
#if PLATFORM_WINDOWS
			Sleep(10);
#else
			sleep(10);
#endif
			count++;
		}

		if(req&&req->valid&&req->data)
		{
			memcpy(target,req->data,x*y*z*4*sizeof(float));
			trueSkyPlugin.ClearColourTableRequests();
			return true;
		}
	}
	return false;
}

void FTrueSkyEditorPlugin::DeferredUpdateCallback()
{
	if (GEditor)
	{
		auto &vp=GEditor->GetAllViewportClients();
		for (auto &v : vp)
		{
			if (!v->IsRealtime())
				v->ToggleRealtime();
			// For heaven's sake. None of the below actually works to get a redraw.
			//v->RequestRealTimeFrames(150);
			//v->Invalidate();
			//v->Tick( 0.0);
		}
		GEditor->RedrawAllViewports();
	}
}

void FTrueSkyEditorPlugin::OnTimeChangedCallback(HWND OwnerHWND,float t)
{
	check( Instance );
	if ( SEditorInstance* const EditorInstance = Instance->FindEditorInstance(OwnerHWND) )
	{
		//EditorInstance->SaveSequenceData();
	}
	check( Instance );
	if ( SEditorInstance* const EditorInstance = Instance->FindEditorInstance(OwnerHWND) )
	{
		//EditorInstance->SaveSequenceData();
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
		if (trueSkyPlugin.GetActiveSequence() == EditorInstance->Asset)
		{
			trueSkyPlugin.OnUIChangedTime(t);
		}
	}
}

unsigned int FTrueSkyEditorPlugin::MessageId = 0;

LRESULT CALLBACK FTrueSkyEditorPlugin::EditorWindowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if PLATFORM_WINDOWS
	if ( uMsg == WM_SIZE)//||uMsg==WM_ACTIVATE||uMsg==WM_MOVE)
	{
		if ( HWND ChildHWND = GetWindow(hWnd, GW_CHILD) ) 
		{
			PostMessageW(ChildHWND, MessageId, wParam, lParam);
		}
	}
	check( Instance );
	if(Instance&&Instance->SetView)
	{
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
		const TArray <FLevelEditorViewportClient *>  &viewports=GEditor->GetLevelViewportClients();
		for(int i=0;i<viewports.Num();i++)
		{
			FEditorViewportClient *viewport=viewports[i];
			const FViewportCameraTransform &trans=viewport->GetViewTransform();
			
			FVector pos		=trans.GetLocation();
			FRotator rot	=trans.GetRotation();//.GetInverse();
			
			static float U=0.f,V=90.f,W=270.f;
			FRotator r2(U,V,W);
			FRotationMatrix R2(r2);

			FTransform tra(rot,pos);
		
			FMatrix viewmat		=tra.ToMatrixWithScale();
			FMatrix u			=R2.operator*(viewmat);
			FMatrix viewMatrix	=u.Inverse();
			// This gives us a matrix that's "sort-of" like the view matrix, but not.
			// So we have to mix up the y and z
			FMatrix proj;
			proj.SetIdentity();
			trueSkyPlugin.AdaptViewMatrix(viewMatrix);
			Instance->SetView(i,(const float*)&viewMatrix,(const float *)&proj);
		}
	}
	if(SEditorInstance* const EditorInstance = Instance->FindEditorInstance(hWnd))
	{
		return CallWindowProc( EditorInstance->OrigEditorWindowWndProc, hWnd, uMsg, wParam, lParam );
	}
#endif
	return 0;
}

void FTrueSkyEditorPlugin::InitPaths()
{
	if ( PathEnv == nullptr )
	{
		static FString path;
		path= GetEnvVariable(L"PATH");
		FString DllPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Simul/Win64")));
			
		path=(DllPath+TEXT(";"))+path;
		PathEnv=(const TCHAR*)path.GetCharArray().GetData() ;
		FPlatformMisc::SetEnvironmentVar(TEXT("PATH"), PathEnv);
	}
}

void FTrueSkyEditorPlugin::OnToggleRendering()
{
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
	trueSkyPlugin.OnToggleRendering();
}

IMPLEMENT_TOGGLE(ShowFades)
IMPLEMENT_TOGGLE(ShowCubemaps)
IMPLEMENT_TOGGLE(ShowWaterTextures)
IMPLEMENT_TOGGLE(ShowFlowRays)
IMPLEMENT_TOGGLE(ShowLightningOverlay)
IMPLEMENT_TOGGLE(ShowCompositing)
IMPLEMENT_TOGGLE(ShowRainOverlay)
IMPLEMENT_TOGGLE(ShowCelestialDisplay)
IMPLEMENT_TOGGLE(OnScreenProfiling)
IMPLEMENT_TOGGLE(Show3DCloudTextures)
IMPLEMENT_TOGGLE(Show2DCloudTextures)

bool FTrueSkyEditorPlugin::IsToggleRenderingEnabled()
{
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
	if(trueSkyPlugin.GetActiveSequence())
	{
		return true;
	}
	// No active sequence found!
	trueSkyPlugin.SetRenderingEnabled(false);
	return false;
}

bool FTrueSkyEditorPlugin::IsMenuEnabled()
{
	return true;
}

bool FTrueSkyEditorPlugin::IsToggleRenderingChecked()
{
	return RenderingEnabled;
}

void FTrueSkyEditorPlugin::DeployDefaultContent()
{
	// put the default stuff in:
	FString EnginePath		=FPaths::EngineDir();
	FString ProjectPath		=FPaths::GetPath(FPaths::GetProjectFilePath());
	FString ProjectContent	=FPaths::Combine(*ProjectPath,TEXT("Content"));
	// Note: "DeployToContent" may be empty. The "Content" folder is NOT to be copied.
	FString SrcSimulContent	=FPaths::Combine(*EnginePath, TEXT("Plugins/TrueSkyPlugin/DeployToContent"));
	FString DstSimulContent	=FPaths::Combine(*ProjectContent, TEXT("TrueSky"));
	// IF there is a DeployToContent folder, copy it. DON'T COPY THE "Content" FOLDER.
	if(FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*SrcSimulContent))
	{
		FPlatformFileManager::Get().GetPlatformFile().CopyDirectoryTree(*DstSimulContent, *SrcSimulContent, true);
	}
}

void FTrueSkyEditorPlugin::OnAddSequence()
{
	TrueSkySetupWindow = SNew(SWindow)
			.Title( NSLOCTEXT("InitializeTrueSky", "WindowTitle", "Initialize trueSKY") )
			.ClientSize( FVector2D(600, 550) )
			.SizingRule( ESizingRule::FixedSize )
			.SupportsMinimize(false) .SupportsMaximize(false);
	OnTrueSkySetupDelegate.BindRaw(this,&FTrueSkyEditorPlugin::OnTrueSkySetup);

	TSharedRef<STrueSkySetupTool> TrueSkySetupTool = SNew(STrueSkySetupTool).OnTrueSkySetup(OnTrueSkySetupDelegate);

	TrueSkySetupWindow->SetContent( TrueSkySetupTool );

	// If the main frame exists parent the window to it
	TSharedPtr< SWindow > ParentWindow;
	if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
	{
		IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>( "MainFrame" );
		ParentWindow = MainFrame.GetParentWindow();
	}
	
	bool modal=false;
	if (modal)
	{
		FSlateApplication::Get().AddModalWindow(TrueSkySetupWindow.ToSharedRef(), ParentWindow);
	}
	else if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(TrueSkySetupWindow.ToSharedRef(), ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(TrueSkySetupWindow.ToSharedRef());
	}
	TrueSkySetupWindow->ShowWindow();
}

ATrueSkySequenceActor* FTrueSkyEditorPlugin::GetSequenceActor()
{
	ULevel* const Level = GWorld->PersistentLevel;
	TArray<AActor*> SequenceActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ATrueSkySequenceActor::StaticClass(), SequenceActors);
	ATrueSkySequenceActor* SequenceActor = nullptr;
	if (SequenceActors.Num())
	{
		SequenceActor = Cast<ATrueSkySequenceActor>(SequenceActors[0]);
	}
	return SequenceActor;
}

void FTrueSkyEditorPlugin::OnTrueSkySetup(bool CreateDirectionalLight, ADirectionalLight* DirectionalLight,bool CreateTrueSkyLight,UTrueSkySequenceAsset *Sequence
	,bool DeleteFogs,bool DeleteBPSkySphere, bool OveridePostProcessVolume
	,bool AltitudeLightFunction)
{
	FScopedTransaction Transaction(LOCTEXT("OnTrueSkySetup", "TrueSky Setup"));
	ATrueSkySequenceActor* SequenceActor = GetSequenceActor();
	APostProcessVolume* PostProcessVolume = nullptr;
	if ( SequenceActor == nullptr )
	{
		// Add sequence actor
		SequenceActor=GWorld->SpawnActor<ATrueSkySequenceActor>(ATrueSkySequenceActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if(SequenceActor)
	{
		ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
		trueSkyPlugin.InitializeDefaults(SequenceActor);
	}
	if(CreateTrueSkyLight)
	{
		GWorld->SpawnActor<ATrueSkyLight>(ATrueSkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if (OveridePostProcessVolume)
	{

		TArray<AActor*> PostProcessVolumes;
		UGameplayStatics::GetAllActorsOfClass(GWorld, APostProcessVolume::StaticClass(), PostProcessVolumes);

		for (auto* Pp : PostProcessVolumes)
		{
			APostProcessVolume* PpVolume = Cast<APostProcessVolume>(Pp);
			if (PpVolume->bUnbound)
			{
				PostProcessVolume = PpVolume;
				PostProcessVolume->AddOrUpdateBlendable(LoadObject<UMaterial>(nullptr, TEXT("/TrueSkyPlugin/Translucent/TrueSkyTranslucent.TrueSkyTranslucent")), 1);
				break;
			}
		}

		
	}
	if(PostProcessVolume==nullptr)
	{
		PostProcessVolume = GWorld->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		PostProcessVolume->bUnbound = true;
		PostProcessVolume->SetActorLabel(FString("trueSKYPostProcessVolume"));
		PostProcessVolume->AddOrUpdateBlendable(LoadObject<UMaterial>(nullptr, TEXT("/TrueSkyPlugin/Translucent/TrueSkyTranslucent.TrueSkyTranslucent")), 1);
	}
	if(!DirectionalLight&&CreateDirectionalLight)
	{
		DirectionalLight=GWorld->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if(DirectionalLight)
	{
		DirectionalLight->SetMobility(EComponentMobility::Movable);
#if !SIMUL_BINARY_PLUGIN
		if(AltitudeLightFunction)
			DirectionalLight->GetLightComponent()->LightFunctionMaterial = LoadObject< UMaterial >(nullptr, TEXT("/TrueSkyPlugin/M_AltitudeLightFunction.M_AltitudeLightFunction"));
		else
#endif
			DirectionalLight->GetLightComponent()->LightFunctionMaterial=LoadObject< UMaterial >( nullptr, TEXT("/TrueSkyPlugin/M_LightFunction.M_LightFunction" ));
		// 50km
		DirectionalLight->GetLightComponent()->LightFunctionFadeDistance=100.0f*1000.0f*50.0f;
	}
	if(DeleteFogs)
	{
		TArray<AActor*> AtmosphericFogs;
		UGameplayStatics::GetAllActorsOfClass(GWorld, AAtmosphericFog::StaticClass(), AtmosphericFogs);
		for(auto A:AtmosphericFogs)
		{
			A->Destroy();
		}

		TArray<AActor*> HeightFogs;
		UGameplayStatics::GetAllActorsOfClass(GWorld, AExponentialHeightFog::StaticClass(), HeightFogs);
		for (auto A : HeightFogs)
		{
			A->Destroy();
		}
	}
	if(DeleteBPSkySphere)
	{
		TArray<AActor*> BPSkySpheres;
		UClass* BP_Sky_Sphere = FindObject<UClass>(ANY_PACKAGE, TEXT("BP_Sky_Sphere_C"));
		UGameplayStatics::GetAllActorsOfClass(GWorld, BP_Sky_Sphere, BPSkySpheres);
		for(auto A:BPSkySpheres)
		{
			A->Destroy();
		}
	}
	if(SequenceActor)
	{
		SequenceActor->ActiveSequence=Sequence;
		SequenceActor->Light=DirectionalLight;
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		if(A)
			A->Playing=true;
	}
}

void FTrueSkyEditorPlugin::ExportCloudLayer()
{
	FString filename;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> ParentWindow = MainFrameModule.GetParentWindow();
	TArray<FString> OutFilenames;
	if (DesktopPlatform)
	if ( ParentWindow.IsValid() )
	{
		void* ParentWindowWindowHandle = nullptr;
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}
		FString InOutLastPath = FPaths::ProjectDir() ;
	
		FString FileTypes = TEXT("Fbx File (*.fbx)|*.fbx|All Files (*.*)|*.*");
		DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			FString("Save to FBX"),
			InOutLastPath,
			TEXT(""),
			FileTypes,
			EFileDialogFlags::None,
			OutFilenames
		);
		if(OutFilenames.Num())
			filename=OutFilenames[0];
	}
	ITrueSkyPlugin &trueSkyPlugin=ITrueSkyPlugin::Get();
	trueSkyPlugin.ExportCloudLayer(filename,0);
}

void FTrueSkyEditorPlugin::OnSequenceDestroyed(UTrueSkySequenceAsset*const DestroyedAsset)
{
	SEditorInstance* EditorInstance = FindEditorInstance(DestroyedAsset);
	if(EditorInstance)
	{
		EditorInstance->EditorWindow->RequestDestroyWindow();
	}
}

bool FTrueSkyEditorPlugin::IsAddSequenceEnabled()
{
	ULevel* const Level = GWorld->PersistentLevel;
	for(int i=0;i<Level->Actors.Num();i++)
	{
		if ( Cast<ATrueSkySequenceActor>(Level->Actors[i]) )
			return false;
	}
	return true;
}

void FTrueSkyEditorPlugin::OpenEditor(UTrueSkySequenceAsset*const TrueSkySequence)
{
	if ( TrueSkySequence == nullptr )
		return;

	SEditorInstance* EditorInstance = FindEditorInstance(TrueSkySequence);
	if ( EditorInstance == nullptr )
	{
		EditorInstance = CreateEditorInstance(  nullptr );
		if ( EditorInstance )
		{
			TrueSkySequence->OnDestroyed.BindRaw(this, &FTrueSkyEditorPlugin::OnSequenceDestroyed);

			EditorInstance->Asset = TrueSkySequence;
			EditorInstance->EditorWindow->SetTitle(FText::FromString(TrueSkySequence->GetName()));
#ifdef ENABLE_AUTO_SAVING
			AutoSaveTimer = 4.0f;
#endif
		}
	}
	else
	{
		EditorInstance->EditorWindow->BringToFront();
	}
	// Set sequence asset to UI
	if ( EditorInstance )
	{
		EditorInstance->LoadSequenceData();
	}
}

#undef LOCTEXT_NAMESPACE

