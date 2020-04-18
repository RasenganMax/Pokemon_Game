// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyPluginPrivatePCH.h"
#include "TrueSkySequenceAsset.h"
#include "TrueSkySequenceActor.h"
#include "TrueSkyWaterActor.h"
#ifndef INCLUDE_UE_EDITOR_FEATURES
	#define INCLUDE_UE_EDITOR_FEATURES 0
#endif	// INCLUDE_UE_EDITOR_FEATURES.

#ifndef ENABLE_TRUESKY_WATER
	#define ENABLE_TRUESKY_WATER 1
#endif	// ENABLE_TRUESKY_WATER.

#undef IsMaximized
#undef IsMinimized
#include "GenericPlatform/GenericWindow.h"

#include <string>
#include <vector>

#include "Misc/CoreMisc.h"
#include "Misc/CoreDelegates.h"

#include "RendererInterface.h"
#include "TrueSkyLightComponent.h"
#include "TrueSkyWaterProbeComponent.h"
#include "TrueSkyWaterMaskComponent.h"
#include "DynamicRHI.h"
#include "RHIUtilities.h"
#include "RHIResources.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include <map>
#include <algorithm>
#include <wchar.h>
#include "ModifyDefinitions.h"
#include "TrueSkySettings.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"

#define CONST_UTF8_TO_TCHAR(str) (const TCHAR*)(FUTF8ToTCHAR((const ANSICHAR*)(str)).Get())
#define CONST_TCHAR_TO_UTF8(str) (const ANSICHAR*)FTCHARToUTF8((const TCHAR*)str).Get()

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"

#include "Containers/StringConv.h"
#if PLATFORM_XBOXONE
#include "Application/SlateApplicationBase.h"
#endif

#include "SimpleElementShaders.h"

#include "TrueSkySequencePropertiesFloat.h"
#include "TrueSkySequencePropertiesInt.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarTrueSkyRenderingEnabled(
	TEXT("TrueSky.RenderingEnabled"),
	1,
	TEXT("Toggles TrueSky rendering\n")
	TEXT(" 0: Rendering disabled.\n")
	TEXT(" 1: Rendering enabled.\n"),
	ECVF_Cheat);
#endif

bool simul::internal_logs=false;
// Until Epic updates FPostOpaqueRenderDelegate, define FRenderDelegateParameters to make equivalent, if not using the Simul branch of UE.
typedef FPostOpaqueRenderParameters FRenderDelegateParameters;
typedef FPostOpaqueRenderDelegate FRenderDelegate;

//[Milestone] TrueSky Parallel rendering: begin
int32 ParallelTrueSky = 0;
static FAutoConsoleVariableRef CVarParallelTrueSky(
	TEXT("TrueSky.ParallelRendering"),
	ParallelTrueSky,
	TEXT("Enable TrueSky parallel rendering"),
	ECVF_RenderThreadSafe);

simul::crossplatform::PixelFormat ToSimulPixelFormat(EPixelFormat u, simul::GraphicsDeviceType api)
{
	switch (u)
	{
		case PF_R16F:
			return simul::crossplatform::R_16_FLOAT;
		case PF_FloatRGBA:
			// I mean, what the hell.
			if(api== simul::GraphicsDeviceType::GraphicsDeviceVulkan)
				return simul::crossplatform::RGBA_16_FLOAT;
			else
				return simul::crossplatform::RGBA_32_FLOAT;
		case PF_FloatRGB:
			return simul::crossplatform::RGB_32_FLOAT;
		case PF_FloatR11G11B10:
			return simul::crossplatform::RGB_11_11_10_FLOAT;
		case PF_G32R32F:
			return simul::crossplatform::RG_32_FLOAT;
		case PF_G16R16F:
			return simul::crossplatform::RG_16_FLOAT;
		case PF_R32_FLOAT:
			return simul::crossplatform::R_32_FLOAT;
		case PF_R8G8B8A8:
			return simul::crossplatform::RGBA_8_UNORM;
		case PF_R32_UINT:
			return simul::crossplatform::R_32_UINT;
		case PF_R32G32B32A32_UINT:
			return simul::crossplatform::RGBA_32_UINT;
		case PF_DepthStencil:
			if (api == simul::GraphicsDeviceType::GraphicsDeviceVulkan)
				return simul::crossplatform::D_32_FLOAT_S_8_UINT;
			else
				return simul::crossplatform::D_32_FLOAT;
		case PF_D24:
			return simul::crossplatform::D_24_UNORM_S_8_UINT;
		case PF_B8G8R8A8:
			return simul::crossplatform::BGRA_8_UNORM;
		default:
			return simul::crossplatform::UNKNOWN;
	};
}

class FTrueSkyRenderDelegateParameters : public FRenderDelegateParameters
{
public:
	FTrueSkyRenderDelegateParameters(const FRenderDelegateParameters& p, FRHICommandList* InCurrentCmdList, FRHITexture2D *rendertarget)
		: FRenderDelegateParameters(p)
		, CurrentCmdList(InCurrentCmdList)
		, RenderTargetTexture(rendertarget)
	{}

	FRHICommandList* CurrentCmdList = nullptr;
	FRHITexture2D *RenderTargetTexture = nullptr;
};
//[Milestone] TrueSky Parallel rendering: end

#if PLATFORM_SWITCH
extern int errno;
#endif
#ifndef PLATFORM_UWP
#define PLATFORM_UWP 0
#endif
#ifndef USE_FAST_SEMANTICS_RENDER_CONTEXTS
#define USE_FAST_SEMANTICS_RENDER_CONTEXTS 0
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_PS4 || PLATFORM_UWP || PLATFORM_SWITCH || PLATFORM_LINUX
#define TRUESKY_PLATFORM_SUPPORTED 1
#else
#define TRUESKY_PLATFORM_SUPPORTED 0
#endif
#if (( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) )
#define SIMUL_SUPPORT_D3D12 1
#else
#define SIMUL_SUPPORT_D3D12 0
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE 
#define BREAK_IF_DEBUGGING if ( IsDebuggerPresent()) DebugBreak();
#elif PLATFORM_UWP 
#define BREAK_IF_DEBUGGING if ( IsDebuggerPresent() ) __debugbreak( );
#else
#if PLATFORM_PS4
#define BREAK_IF_DEBUGGING SCE_BREAK();
#else
#define BREAK_IF_DEBUGGING
#endif
#endif
#define ERRNO_CHECK \
if (errno != 0)\
		{\
		static bool checked=false;\
		if(!checked)\
		{\
			BREAK_IF_DEBUGGING\
			checked=true;\
		}\
		errno = 0; \
		}

#define ERRNO_CLEAR \
if (errno != 0)\
		{\
			errno = 0;\
		}
	
#if PLATFORM_PS4
    #include <kernel.h>
    #include <gnm.h>
    #include <gnm/common.h>
    #include <gnm/error.h>
    #include <gnm/shader.h>
    #include "GnmRHI.h"
    #include "GnmRHIPrivate.h"
    #include "GnmMemory.h"
    #include "GnmContext.h"

#elif PLATFORM_SWITCH
    #define USE_FAST_SEMANTICS_RENDER_CONTEXTS 0
    #include <errno.h>
    #define strcpy_s(a, b, c) strcpy(a, c)
    #define wcscpy_s(a, b, c) wcscpy(a, c)
    #define wcscat_s(a, b, c) wcscat(a, c)
#endif

// Dependencies.
#include "RHI.h"
#include "GPUProfiler.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SceneUtils.h"	// For SCOPED_DRAW_EVENT
#include "Containers/StaticArray.h"
#include "ActorCrossThreadProperties.h"
#include "WaterActorCrossThreadProperties.h"
#include "Allocator.h"
#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

#define LOCTEXT_NAMESPACE "TrueSkyPlugin"
#define LLOCTEXT_NAMESPACE L"TrueSkyPlugin"
/** This is a macro that casts a dynamically bound RHI reference to the appropriate D3D type. */
#define DYNAMIC_CAST_D3D11RESOURCE(Type,Name) \
	FD3D11##Type* Name = (FD3D11##Type*)Name##RHI;

DEFINE_LOG_CATEGORY(TrueSky);

#if INCLUDE_UE_EDITOR_FEATURES
DECLARE_LOG_CATEGORY_EXTERN(LogD3D11RHI, Log, All);
#endif
#if PLATFORM_LINUX || PLATFORM_MAC
#include <wchar.h>
#endif
#if PLATFORM_MAC
#endif

#if PLATFORM_WINDOWS || PLATFORM_UWP
#ifndef INTEL_METRICSDISCOVERY
#define INTEL_METRICSDISCOVERY 0
#endif
    #include "../Private/Windows/D3D11RHIBasePrivate.h"
    #include "D3D11Util.h"
    #include "D3D11RHIPrivate.h"
    #include "D3D11State.h"
    #include "D3D11Resources.h"
    typedef unsigned __int64 uint64_t;
    typedef FD3D11DynamicRHI FD3D11CommandListContext;
    #include "D3D12/TrueSkyPluginD3D12.h"
#endif
#if SIMUL_SUPPORT_VULKAN
#include "Vulkan/TrueSkyPluginVulkan.h"
#endif
#if PLATFORM_XBOXONE
    
        #include "D3D12/TrueSkyPluginD3D12.h"
    #endif

#if PLATFORM_SWITCH
    #include "NVNRHI.h"
    #include "NVNManager.h"
    #include "NVN/TrueSkyPluginNVN.h"
    
    #include <nn/fs.h>
    #include <nn/os.h>
    #include <nn/ro.h>

    #pragma clang optimize off

    struct DModuleData
    {
        DModuleData() :pNrr(nullptr), pNro(nullptr), bss(nullptr) {}
        uint8*                      pNrr;
        nn::ro::RegistrationInfo    nrrInfo;
        uint8*                      pNro;
        void*                       bss;
    }NXPluginData;

#endif

#include "Tickable.h"
#include "EngineModule.h"

using namespace simul;
using namespace unreal;

#if PLATFORM_SWITCH
    // NOTE: clang does not like the fact that we have an inline function inside 
    // the simul::unreal namespace, so other cpp won't be able to find it
    simul::unreal::ActorCrossThreadProperties* actorCrossThreadProperties = nullptr;
    simul::unreal::ActorCrossThreadProperties* simul::unreal::GetActorCrossThreadProperties()
    {
        return actorCrossThreadProperties;
    }
#else
    namespace simul
    {
    	namespace unreal
    	{
            ActorCrossThreadProperties *actorCrossThreadProperties=nullptr;
    		ActorCrossThreadProperties *GetActorCrossThreadProperties()
    		{
    			return actorCrossThreadProperties;
    		}
    	}
    }
#endif

TMap<int, WaterActorCrossThreadProperties*> waterActorsCrossThreadProperties;
WaterActorCrossThreadProperties* GetWaterActorCrossThreadProperties(int ID)
{
	return( ( waterActorsCrossThreadProperties.Contains( ID ) ) ? waterActorsCrossThreadProperties[ID] : nullptr );
}
void AddWaterActorCrossThreadProperties(WaterActorCrossThreadProperties* newProperties)
{	
	waterActorsCrossThreadProperties.Add( newProperties->ID, newProperties );
}
void RemoveWaterActorCrossThreadProperties(int ID)
{
	waterActorsCrossThreadProperties.Remove( ID );
}

void *GetPlatformDevice(GraphicsDeviceType api)
{
    void* device = nullptr;
#if PLATFORM_PS4
	// PS4 has no concept of separate devices. For "device" we will specify the immediate context's BaseGfxContext.
	FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();
	FGnmCommandListContext *ct=(FGnmCommandListContext*)(&CommandList.GetContext());
	sce::Gnmx::LightweightGfxContext *bs=&(ct->GetContext());
    device = (void*)bs;
#elif PLATFORM_SWITCH
    device = (void*)FNVNManager::GetDevice();
#else
	device = GDynamicRHI->RHIGetNativeDevice();
#if SIMUL_SUPPORT_VULKAN 
	if (api == GraphicsDeviceVulkan)
	{
		return (void*)GetDeviceVulkan(GDynamicRHI);
	}
#endif
#endif

	return device;
}

void* GetPlatformContext(FTrueSkyRenderDelegateParameters& RenderParameters, GraphicsDeviceType api)
{
	void *pContext=nullptr;
#if PLATFORM_PS4
	FGnmCommandListContext *ctx=(FGnmCommandListContext*)(&RenderParameters.CurrentCmdList->GetContext());
	sce::Gnmx::LightweightGfxContext *lwgfxc=&(ctx->GetContext());
	pContext=lwgfxc;
#endif
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		return GetContextD3D12(&RenderParameters.CurrentCmdList->GetContext());
	}
#endif
#if SIMUL_SUPPORT_VULKAN 
    if (api == GraphicsDeviceVulkan)
	{
		return GetContextVulkan(&RenderParameters.CurrentCmdList->GetContext());
	}
#endif
#if PLATFORM_SWITCH
    FNVNCommandContext* ctx = (FNVNCommandContext*)(&RenderParameters.CurrentCmdList->GetContext());
    return ctx->GetCommandBuffer();
#endif

	return pContext;
}

namespace simul
{
	namespace base
	{
		// Note: the following definition MUST exactly mirror the one in Simul/Base/FileLoader.h, but we wish to avoid includes from external projects,
		//  so it is reproduced here.
		class FileLoader
		{
		public:
			//! Returns a pointer to the current file handler.
			static FileLoader *GetFileLoader();
			//! Returns true if and only if the named file exists. If it has a relative path, it is relative to the current directory.
			virtual bool FileExists(const char *filename_utf8) const = 0;
			//! Set the file handling object: call this before any file operations, if at all.
			static void SetFileLoader(FileLoader *f);
			//! Put the file's entire contents into memory, by allocating sufficiently many bytes, and setting the pointer.
			//! The memory should later be freed by a call to \ref ReleaseFileContents.
			//! The filename should be unicode UTF8-encoded.
			virtual void AcquireFileContents(void*& pointer, unsigned int& bytes, const char* filename_utf8, bool open_as_text) = 0;
			//! Get the file date as a julian day number. Return zero if the file doesn't exist.
			virtual double GetFileDate(const char* filename_utf8) = 0;
			//! Free the memory allocated by AcquireFileContents.		
			virtual void ReleaseFileContents(void* pointer) = 0;
			//! Save the chunk of memory to storage.
			virtual bool Save(void* pointer, unsigned int bytes, const char* filename_utf8, bool save_as_text) = 0;
		};
	}
	namespace unreal
	{
		class UE4FileLoader:public base::FileLoader
		{
			static FString ProcessFilename(const char *filename_utf8)
			{
                FString Filename(CONST_UTF8_TO_TCHAR(filename_utf8));
#if PLATFORM_PS4
				Filename = Filename.ToLower();
#endif
                Filename = Filename.Replace(CONST_UTF8_TO_TCHAR("\\"), CONST_UTF8_TO_TCHAR("/"));
                Filename = Filename.Replace(CONST_UTF8_TO_TCHAR("//"), CONST_UTF8_TO_TCHAR("/"));
				return Filename;
			}
		public:
			UE4FileLoader()
			{}
			virtual ~UE4FileLoader()
			{
			}
			virtual bool FileExists(const char *filename_utf8) const override
			{
				FString Filename = ProcessFilename(filename_utf8);
				//UE_LOG(TrueSky,Display,TEXT("Checking FileExists %s"),*Filename);
				bool result=false;
				if(!Filename.IsEmpty())
				{
					result = FPlatformFileManager::Get().GetPlatformFile().FileExists(*Filename);
					// Now errno may be nonzero, which is unwanted.
					errno = 0;
				}
				//UE_LOG(TrueSky,Display,TEXT("Checking FileExists %s"),*Filename);
				return result;
			}
			virtual void AcquireFileContents(void*& pointer, unsigned int& bytes, const char* filename_utf8, bool open_as_text) override
			{
				pointer = nullptr;
				bytes = 0;
				if (!FileExists(filename_utf8))
				{
					return;
				}
				FString Filename = ProcessFilename(filename_utf8);
				ERRNO_CLEAR
				IFileHandle *fh = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Filename);
				if (!fh)
					return;
				ERRNO_CLEAR
				bytes = fh->Size();
				// We append a zero, in case it's text. UE4 seems to not distinguish reading binaries or strings.
				pointer = new uint8[bytes+1];
				fh->Read((uint8*)pointer, bytes);
				((uint8*)pointer)[bytes] = 0;
				delete fh;
				ERRNO_CLEAR
			}
			virtual double GetFileDate(const char* filename_utf8) override
			{
				FString Filename	=ProcessFilename(filename_utf8);
				if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*Filename))
					return 0.0;
				FDateTime dt		=FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*Filename);
				int64 uts			=dt.ToUnixTimestamp();
				double time_s		=( double(uts)/ 86400.0 );
				// Don't use FDateTime's GetModifiedJulianDay, it's only accurate to the nearest day!!
				return time_s;
			}
			virtual void ReleaseFileContents(void* pointer) override
			{
				delete [] ((uint8*)pointer);
			}
			virtual bool Save(void* pointer, unsigned int bytes, const char* filename_utf8, bool save_as_text) override
			{
				FString Filename = ProcessFilename(filename_utf8);
				IFileHandle *fh = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Filename);
				if (!fh)
				{
					errno=0;
					return false;
				}
				fh->Write((const uint8*)pointer, bytes);
				delete fh;
				errno=0;
				return true;
			}
		};
	}
}
using namespace simul;
using namespace unreal;

static void SetMultiResConstants(TArray<FVector2D> &MultiResConstants,FSceneView *View)
{
#ifdef NV_MULTIRES
	FMultiRes::RemapCBData RemapCBData;
	FMultiRes::CalculateRemapCBData(&View->MultiResConf, &View->MultiResViewports, &RemapCBData);
	
	 MultiResConstants.Add(RemapCBData.MultiResToLinearSplitsX);
	 MultiResConstants.Add(RemapCBData.MultiResToLinearSplitsY);
	
	for (int32 i = 0; i < 3; ++i)
	{
		MultiResConstants.Add(RemapCBData.MultiResToLinearX[i]);
    }
    for (int32 i = 0; i < 3; ++i)
    {
		MultiResConstants.Add(RemapCBData.MultiResToLinearY[i]);
    }
	MultiResConstants.Add(RemapCBData.LinearToMultiResSplitsX);
	MultiResConstants.Add(RemapCBData.LinearToMultiResSplitsY);
    for (int32 i = 0; i < 3; ++i)
    {
		MultiResConstants.Add(RemapCBData.LinearToMultiResX[i]);
    }
    for (int32 i = 0; i < 3; ++i)
    {
            MultiResConstants.Add(RemapCBData.LinearToMultiResY[i]);
    }
    check(FMultiRes::Viewports::Count == 9);
    for (int32 i = 0; i < FMultiRes::Viewports::Count; ++i)
    {
		const FViewportBounds& Viewport = View->MultiResViewportArray[i];
		MultiResConstants.Add(FVector2D(Viewport.TopLeftX, Viewport.TopLeftY));
		MultiResConstants.Add(FVector2D(Viewport.Width, Viewport.Height));
    }
    for (int32 i = 0; i < FMultiRes::Viewports::Count; ++i)
    {
		const FIntRect& ScissorRect = View->MultiResScissorArray[i];
		MultiResConstants.Add(FVector2D(ScissorRect.Min));
		MultiResConstants.Add(FVector2D(ScissorRect.Max));
    }
#endif
}

static simul::unreal::UE4FileLoader ue4SimulFileLoader;

#if PLATFORM_PS4
    typedef int moduleHandle;
#elif PLATFORM_SWITCH
    typedef nn::ro::Module moduleHandle;
#else
    typedef void* moduleHandle;
#endif

#if !TRUESKY_PLATFORM_SUPPORTED
typedef void* PlatformRenderTarget;
typedef void* PlatformDepthStencilTarget;
typedef void* PlatformTexture;
template<typename T> void *GetPlatformTexturePtr(T *t2, GraphicsDeviceType api)
{
	return nullptr;
}
PlatformTexture *GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
	return nullptr;
}

PlatformRenderTarget *GetPlatformRenderTarget(FRHITexture2D *t2d, GraphicsDeviceType api)
{
	return nullptr;
}
#endif

#if PLATFORM_LINUX
typedef void* PlatformRenderTarget;
typedef void* PlatformDepthStencilTarget;
typedef void* PlatformTexture;

PlatformTexture GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
    if (api == GraphicsDeviceVulkan)
    {
        return t.GetReference() ? t.GetReference()->GetNativeResource() : nullptr;
    }
    return nullptr;
}

void* GetPlatformRenderTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
    if (api == GraphicsDeviceVulkan)
    {
        return (void*)GetRenderTargetVulkan(t2d);// actually just an Image
    }
    return nullptr;
}

void* GetPlatformDepthStencilTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
    if (api == GraphicsDeviceVulkan)
    {
        return (void*)GetRenderTargetVulkan(t2d);
    }
    return nullptr;
}
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_UWP
	typedef void* PlatformRenderTarget;
	typedef void* PlatformDepthStencilTarget;
	typedef void* PlatformTexture;

PlatformTexture GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		return t.GetReference()?t.GetReference()->GetNativeResource():nullptr;
		//return (void*)GetPlatformTexturePtrD3D12(t2d);
	}
#endif
#if SIMUL_SUPPORT_VULKAN
	else if (api == GraphicsDeviceVulkan)
	{
		return t.GetReference() ? t.GetReference()->GetNativeResource() : nullptr;
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP  ) 
	ID3D11Texture2D *T = nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t.GetReference());
	if (m)
	{
		T = (ID3D11Texture2D*)(m->GetResource());
	}
	return T;
#endif
	return nullptr;
}


void* GetPlatformRenderTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		return (void*)GetRenderTargetD3D12(t2d);
	}
#endif
#if SIMUL_SUPPORT_VULKAN 
	if (api == GraphicsDeviceVulkan)
	{
		return (void*)GetRenderTargetVulkan(t2d);// actually just an Image
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP  )
	ID3D11RenderTargetView *T=nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t2d);
	if(m)
	{
		T = (ID3D11RenderTargetView*)(m->GetRenderTargetView(0,0));
		return (void*)T;
	}
#endif
	return nullptr;
}

void* GetPlatformDepthStencilTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		return (void*)GetRenderTargetD3D12(t2d);
	}
#endif
#if SIMUL_SUPPORT_VULKAN 
	if (api == GraphicsDeviceVulkan)
	{
		return (void*)GetRenderTargetVulkan(t2d);
	}
#endif
#if ( PLATFORM_WINDOWS || PLATFORM_UWP ) 
	ID3D11DepthStencilView *T = nullptr;
	FD3D11TextureBase *m = static_cast<FD3D11Texture2D*>(t2d);
	if (m)
	{
		T = (ID3D11DepthStencilView*)m->GetDepthStencilView(FExclusiveDepthStencil(FExclusiveDepthStencil::Type::DepthWrite_StencilWrite));
			return (void*)T;
	}
#endif
	return nullptr;
}
#endif

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_UWP || PLATFORM_LINUX
void* GetPlatformTexturePtr(FRHITexture2D *t2, GraphicsDeviceType api)
{
	if(!t2)
		return nullptr;
	return t2->GetNativeResource();
}

void* GetPlatformTexturePtr(FRHITexture3D *t2, GraphicsDeviceType api)
{
	if(!t2)
		return nullptr;
	return t2->GetNativeResource();
}

void* GetPlatformTexturePtr(UTexture *t, GraphicsDeviceType api)
{
	if (t&&t->Resource)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->Resource->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}
	
void* GetPlatformTexturePtr(FTexture *t, GraphicsDeviceType api)
{
	if (t)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}
#endif

#if PLATFORM_PS4
typedef sce::Gnm::Texture PlatformTexture;
typedef sce::Gnm::RenderTarget PlatformRenderTarget;
typedef sce::Gnm::DepthRenderTarget PlatformDepthStencilTarget;

sce::Gnm::Texture *GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
	sce::Gnm::Texture *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t.GetReference());
	if (m)
	{
		T = (sce::Gnm::Texture*)(m->Surface.Texture);
	}
	return T;
}

sce::Gnm::Texture *GetPlatformTexturePtr(FRHITexture2D *t2, GraphicsDeviceType api)
{
	sce::Gnm::Texture *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2);
	if(m)
	{
		T = (sce::Gnm::Texture*)(m->Surface.Texture);
	}
	return T;
}

PlatformTexture *GetPlatformTexturePtr(FTexture *t, GraphicsDeviceType api)
{
	if (t)
	{
		FRHITexture2D *t2 = static_cast<FRHITexture2D*>(t->TextureRHI.GetReference());
		return GetPlatformTexturePtr(t2,api);
	}
	return nullptr;
}

PlatformRenderTarget *GetPlatformRenderTarget(FRHITexture2D *t2d, GraphicsDeviceType api)
{
	sce::Gnm::RenderTarget *T=nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2d);
	if(m)
	{
		T = (sce::Gnm::RenderTarget*)(m->Surface.ColorBuffer);
	}
	return T;
}

PlatformDepthStencilTarget *GetPlatformDepthStencilTarget(FRHITexture2D *t2d)
{
	sce::Gnm::DepthRenderTarget *T = nullptr;
	FGnmTexture2D *m = static_cast<FGnmTexture2D*>(t2d);
	if (m)
	{
		T = (sce::Gnm::DepthRenderTarget*)(m->Surface.DepthBuffer);
	}
	return T;
}
#endif


#if PLATFORM_SWITCH

typedef nvn::Texture        PlatformTexture;
typedef RenderTargetHolder  PlatformRenderTarget;
typedef void*               PlatformDepthStencilTarget;

PlatformTexture* GetPlatformTexturePtr(FTextureRHIRef t, GraphicsDeviceType api)
{
    PlatformTexture* T = nullptr;
    FNVNTexture2D *m = static_cast<FNVNTexture2D*>(t.GetReference());
    if (m)
    {
        T = (PlatformTexture*)(m->GetSurface().GetTexture());
    }
    return T;
}

PlatformTexture* GetPlatformTexturePtr(FRHITexture2D* t2, GraphicsDeviceType api)
{
    PlatformTexture* T = nullptr;
    FNVNTexture2D*   m = static_cast<FNVNTexture2D*>(t2);
    if (m)
    {
        T = (PlatformTexture*)(m->GetSurface().GetTexture());
    }
    return T;
}

PlatformTexture* GetPlatformTexturePtr(FTexture* t, GraphicsDeviceType api)
{
    if (t)
    {
        FRHITexture2D* t2 = static_cast<FRHITexture2D*>(t->TextureRHI.GetReference());
        return GetPlatformTexturePtr(t2, api);
    }
    return nullptr;
}

PlatformRenderTarget* GetPlatformRenderTarget(FRHITexture2D* t2d, GraphicsDeviceType api)
{
    return GetRenderTargetHolderNVN(t2d);
}

PlatformDepthStencilTarget* GetPlatformDepthStencilTarget(FRHITexture2D *t2d)
{
    return nullptr;
}
#endif

//! Returns a view to the RenderTarget in RenderParameters.
void* GetColourTarget(FTrueSkyRenderDelegateParameters& RenderParameters, GraphicsDeviceType CurrentRHIType)
{
	void* target = nullptr;
#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) 
	if (CurrentRHIType == GraphicsDeviceD3D11 || CurrentRHIType==GraphicsDeviceD3D11_FastSemantics)
	{
		target = (void*)GetPlatformRenderTarget(RenderParameters.RenderTargetTexture, CurrentRHIType);
	}
#if SIMUL_SUPPORT_D3D12
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		target = (void*)GetRenderTargetD3D12(RenderParameters.RenderTargetTexture);
	}
#endif
#if SIMUL_SUPPORT_VULKAN 
	if (CurrentRHIType == GraphicsDeviceVulkan)
	{
		target = (void*)GetRenderTargetVulkan(RenderParameters.RenderTargetTexture);
	}
#endif
#elif PLATFORM_PS4
	target = (void*)GetPlatformRenderTarget(RenderParameters.RenderTargetTexture, CurrentRHIType);
#elif PLATFORM_SWITCH
    target = (void*)GetPlatformRenderTarget(RenderParameters.RenderTargetTexture, CurrentRHIType);
#endif
	return target;
}

#if PLATFORM_PS4
void RestorePS4ClipState(FGnmCommandListContext *GnmCommandListContext)
{
	GnmContextType *GnmContext = &GnmCommandListContext->GetContext();
	sce::Gnm::ClipControl Clip;
	Clip.init();
	Clip.setClipSpace(Gnm::kClipControlClipSpaceDX);

	// ISR multi-view writes to S_VIEWPORT_INDEX in vertex shaders
	static const auto CVarInstancedStereo = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.InstancedStereo"));
	static const auto CVarMultiView = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.MultiView"));

	if (CVarInstancedStereo &&
		CVarInstancedStereo->GetValueOnAnyThread() &&
		CVarMultiView &&
		CVarMultiView->GetValueOnAnyThread())
	{
		Clip.setForceViewportIndexFromVsEnable(true);
	}

	GnmContext->setClipControl(Clip);

	Gnm::DbRenderControl DBRenderControl;
	DBRenderControl.init();
	GnmContext->setDbRenderControl(DBRenderControl);

	// Can't access these values, let's hope no-one changed them.
	GnmContext->setDepthBoundsRange(0.0f, 1.0f);//CachedDepthBoundsMin, CachedDepthBoundsMax );
}
#endif
static void AddRef(FRHITexture *t)
{
	if (t)
		t->AddRef();
}
static void Release(FRHITexture *t)
{
	if (t)
		t->Release();
}
//FRHITexture *stored = nullptr;
void StoreUEState(FTrueSkyRenderDelegateParameters& RenderParameters, GraphicsDeviceType api)
{
	if (actorCrossThreadProperties->OverlayRT.GetReference())
		AddRef(actorCrossThreadProperties->OverlayRT.GetReference());
	if (actorCrossThreadProperties->TranslucentRT.GetReference())
		AddRef(actorCrossThreadProperties->TranslucentRT.GetReference());
	if( RenderParameters.DepthTexture )
		AddRef(RenderParameters.DepthTexture);
	if( RenderParameters.RenderTargetTexture )
		AddRef(RenderParameters.RenderTargetTexture);
#if SIMUL_SUPPORT_VULKAN
	if (api == GraphicsDeviceVulkan)
		StoreStateVulkan(&RenderParameters.CurrentCmdList->GetContext());
#endif
}

//! Restores the rendering state 
void RestoreUEState(FTrueSkyRenderDelegateParameters& RenderParameters, GraphicsDeviceType api)
{
	// Release the textures
	if (actorCrossThreadProperties->TranslucentRT.GetReference())
		actorCrossThreadProperties->TranslucentRT->Release();
	if (actorCrossThreadProperties->OverlayRT.GetReference())
		actorCrossThreadProperties->OverlayRT->Release();

	if( RenderParameters.DepthTexture )
		RenderParameters.DepthTexture->Release();
	if( RenderParameters.RenderTargetTexture )
		RenderParameters.RenderTargetTexture->Release();
#if SIMUL_SUPPORT_VULKAN
	if (api == GraphicsDeviceVulkan)
		RestoreStateVulkan(&RenderParameters.CurrentCmdList->GetContext());
#endif
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		RestoreHeapsD3D12(&RenderParameters.CurrentCmdList->GetContext());
	}
#endif
#if PLATFORM_PS4
	if (api == GraphicsDevicePS4)
	{
		FGnmCommandListContext* CommandListContext = (FGnmCommandListContext*)(&RenderParameters.CurrentCmdList->GetContext());
		RestorePS4ClipState(CommandListContext);
		CommandListContext->RestoreCachedDCBState();
	}
#endif
#if PLATFORM_SWITCH
	FNVNCommandContext* CommandListContext = (FNVNCommandContext*)(&RenderParameters.CurrentCmdList->GetContext());
	CommandListContext->RestoreState();
#endif
}

#ifndef UE_LOG_ONCE
#define UE_LOG_ONCE(a,b,c,...) {static bool done=false; if(!done) {UE_LOG(a,b,c, TEXT(""), ##__VA_ARGS__);done=true;}}
#endif

static void FlipFlapMatrix(FMatrix &v,bool flipx,bool flipy,bool flipz,bool flapx,bool flapy,bool flapz)
{
	if(flipx)
	{
		v.M[0][0]*=-1.f;
		v.M[0][1]*=-1.f;
		v.M[0][2]*=-1.f;
		v.M[0][3]*=-1.f;
	}
	if(flipy)
	{
		v.M[1][0]*=-1.f;
		v.M[1][1]*=-1.f;
		v.M[1][2]*=-1.f;
		v.M[1][3]*=-1.f;
	}
	if(flipz)
	{
		v.M[2][0]*=-1.f;
		v.M[2][1]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[2][3]*=-1.f;
	}
	if(flapx)
	{
		v.M[0][0]*=-1.f;
		v.M[1][0]*=-1.f;
		v.M[2][0]*=-1.f;
		v.M[3][0]*=-1.f;
	}
	if(flapy)
	{
		v.M[0][1]*=-1.f;
		v.M[1][1]*=-1.f;
		v.M[2][1]*=-1.f;
		v.M[3][1]*=-1.f;
	}
	if(flapz)
	{
		v.M[0][2]*=-1.f;
		v.M[1][2]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[3][2]*=-1.f;
	}
}

static void AdaptProjectionMatrix(FMatrix &projMatrix, float metresPerUnit)
{
	projMatrix.M[2][0]	*=-1.0f;
	projMatrix.M[2][1]	*=-1.0f;
	projMatrix.M[2][3]	*=-1.0f;
	projMatrix.M[3][2]	*= metresPerUnit;
}
// Bizarrely, Res = Mat1.operator*(Mat2) means Res = Mat2^T * Mat1, as
 //* opposed to Res = Mat1 * Mat2.
//  Equally strangely, the view matrix we get from rendering is NOT the same orientation as the one from the Editor Viewport class.
void AdaptScaledMatrix(FMatrix &sMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*sMatrix;
	//FMatrix TsToUe	=actorCrossThreadProperties->Transform.ToMatrixWithScale();
	FMatrix UeToTs	=u.InverseFast();
	static bool apply_scale=false;
	for(int i=apply_scale?0:3;i<4;i++)
	{
		for(int j=0;j<3;j++)
		{
			UeToTs.M[i][j]			*=actorCrossThreadProperties->MetresPerUnit;
		}
	}

	for(int i=0;i<4;i++)
		std::swap(UeToTs.M[i][0],UeToTs.M[i][1]);
	sMatrix=UeToTs;
}

// NOTE: What Unreal calls "y", we call "x". This is because trueSKY uses the engineering standard of a right-handed coordinate system,
// Whereas UE uses the graphical left-handed coordinates.
//
void AdaptViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*viewMatrix;
	u.M[3][0]	*= metresPerUnit;
	u.M[3][1]	*= metresPerUnit;
	u.M[3][2]	*= metresPerUnit;
	static float U=0.f,V=90.f,W=0.f;
	FRotator rot(U,V,W);
	FMatrix v;
	{
		FRotationMatrix RotMatrix(rot);
		FMatrix r	=RotMatrix.GetTransposed();
		v			=r.operator*(u);
		static bool x=true,y=false,z=false,X=false,Y=false,Z=true;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	viewMatrix=v;
}

// Just an ordinary transformation matrix: we must convert it from UE's left-handed system to right-handed for trueSKY
static void RescaleMatrix(FMatrix &viewMatrix,float metresPerUnit)
{
	viewMatrix.M[3][0]	*= metresPerUnit;
	viewMatrix.M[3][1]	*= metresPerUnit;
	viewMatrix.M[3][2]	*= metresPerUnit;
	static bool fulladapt=true;
	if(fulladapt)
	{
		static float U=0.f,V=-90.f,W=0.f;
		FRotationMatrix RotMatrix(FRotator(U,V,W));
		FMatrix r=RotMatrix.GetTransposed();
		FMatrix v=viewMatrix.operator*(r);		// i.e. r * viewMatrix
		static bool postm=true;
		if(postm)
			v=RotMatrix*v;
		
	
		static bool x=true,y=false,z=false,X=true,Y=false,Z=false;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
		static bool inv=true;
		if(inv)
			v=v.Inverse();
	
		viewMatrix=v;
		return;
	}
}
static void AdaptCubemapMatrix(FMatrix &viewMatrix)
{
	static float U=0.f,V=90.f,W=0.f;
	FRotationMatrix RotMatrix(FRotator(U,V,W));
	FMatrix r=RotMatrix.GetTransposed();
	FMatrix v=viewMatrix.operator*(r);
	static bool postm=true;
	if(postm)
		v=r.operator*(viewMatrix);
	{
		static bool x=true,y=false,z=false,X=false,Y=false,Z=true;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	{
		static bool x=true,y=true,z=false,X=false,Y=false,Z=false;
		FlipFlapMatrix(v,x,y,z,X,Y,Z);
	}
	viewMatrix=v;
}


#define ENABLE_AUTO_SAVING


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
	std::string str_utf8;
    str_utf8=CONST_TCHAR_TO_UTF8(*Source);
    return str_utf8;
#if 0
	const wchar_t *src_w=(const wchar_t*)(Source.GetCharArray().GetData());
	if(!src_w)
		return str_utf8;
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
    mbstate_t mbs;
    mbrlen (NULL,0,&mbs);    /* initialize mbs */
    size_t l=wcsrtombs(output_buffer, &src_w, (size_t)size_needed,&mbs );
    if (l==size_needed)
        output_buffer[size_needed]='\0';
#endif
    if(l<=size_needed  )
        output_buffer[l]=0;
    else
    {
        UE_LOG(TrueSky,Error,TEXT("String conversion error."));
    }
	str_utf8=std::string(output_buffer);
	delete [] output_buffer;
	return str_utf8;
#endif
}

#define DECLARE_TOGGLE(name)\
	void					OnToggle##name();\
	bool					IsToggled##name();

#define IMPLEMENT_TOGGLE(name)\
	void FTrueSkyPlugin::OnToggle##name()\
{\
	if(StaticGetRenderBool!=nullptr&&StaticSetRenderBool!=nullptr)\
	{\
		bool current=StaticGetRenderBool(#name);\
		StaticSetRenderBool(#name,!current);\
	}\
}\
\
bool FTrueSkyPlugin::IsToggled##name()\
{\
	if(StaticGetRenderBool!=nullptr)\
		if(StaticGetRenderBool)\
			return StaticGetRenderBool(#name);\
	return false;\
}

#define DECLARE_ACTION(name)\
	void					OnTrigger##name()\
	{\
		if(StaticTriggerAction!=nullptr)\
			StaticTriggerAction(#name);\
	}


class FTrueSkyTickable : public  FTickableGameObject
{
public:
	/** Tick interface */
	void					Tick( float DeltaTime );
	bool					IsTickable() const;
	TStatId					GetStatId() const;
};

struct int4
{
	int x, y, z, w;
};

struct RenderFrameStruct //MUST be kept in sync with any plugin version that supports this UE version
{
	static const int VERSION = 5;
	int version;
	void* device;
	void *pContext;
	int view_id;
	const float* viewMatrix4x4;
	const float* projMatrix4x4;
	void* depthTexture;
	simul::crossplatform::PixelFormat depthFormat;
	void* colourTarget;
	simul::crossplatform::PixelFormat colourFormat;
	void* colourTargetTexture;
	bool isColourTargetTexture;
	Viewport depthViewports[3];
	Viewport targetViewports[3];
	PluginStyle style;
	float exposure, gamma;
	uint32 framenumber;
	const float *nvMultiResConstants;
	int colourSamples;
	int depthSamples;
	ExternalTexture colour;
	ExternalTexture depth;
};

struct CopySkylightStruct
{
	static const int  static_version = 4;
	int			version;

	void		*pContext;
	void		*targetTex;
	int			cube_id;
	float		*shValues;
	int			shOrder;

	int			updateFrequency;
	float		blend;
	int			amortization;
	bool		allFaces;
	bool		allMips;

	float		engineToSimulMatrix4x4[16];
	float		exposure;
	float		gamma;
	float		ground_colour[3];

	int			size;
	simul::crossplatform::PixelFormat pixelFormat;
	int			mips;
};

struct WaterProbeValues
{
	int ID;
	float location[3];
	float velocity[3];
	float radius;
	float dEnergy;
};

struct WaterMeshObjectValues
{
	int ID;
	float location[3];
	float rotation[4];
	float scale[3];
	int noOfVertices;
	int noOfIndices;
	const float *vertices;
	const uint32 *indices;
};

struct waterMaskingObject
{
	WaterMeshObjectValues values;
	bool active;
	bool totalMask;
	maskObjectType objectType;
};

class FTrueSkyPlugin : public ITrueSkyPlugin
#ifdef SHARED_FROM_THIS
	, public TSharedFromThis<FTrueSkyPlugin,(ESPMode::Type)0>
#endif
	, public FSelfRegisteringExec
{
	FCriticalSection criticalSection;
	bool GlobalOverlayFlag;
	bool bIsInstancedStereoEnabled;
	bool post_trans_registered;
	TMap<int32,FTextureRHIRef> skylightTargetTextures;
public:
	FTrueSkyPlugin();
	virtual ~FTrueSkyPlugin();

	static FTrueSkyPlugin*	Instance;
	void					OnDebugTrueSky(class UCanvas* Canvas, APlayerController*);

	/** IModuleInterface implementation */
	void					StartupModule() override;
	void					PreUnloadCallback() override;
	void					ShutdownModule() override;
	void					PreExit();
	bool					SupportsDynamicReloading() override;

	// FSelfRegisteringExec
	virtual bool Exec( UWorld* WorldIn, const TCHAR* Cmd, FOutputDevice& Out = *GLog ) override;

	/** Render delegates */
	void					DelegatedRenderFrame( FRenderDelegateParameters& RenderParameters );
	void					DelegatedRenderPostTranslucent( FRenderDelegateParameters& RenderParameters );
	void					DelegatedRenderOverlays(FRenderDelegateParameters& RenderParameters);
	void					RenderFrame(uint64_t uid,FTrueSkyRenderDelegateParameters& RenderParameters,EStereoscopicPass StereoPass,bool bMultiResEnabled,int FrameNumber);
	void					RenderPostTranslucent(uint64_t uid,FTrueSkyRenderDelegateParameters& RenderParameters,EStereoscopicPass StereoPass,bool bMultiResEnabled,int FrameNumber,bool clear);
	void					RenderOverlays(uint64_t uid,FTrueSkyRenderDelegateParameters& RenderParameters,int FrameNumber);
	
	// This updates the TrueSkyLightComponents i.e. trueSky Skylights
	void					UpdateTrueSkyLights(FTrueSkyRenderDelegateParameters& RenderParameters,int FrameNumber);

	/** Init rendering */
	bool					InitRenderingInterface(  );

	/** Enable rendering */
	void					SetRenderingEnabled( bool Enabled );
	void					SetPostOpaqueEnabled(bool);
	void					SetPostTranslucentEnabled(bool);
	void					SetOverlaysEnabled(bool);

	/** Enable water rendering*/
	void					SetWaterRenderingEnabled(bool Enabled);

	/** Check for sequence actor to disable access to water variables*/
	bool					GetWaterRenderingEnabled();

	/** If there is a TrueSkySequenceActor in the persistent level, this returns that actor's TrueSkySequenceAsset */
	UTrueSkySequenceAsset*	GetActiveSequence();
	void UpdateFromActor(FTrueSkyRenderDelegateParameters& RenderParameters);
	
	void					*GetRenderEnvironment() override;
	bool					TriggerAction(const char *name);
	bool					TriggerAction(const FString &fname) override;
	static void				LogCallback(const char *txt);
	
	void					SetPointLight(int id,FLinearColor c,FVector pos,float min_radius,float max_radius) override;

	float					GetCloudinessAtPosition(int32 queryId,FVector pos) override;
	VolumeQueryResult		GetStateAtPosition(int32 queryId,FVector pos) override;
	LightingQueryResult		GetLightingAtPosition(int32 queryId,FVector pos) override;
	float					CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos) override;

	void					SetRenderBool(const FString &fname, bool value) override;
	bool					GetRenderBool(const FString &fname) const override;

	void					SetRenderFloat(const FString &fname, float value) override;
	float					GetRenderFloat(const FString &fname) const override;
	float					GetRenderFloatAtPosition(const FString &fname,FVector pos) const override;
	float					GetFloatAtPosition(int64 Enum,FVector pos,int32 uid) const override;
	float					GetFloat(int64 TrueSkyEnum) const override;
	FVector					GetVector(int64 TrueSkyEnum) const override;
	void					SetVector(int64 TrueSkyEnum,FVector value) const override;
	
	void					SetRender(const FString &fname,const TArray<FVariant> &params) override;
	void					SetRenderInt(const FString& name, int value) override;
	int						GetRenderInt(const FString& name) const override;
	int						GetRenderInt(const FString& name,const TArray<FVariant> &params) const override;
	
	void					SetRenderString(const FString &fname, const FString & value) override;
	FString					GetRenderString(const FString &fname) const override;

	void					SetKeyframeInt(unsigned uid, const FString& name, int value) override;
	int						GetKeyframeInt(unsigned uid, const FString& name) const override;
	void					SetKeyframeFloat(unsigned uid,const FString &fname, float value) override;
	float					GetKeyframeFloat(unsigned uid,const FString &fname) const override;

	void					SetKeyframerInt(unsigned uid, const FString& name, int value) override;
	int						GetKeyframerInt(unsigned uid, const FString& name) const override;
	void					SetKeyframerFloat(unsigned uid, const FString& fname, float value) override;
	float					GetKeyframerFloat(unsigned uid, const FString& fname) const override;

	void					CreateBoundedWaterObject(unsigned int ID, float* dimension, float* location) override;
	void					RemoveBoundedWaterObject(unsigned int ID) override;
							   
	bool					AddWaterProbe(int ID, FVector location, FVector velocity, float radius) override;
	void					RemoveWaterProbe(int ID) override;
	FVector4				GetWaterProbeValues(int ID) override;
	void					UpdateWaterProbeValues(int ID, FVector location, FVector velocity, float radius, float dEnergy) override;

	bool					AddWaterBuoyancyObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices) override;
	void					UpdateWaterBuoyancyObjectValues(int ID, FVector location, FQuat rotation, FVector scale) override;
	TArray<float>			GetWaterBuoyancyObjectResults(int ID, int noOFVertices) override;
	void					RemoveWaterBuoyancyObject(int ID) override;

	bool					AddWaterMaskObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices, TArray<uint32> indices, maskObjectType objectType, bool active, bool totalMask) override;
	void					UpdateWaterMaskObjectValues(int ID, FVector location, FQuat rotation, FVector scale, maskObjectType objectType, bool active, bool totalMask) override;
	void					RemoveWaterMaskObject(int ID) override;

	virtual void			SetWaterFloat(const FString &fname, unsigned int ID, float value) override;
	virtual void			SetWaterInt(const FString &fname, unsigned int ID, int value) override;
	virtual void			SetWaterBool(const FString &fname, unsigned int ID, bool value) override;
	virtual void			SetWaterVector(const FString &fname, int ID,const FVector &value) override;
	virtual float			GetWaterFloat(const FString &fname, unsigned int ID)  const override;
	virtual int				GetWaterInt(const FString &fname, unsigned int ID) const override;
	virtual bool			GetWaterBool(const FString &fname, unsigned int ID) const override;
	virtual FVector			GetWaterVector(const FString &fname, unsigned int ID) const override;

	void SetInt( ETrueSkyPropertiesInt Property, int32 Value );
	void SetVector( ETrueSkyPropertiesVector Property, FVector Value );
	void SetFloat( ETrueSkyPropertiesFloat Property, float Value );

	void SetWaterInt(ETrueSkyWaterPropertiesInt Property, int ID, int32 Value);
	void SetWaterVector(ETrueSkyWaterPropertiesVector Property, int ID, const FVector &Value);
	void SetWaterFloat(ETrueSkyWaterPropertiesFloat Property, int ID, float Value);

	FMatrix UEToTrueSkyMatrix(bool apply_scale=true) const;
	FMatrix TrueSkyToUEMatrix(bool apply_scale=true) const;
	virtual UTexture *GetAtmosphericsTexture()
	{
		return AtmosphericsTexture;
	}
	struct CloudVolume
	{
		FTransform transform;
		FVector extents;
	};
	virtual void ClearCloudVolumes()
	{
	criticalSection.Lock();
		cloudVolumes.clear();
	criticalSection.Unlock();
	}
	virtual void SetCloudVolume(int i,const FTransform &tr,FVector ext)
	{
	criticalSection.Lock();
		cloudVolumes[i].transform=tr;
		cloudVolumes[i].extents=ext;
	criticalSection.Unlock();
	}
	
	virtual int32 SpawnLightning(FVector startpos,FVector endpos,float magnitude,FVector colour) override
	{
		startpos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,startpos);
		endpos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,endpos);
		return StaticSpawnLightning(((const float*)(&startpos)),((const float*)(&endpos)) , magnitude,((const float*)(&colour)) );
	}
	virtual void RequestColourTable(unsigned uid,int x,int y,int z) override
	{
		if(colourTableRequests.find(uid)==colourTableRequests.end()||colourTableRequests[uid]==nullptr)
			colourTableRequests[uid]=new ColourTableRequest;

		ColourTableRequest *req=colourTableRequests[uid];
		
		req->uid=uid;
		req->x=x;
		req->y=y;
		req->z=z;
	}
	virtual const ColourTableRequest *GetColourTable(unsigned uid) override
	{
		auto it=colourTableRequests.find(uid);
		if(it==colourTableRequests.end())
			return nullptr;
		if(it->second->valid==false)
			return nullptr;
		return it->second;
	}
	virtual void ClearColourTableRequests() override
	{
		for(auto i:colourTableRequests)
		{
			delete [] i.second->data;
			delete i.second;
		}
		colourTableRequests.clear();
	}
	virtual SimulVersion GetVersion() const override
	{
		return simulVersion;
	}
	int64	GetEnum(const FString &name) const override;

	simul::unreal::RenderingInterface *GetRenderingInterface() override;
protected:
	bool PostOpaqueEnabled ;
	bool PostTranslucentEnabled ;
	bool OverlaysEnabled ;
	
	std::map<unsigned,ColourTableRequest*> colourTableRequests;
	std::map<int,CloudVolume> cloudVolumes;

	void					PlatformSetGraphicsDevice();
	bool					EnsureRendererIsInitialized();
	
	UTexture				*AtmosphericsTexture;
	void					OnMainWindowClosed(const TSharedRef<SWindow>& Window);

	/** Called when Toggle rendering button is pressed */
	void					OnToggleRendering();

	void					OnUIChangedSequence();
	void					OnUIChangedTime(float t);

	void					ExportCloudLayer(const FString& filenameUtf8,int index);

	/** Called when the Actor is pointed to a different sequence.*/
	void					SequenceChanged();
	/** Returns true if Toggle rendering button should be enabled */
	bool					IsToggleRenderingEnabled();
	/** Returns true if Toggle rendering button should be checked */
	bool					IsToggleRenderingChecked();

	/** Called when the Toggle Fades Overlay button is pressed*/
	DECLARE_TOGGLE(ShowFades)
	DECLARE_TOGGLE(ShowCompositing)
	DECLARE_TOGGLE(ShowWaterTextures)
	DECLARE_TOGGLE(ShowFlowRays)
	DECLARE_TOGGLE(Show3DCloudTextures)
	DECLARE_TOGGLE(Show2DCloudTextures)

	void ShowDocumentation()
	{
	//	FString DocumentationUrl = FDocumentationLink::ToUrl(Link);
		FString DocumentationUrl="https://docs.simul.co";
		FPlatformProcess::LaunchURL(*DocumentationUrl, nullptr, nullptr);
	}
	
	/** Returns true if user can add a sequence actor */
	bool					IsAddSequenceEnabled();

	/** Initializes all necessary paths */
	void					InitPaths();
	

	// global so that we can store the VR viewports.
	RenderFrameStruct frameStruct;
	CopySkylightStruct copySkylightStruct;
	void SetEditorCallback(FTrueSkyEditorCallback c) override
	{
		TrueSkyEditorCallback=c;
	}
	void InitializeDefaults(ATrueSkySequenceActor *a) override
	{
		a->initializeDefaults=true;
	}

	void AdaptScaledMatrix(FMatrix &sMatrix);
	void AdaptViewMatrix(FMatrix &viewMatrix,bool editor_version=false) const;
	
	typedef int (*FStaticInitInterface)(  );
	typedef int (*FGetSimulVersion)(int *major,int *minor,int *build);
	typedef void(*FStaticSetMemoryInterface)(simul::base::MemoryInterface *);
	typedef int (*FStaticShutDownInterface)(  );
	typedef void (*FLogCallback)(const char *);
	typedef void (*FStaticSetDebugOutputCallback)(FLogCallback);
	typedef void (*FStaticSetGraphicsDevice)(void* device, GraphicsDeviceType deviceType,GraphicsEventType eventType);
	typedef void (*FStaticSetGraphicsDeviceAndContext)(void* device,void* context, GraphicsDeviceType deviceType, GraphicsEventType eventType);
	typedef int (*FStaticPushPath)(const char*,const char*);
	typedef void(*FStaticSetFileLoader)(simul::base::FileLoader *);
	typedef int (*FStaticGetOrAddView)( void *);
	typedef int (*FStaticGetLightningBolts)(LightningBolt *,int maxb);
	typedef int (*FStaticSpawnLightning)(const float startpos[3],const float endpos[3],float magnitude,const float colour[3]);
	typedef int (*FStaticRenderFrame)(void* device,void* deviceContext,int view_id
		,float* viewMatrix4x4
		,float* projMatrix4x4
		,void* depthTexture
		,void* depthResource
		,const Viewport *depthViewport
		,const Viewport *v
		,PluginStyle s
		,float exposure
		,float gamma
		,int framenumber
		,const float *nvMultiResConstants);
	typedef int(*FStaticRenderFrame2)(RenderFrameStruct* frameStruct);
	typedef int (*FStaticCopySkylight)(void *pContext
												,int cube_id
												,float* shValues
												,int shOrder
												,void *targetTex
												,const float *engineToSimulMatrix4x4
												,int updateFrequency
												,float blend
												,float copy_exposure
												,float copy_gamma);
	typedef int (*FStaticCopySkylight2)(void *pContext
												,int cube_id
												,float* shValues
												,int shOrder
												,void *targetTex
												,const float *engineToSimulMatrix4x4
												,int updateFrequency
												,float blend
												,float copy_exposure
												,float copy_gamma
												,const float *ground_colour);
	typedef int (*FStaticCopySkylight3)(CopySkylightStruct *sk);
	typedef int (*FStaticProbeSkylight)(void *pContext
												,int cube_id
												,int mip_size
												,int face_index
												,int x
												,int y
												,int w
												,int h
												,float *targetValues);
	typedef int (*FStaticRenderOverlays)(void* device,void* deviceContext,void* depthTexture,
				 const float* viewMatrix4x4,const float* projMatrix4x4,int view_id, void *colourTarget, const Viewport *viewports);
	typedef int(*FStaticRenderOverlays2)(RenderFrameStruct* frameStruct);
	typedef int (*FStaticTick)( float deltaTime );
	typedef int (*FStaticOnDeviceChanged)( void * device );
	typedef void* (*FStaticGetEnvironment)();
	typedef int (*FStaticSetSequence)( std::string sequenceInputText );
	typedef int(*FStaticSetSequence2)(const char *sequenceInputText);
	typedef int(*FStaticSetMoon)(int id,ExternalMoon *);
	typedef class UE4PluginRenderInterface* (*FStaticGetRenderInterfaceInstance)();
	typedef void (*FStaticSetPointLight)(int id,const float pos[3],float radius,float maxradius,const float radiant_flux[3]);
	typedef void (*FStaticCloudPointQuery)(int id,const float *pos, VolumeQueryResult *res);
	typedef void (*FStaticLightingQuery)(int id,const float *pos, LightingQueryResult *res);
	typedef void (*FStaticCloudLineQuery)(int id,const float *startpos,const float *endpos, LineQueryResult *res);
	
	typedef void(*FStaticSetRenderTexture)(const char *name, void* texturePtr);
	typedef void(*FStaticSetRenderTexture2)(const char *name, ExternalTexture* t);
	typedef void(*FStaticSetMatrix4x4)(const char *name, const float []);
	
	typedef void (*FStaticSetRender)(const char *name, int num_params, const VariantPass *params);
	typedef void (*FStaticSetRenderBool)(const char *name, bool value);
	typedef bool (*FStaticGetRenderBool)(const char *name);
	typedef void (*FStaticSetRenderFloat)(const char *name, float value);
	typedef float (*FStaticGetRenderFloat)(const char *name);
	
	typedef void (*FStaticSetRenderInt)(const char *name, int value);
	typedef int (*FStaticGetRender)(const char *name, int num_params, const VariantPass *params);
	typedef int (*FStaticGetRenderInt)(const char *name, int num_params, const VariantPass *params);
	typedef int (*FStaticGetEnum)(const char *name);
	typedef void (*FStaticProcessQueries)(int num, Query *q);

	typedef bool (*FStaticCreateBoundedWaterObject) (unsigned int ID, float* dimension, float* location);
	typedef void (*FStaticRemoveBoundedWaterObject) (unsigned int ID);

	typedef bool (*FStaticAddWaterProbe) (WaterProbeValues *probeValues);
	typedef void (*FStaticRemoveWaterProbe) (int ID);
	typedef void (*FStaticUpdateWaterProbeValues) (WaterProbeValues *probeValues);
	typedef void (*FStaticGetWaterProbeValues) (int ID, vec4* result);

	typedef bool(*FStaticAddWaterBuoyancyObject) (WaterMeshObjectValues *buoyancyObjectValues);
	typedef void(*FStaticUpdateWaterBuoyancyObjectValues) (WaterMeshObjectValues *buoyancyObjectValues);
	typedef float*(*FStaticGetWaterBuoyancyObjectResults) (int ID);
	typedef void(*FStaticRemoveWaterBuoyancyObject) (int ID);

	typedef bool(*FStaticAddWaterMaskObject) (waterMaskingObject* newObject);
	typedef void(*FStaticUpdateWaterMaskObjectValues) (waterMaskingObject* values);
	typedef void(*FStaticRemoveWaterMaskObject) (int ID);

	typedef void (*FStaticSetWaterFloat) (const char* name, unsigned int ID, float value);
	typedef void (*FStaticSetWaterInt) (const char* name, unsigned int ID, int value);
	typedef void (*FStaticSetWaterBool) (const char* name, unsigned int ID, bool value);
	typedef void (*FStaticSetWaterVector) (const char* name, int ID, const float* value);
	typedef float (*FStaticGetWaterFloat) (const char* name, unsigned int ID);
	typedef int (*FStaticGetWaterInt) (const char* name, unsigned int ID);
	typedef bool (*FStaticGetWaterBool) (const char* name, unsigned int ID);
	typedef bool (*FStaticGetWaterVector) (const char* name, unsigned int ID,float *value);

	typedef void (*FStaticSetRenderString)( const char *name,const FString &value );
	typedef void (*FStaticGetRenderString)( const char *name ,char* value,int len);
	typedef bool (*FStaticTriggerAction)( const char *name );
	
	typedef void (*FStaticExportCloudLayerToGeometry)(const char *filenameUtf8,int index);

	typedef void (*FStaticSetKeyframeFloat)(unsigned,const char *name, float value);
	typedef float (*FStaticGetKeyframeFloat)(unsigned,const char *name);
	typedef void (*FStaticSetKeyframeInt)(unsigned,const char *name, int value);
	typedef int (*FStaticGetKeyframeInt)(unsigned,const char *name);

	typedef void (*FStaticSetKeyframerFloat)(unsigned, const char* name, float value);
	typedef float (*FStaticGetKeyframerFloat)(unsigned, const char* name);
	typedef void (*FStaticSetKeyframerInt)(unsigned, const char* name, int value);
	typedef int (*FStaticGetKeyframerInt)(unsigned, const char* name);

	typedef float (*FStaticGetRenderFloatAtPosition)(const char* name,const float *pos);

	// Using enums rather than char strings:
	typedef float (*FStaticGetFloatAtPosition)(long long enum_,const float *pos,int uid);
	typedef int(*FStaticGet)(long long enum_, VariantPass *variant);
	typedef int (*FStaticSet)(long long enum_, const VariantPass *variant);
	typedef int (*FStaticWaterSet)(long long enum_, int ID, const VariantPass *variant);

	typedef bool (*FStaticFillColourTable)(unsigned uid,int x,int y,int z,float *target);
	typedef RenderingInterface * (*FStaticGetRenderingInterface)();
	typedef void (*FStaticExecuteDeferredRendering)();

	FTrueSkyEditorCallback				TrueSkyEditorCallback;

	FStaticInitInterface				StaticInitInterface;
	FGetSimulVersion					GetSimulVersion;
	FStaticSetMemoryInterface			StaticSetMemoryInterface;
	FStaticShutDownInterface			StaticShutDownInterface;
	FStaticSetDebugOutputCallback		StaticSetDebugOutputCallback;
	FStaticSetGraphicsDevice			StaticSetGraphicsDevice;
	FStaticSetGraphicsDeviceAndContext	StaticSetGraphicsDeviceAndContext;	// Required for dx12 and switch
	FStaticPushPath						StaticPushPath;
	FStaticSetFileLoader				StaticSetFileLoader;
	FStaticGetOrAddView					StaticGetOrAddView;
	FStaticRenderFrame					StaticRenderFrame;
	FStaticRenderFrame2					StaticRenderFrame2;
	FStaticCopySkylight					StaticCopySkylight;
	FStaticCopySkylight2				StaticCopySkylight2;
	FStaticCopySkylight3				StaticCopySkylight3;
	FStaticProbeSkylight				StaticProbeSkylight;
						
	FStaticRenderOverlays				StaticRenderOverlays;
	FStaticRenderOverlays2				StaticRenderOverlays2;
	FStaticTick							StaticTick;
	FStaticOnDeviceChanged				StaticOnDeviceChanged;
	FStaticGetEnvironment				StaticGetEnvironment;
	FStaticSetSequence					StaticSetSequence;
	FStaticSetSequence2					StaticSetSequence2;
	FStaticSetMoon						StaticSetMoon;
	FStaticGetRenderInterfaceInstance	StaticGetRenderInterfaceInstance;
	FStaticSetPointLight				StaticSetPointLight;
	FStaticCloudPointQuery				StaticCloudPointQuery;
	FStaticLightingQuery				StaticLightingQuery;
	FStaticCloudLineQuery				StaticCloudLineQuery;
	FStaticSetRenderTexture				StaticSetRenderTexture;
	FStaticSetRenderTexture2			StaticSetRenderTexture2;
	FStaticSetMatrix4x4					StaticSetMatrix4x4;
	FStaticSetRenderBool				StaticSetRenderBool;
	FStaticSetRender					StaticSetRender;
	FStaticGetRenderBool				StaticGetRenderBool;
	FStaticSetRenderFloat				StaticSetRenderFloat;
	FStaticGetRenderFloat				StaticGetRenderFloat;
	FStaticSetRenderInt					StaticSetRenderInt;
	FStaticGetRender					StaticGetRender;
	FStaticGetRenderInt					StaticGetRenderInt;
	FStaticGetEnum						StaticGetEnum;
	FStaticProcessQueries				StaticProcessQueries;
	FStaticSetRenderString				StaticSetRenderString;
	FStaticGetRenderString				StaticGetRenderString;
	FStaticTriggerAction				StaticTriggerAction;
	
	FStaticCreateBoundedWaterObject		StaticCreateBoundedWaterObject;
	FStaticRemoveBoundedWaterObject		StaticRemoveBoundedWaterObject;
	FStaticAddWaterProbe				StaticAddWaterProbe;
	FStaticRemoveWaterProbe				StaticRemoveWaterProbe;
	FStaticUpdateWaterProbeValues		StaticUpdateWaterProbeValues;
	FStaticGetWaterProbeValues			StaticGetWaterProbeValues;
	FStaticAddWaterBuoyancyObject		StaticAddWaterBuoyancyObject;
	FStaticUpdateWaterBuoyancyObjectValues StaticUpdateWaterBuoyancyObjectValues;
	FStaticGetWaterBuoyancyObjectResults StaticGetWaterBuoyancyObjectResults;
	FStaticRemoveWaterBuoyancyObject	StaticRemoveWaterBuoyancyObject;
	FStaticAddWaterMaskObject			StaticAddWaterMaskObject;
	FStaticUpdateWaterMaskObjectValues	StaticUpdateWaterMaskObjectValues;
	FStaticRemoveWaterMaskObject		StaticRemoveWaterMaskObject;

	FStaticSetWaterFloat				StaticSetWaterFloat;
	FStaticSetWaterInt					StaticSetWaterInt;
	FStaticSetWaterBool					StaticSetWaterBool;
	FStaticSetWaterVector				StaticSetWaterVector;
	FStaticGetWaterFloat				StaticGetWaterFloat;
	FStaticGetWaterInt					StaticGetWaterInt;
	FStaticGetWaterBool					StaticGetWaterBool;
	FStaticGetWaterVector				StaticGetWaterVector;
	
	FStaticExportCloudLayerToGeometry	StaticExportCloudLayerToGeometry;

	FStaticSetKeyframeFloat				StaticSetKeyframeFloat;
	FStaticGetKeyframeFloat				StaticGetKeyframeFloat;
	FStaticSetKeyframeInt				StaticSetKeyframeInt;
	FStaticGetKeyframeInt				StaticGetKeyframeInt;

	FStaticSetKeyframerFloat			StaticSetKeyframerFloat;
	FStaticGetKeyframerFloat			StaticGetKeyframerFloat;
	FStaticSetKeyframerInt				StaticSetKeyframerInt;
	FStaticGetKeyframerInt				StaticGetKeyframerInt;

	FStaticGetRenderFloatAtPosition		StaticGetRenderFloatAtPosition;
	FStaticGetFloatAtPosition			StaticGetFloatAtPosition;
	FStaticGet							StaticGet;
	FStaticSet							StaticSet;
	FStaticWaterSet						StaticWaterSet;
	FStaticGetLightningBolts			StaticGetLightningBolts;

	FStaticSpawnLightning				StaticSpawnLightning;

	FStaticFillColourTable				StaticFillColourTable;
	FStaticGetRenderingInterface		StaticGetRenderingInterface;
	FStaticExecuteDeferredRendering		StaticExecuteDeferredRendering;
	const TCHAR*			PathEnv;
	GraphicsDeviceType		CurrentRHIType;
	bool					RenderingEnabled;
	bool					RendererInitialized;

	bool					WaterRenderingEnabled;

	float					CachedDeltaSeconds;
	float					AutoSaveTimer;		// 0.0f == no auto-saving
	
	bool					actorPropertiesChanged;
	bool					haveEditor;
	bool					exportNext;
	FString					exportFilenameUtf8;
	UTrueSkySequenceAsset *sequenceInUse;
	uint32 LastFrameNumber, LastStereoFrameNumber;
	SimulVersion simulVersion;
 
	// Callback for when the settings were saved.
	bool HandleSettingsSaved()
	{
		UTrueSkySettings* Settings = GetMutableDefault<UTrueSkySettings>();
		bool ResaveSettings = false;
 
		// You can put any validation code in here and resave the settings in case an invalid
		// value has been entered
 
		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}
 
		return true;
	}

	//MILESTONE TODO: verify splitscreen support: "LastTaskCompletionEvent" should be enough to sync (serialize) all opaque and translucent passes
	//				  and views, but, if more parallelism is required, add more completion events to sync them.
	FGraphEventRef LastTaskCompletionEvent;

	/// Initialize the struct for sending to the render dll.
	void InitRenderFrameStruct(RenderFrameStruct &frameStruct, FTrueSkyRenderDelegateParameters &RenderParameters
		, FRHITexture2D *TargetTexture
		, bool bMultiResEnabled
		, int view_id
		, int FrameNumber
		, PluginStyle style
		, bool isTranslucentPass) const ;
	void SetRenderTexture(FRHICommandListImmediate *RHICmdList,const char *name,FRHITexture*);
	int StructSizeWrong(const char *structName, size_t thisSize) const;
};

IMPLEMENT_MODULE( FTrueSkyPlugin, TrueSkyPlugin )
FTrueSkyPlugin* FTrueSkyPlugin::Instance = nullptr;
static FString trueSkyPluginPath="../../Plugins/TrueSkyPlugin";

FTrueSkyPlugin::FTrueSkyPlugin()
	:GlobalOverlayFlag(true)
	,bIsInstancedStereoEnabled(false)
	,post_trans_registered(false)
	,PostOpaqueEnabled(true)
	,PostTranslucentEnabled(true)
	,OverlaysEnabled(true)
	,AtmosphericsTexture(nullptr)
	,PathEnv(nullptr)
	,CurrentRHIType(GraphicsDeviceUnknown)
	,RenderingEnabled(false)
	,RendererInitialized(false)
	,WaterRenderingEnabled(false)
	,CachedDeltaSeconds(0.0f)
	,AutoSaveTimer(0.0f)
	,actorPropertiesChanged(true)
	,haveEditor(false)
	,exportNext(false)
	,sequenceInUse(nullptr)
	,LastFrameNumber(-1)
	,LastStereoFrameNumber(-1)
{
	simulVersion=ToSimulVersion(0,0,0);
	if(Instance)
		UE_LOG(TrueSky, Warning, TEXT("Instance is already set!"));
	if(!actorCrossThreadProperties)
		actorCrossThreadProperties=new ActorCrossThreadProperties;
	
	InitPaths();
	Instance = this;
#ifdef SHARED_FROM_THIS
TSharedRef< FTrueSkyPlugin,(ESPMode::Type)0 > ref=AsShared();
#endif
#if PLATFORM_WINDOWS || PLATFORM_UWP
	GlobalOverlayFlag=true;
#endif
	AutoSaveTimer = 0.0f;
	//we need to pass through real DeltaSecond; from our scene Actor?
	CachedDeltaSeconds = 0.0333f;
}

FTrueSkyPlugin::~FTrueSkyPlugin()
{
	Instance = nullptr;
	delete actorCrossThreadProperties;
	actorCrossThreadProperties=nullptr;
}

bool FTrueSkyPlugin::SupportsDynamicReloading()
{
	return true;
}

void *FTrueSkyPlugin::GetRenderEnvironment()
{
	if( StaticGetEnvironment != nullptr )
	{
		return StaticGetEnvironment();
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to call StaticGetEnvironment before it has been set"));
		return nullptr;
	}
}

void FTrueSkyPlugin::LogCallback(const char *txt)
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
		UE_LOG(TrueSky,Error,TEXT("%s"), *substr);
	}
	else if(substr.Contains("warning"))
	{
		UE_LOG(TrueSky,Warning,TEXT("%s"), *substr);
	}
	else
	{
		UE_LOG(TrueSky,Display,TEXT("%s"), *substr);
	}
}

void FTrueSkyPlugin::AdaptScaledMatrix(FMatrix &sMatrix)
{
	::AdaptScaledMatrix(sMatrix,actorCrossThreadProperties->MetresPerUnit,actorCrossThreadProperties->Transform.ToMatrixWithScale());
}

void FTrueSkyPlugin::AdaptViewMatrix(FMatrix &viewMatrix,bool editor_version) const
{
	::AdaptViewMatrix(viewMatrix,actorCrossThreadProperties->MetresPerUnit,actorCrossThreadProperties->Transform.ToMatrixWithScale());
}

bool FTrueSkyPlugin::TriggerAction(const char *name)
{
	if( StaticTriggerAction != nullptr )
	{
		return StaticTriggerAction( name );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to TriggerAction before it has been set"));
	}
	return false;
}

bool FTrueSkyPlugin::TriggerAction(const FString &fname)
{
	std::string name=FStringToUtf8(fname);
	if( StaticTriggerAction != nullptr )
	{
		return StaticTriggerAction( name.c_str() );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to TriggerAction before it has been set"));
	}
	return false;
}
	
void FTrueSkyPlugin::SetRenderBool(const FString &fname, bool value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderBool != nullptr )
	{
		StaticSetRenderBool(name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render bool before StaticSetRenderBool has been set"));
	}
}

bool FTrueSkyPlugin::GetRenderBool(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderBool != nullptr )
	{
		return StaticGetRenderBool( name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render bool before StaticGetRenderBool has been set"));
	return false;
}

void FTrueSkyPlugin::SetRenderFloat(const FString &fname, float value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderFloat != nullptr )
	{
		StaticSetRenderFloat( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render float before StaticSetRenderFloat has been set"));
	}
}

float FTrueSkyPlugin::GetRenderFloat(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderFloat != nullptr )
	{
		return StaticGetRenderFloat( name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render float before StaticGetRenderFloat has been set"));
	return 0.0f;
}

	

void FTrueSkyPlugin::SetRenderInt(const FString &fname, int value)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRenderInt != nullptr )
	{
		StaticSetRenderInt( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render int before StaticSetRenderInt has been set"));
	}
}

void FTrueSkyPlugin::SetRender(const FString &fname, const TArray<FVariant> &params)
{
	std::string name=FStringToUtf8(fname);
	if( StaticSetRender != nullptr )
	{
		VariantPass varlist[6];
		int num_params=params.Num();
		if(num_params>5)
		{
			UE_LOG_ONCE(TrueSky, Warning, TEXT("Too many parameters."));
			return ;
		}
		for(int i=0;i<num_params;i++)
		{
			if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Int64)
				varlist[i].intVal=params[i].GetValue<int64>();
			else if(params[i].GetType()==EVariantTypes::Float)
				varlist[i].floatVal=params[i].GetValue<float>();
			else if(params[i].GetType()==EVariantTypes::Double)
				varlist[i].floatVal=params[i].GetValue<double>();
			else
			{
				UE_LOG_ONCE(TrueSky, Warning, TEXT("Unsupported variant type."));
				return ;
			}
		}
		//varlist[num_params].intVal=0;
		StaticSetRender( name.c_str(), num_params,varlist);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render int before StaticSetRenderInt has been set"));
	}
}
int FTrueSkyPlugin::GetRenderInt(const FString &fname) const
{
	TArray<FVariant> params;
	return GetRenderInt(fname,params);
}

int64 FTrueSkyPlugin::GetEnum(const FString &fname) const
{
	std::string name=FStringToUtf8(fname);
	if(StaticGetEnum)
		return StaticGetEnum(name.c_str());
	else
		return 0;
}

simul::unreal::RenderingInterface *FTrueSkyPlugin::GetRenderingInterface()
{
	if(StaticGetRenderingInterface)
	{
		return StaticGetRenderingInterface();
	}
	else
		return nullptr;
}

int FTrueSkyPlugin::GetRenderInt(const FString &fname,const TArray<FVariant> &params) const
{
	std::string name=FStringToUtf8(fname);
	if( StaticGetRenderInt != nullptr )
	{
		VariantPass varlist[6];
		int num_params=params.Num();
		if(num_params>5)
		{
			UE_LOG_ONCE(TrueSky, Warning, TEXT("Too many parameters."));
			return 0;
		}
		for(int i=0;i<num_params;i++)
		{
			if(params[i].GetType()==EVariantTypes::Int32)
				varlist[i].intVal=params[i].GetValue<int32>();
			else if(params[i].GetType()==EVariantTypes::Int64)
				varlist[i].intVal=params[i].GetValue<int64>();
			else if(params[i].GetType()==EVariantTypes::Float)
				varlist[i].floatVal=params[i].GetValue<float>();
			else if(params[i].GetType()==EVariantTypes::Double)
				varlist[i].floatVal=params[i].GetValue<double>();
			else
			{
				UE_LOG_ONCE(TrueSky, Warning, TEXT("Unsupported variant type."));
				return 0;
			}
		}
		//varlist[num_params].intVal=0;
		return StaticGetRenderInt( name.c_str(),num_params,varlist);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render float before StaticGetRenderInt has been set"));
	return 0;
}

void FTrueSkyPlugin::SetRenderString(const FString &fname, const FString &value)
{
	if( StaticSetRenderString != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		StaticSetRenderString( name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set render string before StaticSetRenderString has been set"));
	}
}

FString FTrueSkyPlugin::GetRenderString(const FString &fname) const
{
	if( StaticGetRenderString != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		static char txt[14500];
		StaticGetRenderString( name.c_str(),txt,14500);
		return FString(txt);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get render string before StaticGetRenderString has been set"));
	return "";
}

void FTrueSkyPlugin::CreateBoundedWaterObject(unsigned int ID, float* dimension, float* location)
{
	if (StaticCreateBoundedWaterObject != nullptr && waterActorsCrossThreadProperties[ID]->Render)
	{
		waterActorsCrossThreadProperties[ID]->BoundedObjectCreated = StaticCreateBoundedWaterObject(ID, dimension, location);
	}
}

void FTrueSkyPlugin::RemoveBoundedWaterObject(unsigned int ID)
{
	if (StaticRemoveBoundedWaterObject != nullptr)
	{
		StaticRemoveBoundedWaterObject(ID);
		waterActorsCrossThreadProperties[ID]->BoundedObjectCreated = false;
	}
	else
	{
		//UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to remove water object before StaticRemoveBoundedWaterObject has been set"));
	}
} 

bool FTrueSkyPlugin::AddWaterProbe(int ID, FVector location, FVector velocity, float radius)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;

	WaterProbeValues probeValues;
	probeValues.ID = ID;
	memcpy(&probeValues.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&probeValues.velocity, (const float*)(&velocity), 3 * sizeof(float));
	probeValues.radius = radius * actorCrossThreadProperties->MetresPerUnit;

	if (StaticAddWaterProbe != nullptr)
		return StaticAddWaterProbe(&probeValues);

	return false;
}

void FTrueSkyPlugin::RemoveWaterProbe(int ID)
{
	if (StaticRemoveWaterProbe != nullptr)
		StaticRemoveWaterProbe(ID);
}

void FTrueSkyPlugin::UpdateWaterProbeValues(int ID, FVector location, FVector velocity, float radius, float dEnergy)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;
	FVector tempVelocity = velocity * actorCrossThreadProperties->MetresPerUnit;
	WaterProbeValues probeValues;
	probeValues.ID = ID;
	memcpy(&probeValues.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&probeValues.velocity, (const float*)(&tempVelocity), 3 * sizeof(float));
	probeValues.radius = radius * actorCrossThreadProperties->MetresPerUnit;
	probeValues.dEnergy = dEnergy;
	if (StaticUpdateWaterProbeValues != nullptr)
		StaticUpdateWaterProbeValues(&probeValues);
}

FVector4 FTrueSkyPlugin::GetWaterProbeValues(int ID)
{
	if (StaticGetWaterProbeValues != nullptr)
	{
		vec4 result;// = StaticGetWaterProbeValues(ID);
		StaticGetWaterProbeValues(ID, &result);
		return FVector4(result.x, result.y, result.z, result.w);
	}

	else return FVector4(0.0, 0.0, 0.0, 0.0);
}

bool FTrueSkyPlugin::AddWaterBuoyancyObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;
	WaterMeshObjectValues buoyancyObjectValues;
	buoyancyObjectValues.ID = ID;
	memcpy(&buoyancyObjectValues.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&buoyancyObjectValues.rotation, (const float*)(&rotation), 4 * sizeof(float));
	memcpy(&buoyancyObjectValues.scale, (const float*)(&scale), 3 * sizeof(float));
	buoyancyObjectValues.noOfVertices = vertices.Num();
	buoyancyObjectValues.vertices = (const float*)(vertices.GetData());

	if(StaticAddWaterBuoyancyObject != nullptr)
		return StaticAddWaterBuoyancyObject(&buoyancyObjectValues);

	return false;
}

void FTrueSkyPlugin::UpdateWaterBuoyancyObjectValues(int ID, FVector location, FQuat rotation, FVector scale)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;
	WaterMeshObjectValues buoyancyObjectValues;
	buoyancyObjectValues.ID = ID;
	memcpy(&buoyancyObjectValues.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&buoyancyObjectValues.rotation, (const float*)(&rotation), 4 * sizeof(float));
	memcpy(&buoyancyObjectValues.scale, (const float*)(&scale), 3 * sizeof(float));
	buoyancyObjectValues.noOfVertices = 0;
	buoyancyObjectValues.vertices = nullptr;

	if(StaticUpdateWaterBuoyancyObjectValues != nullptr)
		return StaticUpdateWaterBuoyancyObjectValues(&buoyancyObjectValues);
}

TArray<float> FTrueSkyPlugin::GetWaterBuoyancyObjectResults(int ID, int noOfVertices)
{
	TArray<float> finalResults;
	float* results = nullptr;// new float[noOfVertices * sizeof(float)];

	if(StaticGetWaterBuoyancyObjectResults != nullptr)
	{
		results = StaticGetWaterBuoyancyObjectResults(ID);
		if(results != nullptr)
			for (int i = 0; i < noOfVertices; i++)
			{
				finalResults.Add(results[i] / actorCrossThreadProperties->MetresPerUnit);
			}
	}
	
	return finalResults;
}

void FTrueSkyPlugin::RemoveWaterBuoyancyObject(int ID)
{
	if(StaticRemoveWaterBuoyancyObject != nullptr)
		return StaticRemoveWaterBuoyancyObject(ID);
}

bool FTrueSkyPlugin::AddWaterMaskObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices, TArray<uint32> indices, maskObjectType objectType, bool active, bool totalMask)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;
	waterMaskingObject maskObjectValues;
	maskObjectValues.values.ID = ID;
	memcpy(&maskObjectValues.values.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&maskObjectValues.values.rotation, (const float*)(&rotation), 4 * sizeof(float));
	memcpy(&maskObjectValues.values.scale, (const float*)(&scale), 3 * sizeof(float));
	maskObjectValues.values.noOfVertices = vertices.Num();
	maskObjectValues.values.noOfIndices = indices.Num();
	maskObjectValues.objectType = objectType;
	if (maskObjectValues.objectType == custom && vertices.Num() != 0 && indices.Num() != 0)
	{
		maskObjectValues.values.vertices = (const float*)(vertices.GetData());
		maskObjectValues.values.indices = (const uint32*)(indices.GetData());
	}
	maskObjectValues.active = active;
	maskObjectValues.totalMask = totalMask;
	if (StaticAddWaterMaskObject != nullptr)
		return StaticAddWaterMaskObject(&maskObjectValues);

	return false;
}

void FTrueSkyPlugin::UpdateWaterMaskObjectValues(int ID, FVector location, FQuat rotation, FVector scale, maskObjectType objectType, bool active, bool totalMask)
{
	FVector tempLocation = (actorCrossThreadProperties->WaterOffset + location) * actorCrossThreadProperties->MetresPerUnit;
	waterMaskingObject maskObjectValues;
	maskObjectValues.values.ID = ID;
	memcpy(&maskObjectValues.values.location, (const float*)(&tempLocation), 3 * sizeof(float));
	memcpy(&maskObjectValues.values.rotation, (const float*)(&rotation), 4 * sizeof(float));
	memcpy(&maskObjectValues.values.scale, (const float*)(&scale), 3 * sizeof(float));
	maskObjectValues.values.noOfVertices = 0;
	maskObjectValues.values.vertices = nullptr;
	maskObjectValues.values.noOfIndices = 0;
	maskObjectValues.values.indices = nullptr;
	maskObjectValues.objectType = objectType;
	maskObjectValues.active = active;
	maskObjectValues.totalMask = totalMask;

	if (StaticUpdateWaterMaskObjectValues != nullptr)
		return StaticUpdateWaterMaskObjectValues(&maskObjectValues);
}


void FTrueSkyPlugin::RemoveWaterMaskObject(int ID)
{
	if (StaticRemoveWaterMaskObject != nullptr)
		return StaticRemoveWaterMaskObject(ID);
}

void FTrueSkyPlugin::SetWaterFloat(const FString &fname, unsigned int ID, float value)
{	
	if (StaticSetWaterFloat != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterFloat(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water float before StaticSetWaterFloat has been set"));
	}
}

void FTrueSkyPlugin::SetWaterInt(const FString &fname, unsigned int ID, int value)
{
	if (StaticSetWaterInt != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterInt(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water int before StaticSetWaterInt has been set"));
	}
}

void FTrueSkyPlugin::SetWaterBool(const FString &fname, unsigned int ID, bool value)
{
	if (StaticSetWaterBool != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterBool(name.c_str(), ID, value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water bool before StaticSetWaterBool has been set"));
	}
}

void FTrueSkyPlugin::SetWaterVector(const FString &fname, int ID, const FVector &value)
{
	if (StaticSetWaterVector != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetWaterVector(name.c_str(), ID, (const float*)&value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set water vector before StaticSetWaterVector has been set"));
	}
}

float FTrueSkyPlugin::GetWaterFloat(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterFloat != nullptr)
	{
		return StaticGetWaterFloat(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water float before StaticGetWaterFloat has been set"));
	return 0.0f;
}

int FTrueSkyPlugin::GetWaterInt(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterInt != nullptr)
	{
		return StaticGetWaterInt(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water int before StaticGetWaterInt has been set"));
	return 0;
}

bool FTrueSkyPlugin::GetWaterBool(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	if (StaticGetWaterFloat != nullptr)
	{
		return StaticGetWaterBool(name.c_str(), ID);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water bool before StaticGetWaterBool has been set"));
	return false;
}

FVector FTrueSkyPlugin::GetWaterVector(const FString &fname, unsigned int ID) const
{
	std::string name = FStringToUtf8(fname);
	FVector value;
	if (StaticGetWaterVector != nullptr)
	{
		StaticGetWaterVector(name.c_str(), ID, (float*)&value);
		return value;
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get water vector before StaticGetWaterVector has been set"));
	return value;
}

void FTrueSkyPlugin::SetKeyframerInt(unsigned uid, const FString& fname, int value)
{
	if (StaticSetKeyframerInt != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetKeyframerInt(uid, name.c_str(), value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticSetKeyframerInt has been set"));
	}
}

int FTrueSkyPlugin::GetKeyframerInt(unsigned uid, const FString& fname) const
{
	if (StaticSetKeyframerInt != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticGetKeyframerInt(uid, name.c_str());
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticGetKeyframerInt has been set"));
	}
	return 0;
}

void FTrueSkyPlugin::SetKeyframerFloat(unsigned uid, const FString& fname, float value)
{
	if (StaticSetKeyframerFloat != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		StaticSetKeyframerFloat(uid, name.c_str(), value);
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticSetKeyframerFloat has been set"));
	}
}

float FTrueSkyPlugin::GetKeyframerFloat(unsigned uid, const FString& fname) const
{
	if (StaticGetKeyframerFloat != nullptr)
	{
		std::string name = FStringToUtf8(fname);
		return StaticGetKeyframerFloat(uid, name.c_str());
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticGetKeyframerFloat has been set"));
	}
	return 0.0f;
}

void FTrueSkyPlugin::SetKeyframeFloat(unsigned uid,const FString &fname, float value)
{
	if( StaticSetKeyframeFloat != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		StaticSetKeyframeFloat(uid,name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe float before StaticSetKeyframeFloat has been set"));
	}
}

float FTrueSkyPlugin::GetKeyframeFloat(unsigned uid,const FString &fname) const
{
	if( StaticGetKeyframeFloat != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		return StaticGetKeyframeFloat( uid,name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get Keyframe float before StaticGetKeyframeFloat has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetRenderFloatAtPosition(const FString &fname,FVector pos) const
{
	if( StaticGetRenderFloatAtPosition != nullptr )
	{
		std::string name=FStringToUtf8(fname);
		ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
		pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,pos);
		return StaticGetRenderFloatAtPosition( name.c_str(),(const float*)&pos);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetRenderFloatAtPosition has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetFloatAtPosition(int64 Enum,FVector pos,int32 uid) const
{
	if( StaticGetFloatAtPosition != nullptr )
	{
		ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
		pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,pos);
		return StaticGetFloatAtPosition( Enum,(const float*)&pos,uid);
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetFloatAtPosition has been set"));
	return 0.0f;
}

float FTrueSkyPlugin::GetFloat(int64 Enum) const
{
	if( StaticGet != nullptr )
	{
		float f;
		StaticGet(Enum,(VariantPass*)&f);
		return f;
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get float at position before StaticGetFloatAtPosition has been set"));
	return 0.0f;
}

FVector FTrueSkyPlugin::GetVector(int64 TrueSkyEnum) const
{
	FVector v;
	if( StaticGet != nullptr )
	{
		StaticGet(TrueSkyEnum,(VariantPass*)&v);
		return v;
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get a vector before StaticGet has been set"));
	return v;
}

void FTrueSkyPlugin::SetVector(int64 TrueSkyEnum,FVector v) const
{
	if( StaticSet != nullptr )
	{
		StaticSet(TrueSkyEnum,(const VariantPass*)&v);
	}
	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a vector before SetVector has been set"));
}

void FTrueSkyPlugin::SetInt(ETrueSkyPropertiesInt Property, int32 Value)
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyIntMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesIntIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyIntMaps[ATrueSkySequenceActor::TSPropertiesIntIdx].Find((int64)Property);
		if (((p->TrueSkyEnum > 0) && ((p->PropertyValue - Value) != 0)) || !p->Initialized)
		{
			p->PropertyValue = Value;
			p->Initialized = true;
			StaticSet(p->TrueSkyEnum, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set an int property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetVector(ETrueSkyPropertiesVector Property, FVector Value)
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyVectorMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesVectorIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyVectorMaps[ATrueSkySequenceActor::TSPropertiesVectorIdx].Find((int64)Property);
		if(((p->TrueSkyEnum > 0) && (!(p->PropertyValue - Value).IsNearlyZero())) || !p->Initialized)
		{
			p->PropertyValue = Value;
			p->Initialized = true;
			StaticSet(p->TrueSkyEnum, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a vector property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetFloat(ETrueSkyPropertiesFloat Property, float Value)
{
	const int32 arrElements = ATrueSkySequenceActor::TrueSkyPropertyFloatMaps.Num();
	if (arrElements && ATrueSkySequenceActor::TSPropertiesFloatIdx <= arrElements - 1)
	{
		auto p = ATrueSkySequenceActor::TrueSkyPropertyFloatMaps[ATrueSkySequenceActor::TSPropertiesFloatIdx].Find((int64)Property);
		if (((p->TrueSkyEnum > 0) && (!FMath::IsNearlyZero(p->PropertyValue - Value)))||!p->Initialized)
		{
			StaticSet(p->TrueSkyEnum, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a float property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetWaterInt(ETrueSkyWaterPropertiesInt Property, int ID, int32 Value)
{
	const int32 arrElements = ATrueSkyWater::TrueSkyWaterPropertyIntMaps.Num();
	if (arrElements && ATrueSkyWater::TSWaterPropertiesIntIdx <= arrElements - 1)
	{
		auto p = ATrueSkyWater::TrueSkyWaterPropertyIntMaps[ATrueSkyWater::TSWaterPropertiesIntIdx].Find((int64)Property);
		if ((p->TrueSkyEnum > 0) || !p->Initialized)
		{
			p->PropertyValue = Value;
			p->Initialized = true;
			StaticWaterSet(p->TrueSkyEnum, ID, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set an int property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetWaterVector(ETrueSkyWaterPropertiesVector Property, int ID, const FVector &Value)
{
	const int32 arrElements = ATrueSkyWater::TrueSkyWaterPropertyVectorMaps.Num();
	if (arrElements && ATrueSkyWater::TSWaterPropertiesVectorIdx <= arrElements - 1)
	{
		auto p = ATrueSkyWater::TrueSkyWaterPropertyVectorMaps[ATrueSkyWater::TSWaterPropertiesVectorIdx].Find((int64)Property);
		if ((p->TrueSkyEnum > 0) || !p->Initialized)
		{
			p->PropertyValue = Value;
			p->Initialized = true;
			StaticWaterSet(p->TrueSkyEnum, ID, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a vector property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetWaterFloat(ETrueSkyWaterPropertiesFloat Property, int ID, float Value)
{
	const int32 arrElements = ATrueSkyWater::TrueSkyWaterPropertyFloatMaps.Num();
	if (arrElements && ATrueSkyWater::TSWaterPropertiesFloatIdx <= arrElements - 1)
	{
		auto p = ATrueSkyWater::TrueSkyWaterPropertyFloatMaps[ATrueSkyWater::TSWaterPropertiesFloatIdx].Find((int64)Property);
		if ((p->TrueSkyEnum > 0) || !p->Initialized)
		{
			p->PropertyValue = Value;
			p->Initialized = true;
			StaticWaterSet(p->TrueSkyEnum, ID, (VariantPass*)&Value);
		}
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set a float property but the index is out of range or the array is empty!"));
	}
}

void FTrueSkyPlugin::SetKeyframeInt(unsigned uid,const FString &fname, int value)
{
	if( StaticSetKeyframeInt != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		StaticSetKeyframeInt( uid,name.c_str(), value );
	}
	else
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to set Keyframe int before StaticSetKeyframeInt has been set"));
	}
}

int FTrueSkyPlugin::GetKeyframeInt(unsigned uid,const FString &fname) const
{
	if( StaticGetKeyframeInt != nullptr )
	{
	std::string name=FStringToUtf8(fname);
		return StaticGetKeyframeInt( uid,name.c_str() );
	}

	UE_LOG_ONCE(TrueSky, Warning, TEXT("Trying to get Keyframe int before StaticGetKeyframeInt has been set"));
	return 0;
}

/** Tickable object interface */
void FTrueSkyTickable::Tick( float DeltaTime )
{
	//if(FTrueSkyPlugin::Instance)
	//	FTrueSkyPlugin::Instance->Tick(DeltaTime);
}

bool FTrueSkyTickable::IsTickable() const
{
	return true;
}

TStatId FTrueSkyTickable::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTrueSkyTickable, STATGROUP_Tickables);
}


void FTrueSkyPlugin::StartupModule()
{
	if(FModuleManager::Get().IsModuleLoaded("MainFrame") )
		haveEditor=true;
	FCoreDelegates::OnPreExit.AddRaw(this, &FTrueSkyPlugin::PreExit);

	GetRendererModule().RegisterPostOpaqueRenderDelegate(FRenderDelegate::CreateRaw(this, &FTrueSkyPlugin::DelegatedRenderFrame));

	post_trans_registered = true;
	GetRendererModule().RegisterOverlayRenderDelegate( FRenderDelegate::CreateRaw(this, &FTrueSkyPlugin::DelegatedRenderPostTranslucent) );
#if WITH_EDITOR
	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "trueSKY",
			LOCTEXT("TrueSkySettingsName", "trueSKY"),
			LOCTEXT("TrueSkySettingsDescription", "Configure the trueSKY plug-in."),
			GetMutableDefault<UTrueSkySettings>()
		);

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FTrueSkyPlugin::HandleSettingsSaved);
		}
	}
#endif //WITH_EDITOR

	RenderingEnabled					=false;
	RendererInitialized					=false;
	WaterRenderingEnabled				=false;
	TrueSkyEditorCallback				=nullptr;
	StaticInitInterface					=nullptr;
	GetSimulVersion						=nullptr;
	StaticSetMemoryInterface			=nullptr;
	StaticShutDownInterface				=nullptr;
	StaticSetDebugOutputCallback		=nullptr;
	StaticSetGraphicsDevice				=nullptr;
	StaticSetGraphicsDeviceAndContext	=nullptr;
	StaticPushPath						=nullptr;
	StaticSetFileLoader					=nullptr;
	StaticGetOrAddView					=nullptr;
	StaticRenderFrame					=nullptr;
	StaticRenderFrame2					=nullptr;
	StaticCopySkylight					=nullptr;
	StaticCopySkylight2					=nullptr;
	StaticCopySkylight3					=nullptr;
	StaticProbeSkylight					=nullptr;
	StaticRenderOverlays				=nullptr;
	StaticRenderOverlays2				=nullptr;
	StaticTick							=nullptr;
	StaticOnDeviceChanged				=nullptr;
	StaticGetEnvironment				=nullptr;
	StaticSetSequence					=nullptr;

	StaticSetSequence2					=nullptr;
	StaticSetMoon						=nullptr;

	StaticGetRenderInterfaceInstance	=nullptr;
	StaticSetPointLight					=nullptr;
	StaticCloudPointQuery				=nullptr;
	StaticLightingQuery					=nullptr;
	StaticCloudLineQuery				=nullptr;
	StaticSetRenderTexture				=nullptr;
	StaticSetRenderTexture2				=nullptr;
	StaticSetMatrix4x4					=nullptr;
	StaticSetRenderBool					=nullptr;
	StaticSetRender						=nullptr;
	StaticGetRenderBool					=nullptr;
	StaticTriggerAction					=nullptr;

	StaticSetRenderFloat				=nullptr;
	StaticGetRenderFloat				=nullptr;
	StaticSetRenderInt					=nullptr;
	StaticGetRender						=nullptr;
	StaticGetRenderInt					=nullptr;	//Milestone
	
	StaticSetRenderString				=nullptr;
	StaticGetRenderString				=nullptr;
										
	StaticCreateBoundedWaterObject		=nullptr;
	StaticRemoveBoundedWaterObject		=nullptr;

	StaticAddWaterProbe					=nullptr;
	StaticRemoveWaterProbe				=nullptr;
	StaticUpdateWaterProbeValues		=nullptr;
	StaticGetWaterProbeValues			=nullptr;

	StaticAddWaterBuoyancyObject		=nullptr;
	StaticUpdateWaterBuoyancyObjectValues=nullptr;
	StaticGetWaterBuoyancyObjectResults	=nullptr;
	StaticRemoveWaterBuoyancyObject		=nullptr;

	StaticAddWaterMaskObject			=nullptr;
	StaticUpdateWaterMaskObjectValues	=nullptr;
	StaticRemoveWaterMaskObject			=nullptr;

	StaticSetWaterFloat					=nullptr;
	StaticSetWaterInt					=nullptr;
	StaticSetWaterBool					=nullptr;
	StaticSetWaterVector				=nullptr;
	StaticGetWaterFloat					=nullptr;
	StaticGetWaterInt					=nullptr;
	StaticGetWaterBool					=nullptr;
	StaticGetWaterVector				=nullptr;
										
	StaticExportCloudLayerToGeometry	=nullptr;
										
	StaticSetKeyframeFloat				=nullptr;
	StaticGetKeyframeFloat				=nullptr;
	StaticSetKeyframeInt				=nullptr;
	StaticGetKeyframeInt				=nullptr;


	StaticSetKeyframerFloat				=nullptr;
	StaticGetKeyframerFloat				=nullptr;
	StaticSetKeyframerInt				=nullptr;
	StaticGetKeyframerInt				=nullptr;


	StaticGetLightningBolts				=nullptr;
	StaticSpawnLightning				=nullptr;

	StaticGetRenderFloatAtPosition		=nullptr;

	StaticGetFloatAtPosition			=nullptr;
	StaticGet							=nullptr;
	StaticSet							=nullptr;
	StaticWaterSet						=nullptr;
	StaticGetEnum						=nullptr;
	StaticProcessQueries				=nullptr;
		
	StaticFillColourTable				=nullptr;
	StaticGetRenderingInterface			=nullptr;
	PathEnv = nullptr;

	// Perform the initialization earlier on the game thread
	EnsureRendererIsInitialized();
}
//FGraphEventRef tsFence;
#if PLATFORM_PS4
#endif

void FTrueSkyPlugin::SetRenderingEnabled( bool Enabled )
{
	RenderingEnabled = Enabled;
}

void FTrueSkyPlugin::SetPostOpaqueEnabled(bool Enabled )
{
	PostOpaqueEnabled = Enabled;
}

void FTrueSkyPlugin::SetPostTranslucentEnabled(bool Enabled )
{
	PostTranslucentEnabled = Enabled;
}

void FTrueSkyPlugin::SetOverlaysEnabled(bool Enabled )
{
	OverlaysEnabled = Enabled;
}


void FTrueSkyPlugin::SetWaterRenderingEnabled( bool Enabled)
{
	WaterRenderingEnabled = Enabled;
}

bool FTrueSkyPlugin::GetWaterRenderingEnabled()
{
	return WaterRenderingEnabled;
}

struct FRHIRenderOverlaysCommand : public FRHICommand<FRHIRenderOverlaysCommand>
{
	FTrueSkyRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIRenderOverlaysCommand(FRenderDelegateParameters p,
												FTrueSkyPlugin *d,uint64_t u,int f)
												:RenderParameters(p, p.RHICmdList, (FSceneRenderTargets::Get(*p.RHICmdList)).GetSceneColorSurface()->GetTexture2D())
												,TrueSkyPlugin(d)
												,uid(u)
												,FrameNumber(f)
	{
	}
	virtual ~FRHIRenderOverlaysCommand()
	{
	}
    void Execute(FRHICommandListBase& CmdList)
	{
		RenderParameters.CurrentCmdList = (FRHICommandList*)&CmdList;
	//	TrueSkyPlugin->RenderOverlays(uid,RenderParameters, FrameNumber);
	}
};

void FTrueSkyPlugin::DelegatedRenderOverlays(FRenderDelegateParameters& RenderParameters)
{	
	return; // This is disabled due to being handled elsewhere.
}

void FTrueSkyPlugin::RenderOverlays(uint64_t uid,FTrueSkyRenderDelegateParameters& RenderParameters,int FrameNumber)
{	
#if TRUESKY_PLATFORM_SUPPORTED
	if(!GlobalOverlayFlag)
		return;
	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
	UpdateFromActor(RenderParameters);
	if(!RendererInitialized )
		return;
	
	StoreUEState(RenderParameters, CurrentRHIType);

	//[Milestone]
	IRHICommandContext *CommandListContext = &RenderParameters.CurrentCmdList->GetContext();

#if !UE_BUILD_SHIPPING	//[Milestone]
	CommandListContext->RHIPushEvent(TEXT("TrueSky::Overlays"), FColor::Black);
#endif

	Viewport viewports[3];
	viewports[0].x=RenderParameters.ViewportRect.Min.X;
	viewports[0].y=RenderParameters.ViewportRect.Min.Y;
	viewports[0].w=RenderParameters.ViewportRect.Width();
	viewports[0].h=RenderParameters.ViewportRect.Height();
	// VR Viewports:
	for(int i=1;i<3;i++)
	{
		viewports[i].x=(i==2?RenderParameters.ViewportRect.Width()/2:0);
		viewports[i].y=RenderParameters.ViewportRect.Min.Y;
		viewports[i].w=RenderParameters.ViewportRect.Width()/2;
		viewports[i].h=RenderParameters.ViewportRect.Height();
	}

	void* device        = GetPlatformDevice(CurrentRHIType);
	void* pContext		= GetPlatformContext(RenderParameters, CurrentRHIType);
	void* depthTexture	= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);

	void* colourTarget	= nullptr;
	colourTarget	=GetPlatformRenderTarget(static_cast<FRHITexture2D*>(actorCrossThreadProperties->OverlayRT.GetReference()), CurrentRHIType);

	//[Milestone]
	//IRHICommandContext *CommandListContext=(IRHICommandContext*)(&RenderParameters.CurrentCmdList->GetContext());

    int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..
	static int overlay_id=0;
	FMatrix projMatrix = RenderParameters.ProjMatrix;
	projMatrix.M[2][3]	*=-1.0f;
	projMatrix.M[3][2]	*= actorCrossThreadProperties->MetresPerUnit;
	FMatrix viewMatrix=RenderParameters.ViewMatrix;
	// We want the transform FROM worldspace TO trueskyspace
	AdaptViewMatrix(viewMatrix);

#if SIMUL_SUPPORT_D3D12
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		OverlaysStoreStateD3D12(CommandListContext, RenderParameters.RenderTargetTexture,RenderParameters.DepthTexture);
	}
#endif
	if (StaticRenderOverlays2&&actorCrossThreadProperties->OverlayRT)
	{
		InitRenderFrameStruct(frameStruct, RenderParameters
			, static_cast<FRHITexture2D*>(actorCrossThreadProperties->OverlayRT.GetReference())
			, false
			, view_id
			, FrameNumber
			, UNREAL_STYLE
			, false);

		StaticRenderOverlays2(&frameStruct);
	}
	else
	{
		StaticRenderOverlays(device, pContext, depthTexture, &(viewMatrix.M[0][0]), &(projMatrix.M[0][0]), view_id, colourTarget, viewports);
	}

#if SIMUL_SUPPORT_D3D12
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		OverlaysRestoreStateD3D12(CommandListContext, RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture);
	}
#endif

#if !UE_BUILD_SHIPPING	//[Milestone]
	CommandListContext->RHIPopEvent();
#endif

	RestoreUEState(RenderParameters, CurrentRHIType);
	
#endif
}

struct FRHIPostOpaqueCommand: public FRHICommand<FRHIPostOpaqueCommand>
{
	FTrueSkyRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	bool bIncludePostTranslucent;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIPostOpaqueCommand(FRenderDelegateParameters p,
		FTrueSkyPlugin *d,uint64_t u,EStereoscopicPass s,bool m,int f,bool post_trans)
		:RenderParameters(p, p.RHICmdList, FSceneRenderTargets::Get(*p.RHICmdList).GetSceneColorSurface()->GetTexture2D())
		,TrueSkyPlugin(d)
		,uid(u)
		,StereoPass(s)
		,bMultiResEnabled(m)
		,bIncludePostTranslucent(post_trans)
		,FrameNumber(f)
	{
	}
	virtual ~FRHIPostOpaqueCommand()
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		if(TrueSkyPlugin)
		{
			RenderParameters.CurrentCmdList = (FRHICommandList*)&CmdList;
			TrueSkyPlugin->RenderFrame(uid,RenderParameters,StereoPass,bMultiResEnabled,FrameNumber);
		}
	}
};

struct FRHIPostTranslucentCommand: public FRHICommand<FRHIPostTranslucentCommand>
{
	FTrueSkyRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	int FrameNumber;
	FORCEINLINE_DEBUGGABLE FRHIPostTranslucentCommand(FRenderDelegateParameters p,
		FTrueSkyPlugin *d,uint64_t u,EStereoscopicPass s,bool m,int f)
		:RenderParameters(p, p.RHICmdList, FSceneRenderTargets::Get(*p.RHICmdList).GetSceneColorSurface()->GetTexture2D())
		,TrueSkyPlugin(d)
		,uid(u)
		,StereoPass(s)
		,bMultiResEnabled(m)
		,FrameNumber(f)
	{
	}
	virtual ~FRHIPostTranslucentCommand()
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		RenderParameters.CurrentCmdList = (FRHICommandList*)&CmdList;
		if(TrueSkyPlugin)
			TrueSkyPlugin->RenderPostTranslucent(uid,RenderParameters,StereoPass,bMultiResEnabled,FrameNumber,false);
	}
};
DECLARE_DWORD_COUNTER_STAT(TEXT("TrueSky Num Parallel Async Chains Links"), STAT_ParallelChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("TrueSky Wait for Parallel Async CmdList"), STAT_ParallelChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("TrueSky Parallel Async Chain Execute"), STAT_ParallelChainExecute, STATGROUP_RHICMDLIST);

struct FTrueSkyRHICommandWaitForAndSubmitSubListParallel final : public FRHICommand<FTrueSkyRHICommandWaitForAndSubmitSubListParallel>
{

	FGraphEventRef TranslateCompletionEvent;
	IRHICommandContextContainer* ContextContainer;
	int32 Num;
	int32 Index;
	GraphicsDeviceType CurrentRHIType;
	const TCHAR* Marker; //string literal
	FRHITexture2D *RenderTargetTexture;
	FORCEINLINE_DEBUGGABLE FTrueSkyRHICommandWaitForAndSubmitSubListParallel(FGraphEventRef& InTranslateCompletionEvent, IRHICommandContextContainer* InContextContainer, 
		int32 InNum, int32 InIndex, GraphicsDeviceType InCurrentRHIType, const TCHAR* InMarker/*string literal*/,FRHITexture2D *rtt)
		: TranslateCompletionEvent(InTranslateCompletionEvent)
		, ContextContainer(InContextContainer)
		, Num(InNum)
		, Index(InIndex)
		, CurrentRHIType(InCurrentRHIType)
		, Marker(InMarker ? InMarker : TEXT(""))
	{
		RenderTargetTexture = rtt;
		check(ContextContainer && Num);
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		check(ContextContainer && Num && IsInRHIThread());
		INC_DWORD_STAT_BY(STAT_ParallelChainLinkCount, 1);

		//Wait for the associated workerthread
		if (TranslateCompletionEvent.GetReference() && !TranslateCompletionEvent->IsComplete())
		{
			SCOPE_CYCLE_COUNTER(STAT_ParallelChainWait);
			if (IsInRenderingThread())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(TranslateCompletionEvent, ENamedThreads::GetRenderThread_Local());
			}
			else if (IsInRHIThread())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(TranslateCompletionEvent, IsRunningRHIInDedicatedThread() ? ENamedThreads::RHIThread : ENamedThreads::AnyThread);
			}
			else
			{
				check(0);
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_ParallelChainExecute);

			//Get the immediate context
			FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();

#if !UE_BUILD_SHIPPING
			//Push a marker
			CommandList.GetContext().RHIPushEvent(Marker, FColor::Black);
#endif

			//Send the deferred context to the device and free the context container.
			ContextContainer->SubmitAndFreeContextContainer(Index, Num);

#if !UE_BUILD_SHIPPING
			//Pop the marker
			CommandList.GetContext().RHIPopEvent();
#endif

			//Invalidate the immediate context
			FTrueSkyRenderDelegateParameters RenderParameters(FRenderDelegateParameters{}, &CommandList, RenderTargetTexture);
			RenderParameters.RHICmdList = &CommandList;//Just in case: currently, RestoreUEState only uses RenderParameters.CurrentCmdList.
			RestoreUEState(RenderParameters, CurrentRHIType);
		}
	}
};

DECLARE_CYCLE_STAT(TEXT("TrueSkyPostOpaque"), STAT_CLP_TrueSkyPostOpaque, STATGROUP_ParallelCommandListMarkers);
class FPostOpaqueAnyThreadTask : public FRenderTask
{

	FTrueSkyRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	bool bIncludePostTranslucent;
	int FrameNumber;
	IRHICommandContextContainer* ContextContainer;
public:
	FORCEINLINE_DEBUGGABLE FPostOpaqueAnyThreadTask(const FRenderDelegateParameters& p,
		FTrueSkyPlugin *d, uint64_t u, EStereoscopicPass s, bool m, int f, bool post_trans, IRHICommandContextContainer* InContextContainer,FRHITexture2D *rtt)
		:RenderParameters(p, nullptr,rtt)
		, TrueSkyPlugin(d)
		, uid(u)
		, StereoPass(s)
		, bMultiResEnabled(m)
		, bIncludePostTranslucent(post_trans)
		, FrameNumber(f)
		, ContextContainer(InContextContainer)
	{
	}
	virtual ~FPostOpaqueAnyThreadTask()
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FPostOpaqueAnyThreadTask, STATGROUP_TaskGraphTasks);
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SCOPE_CYCLE_COUNTER(STAT_CLP_TrueSkyPostOpaque);
		SCOPED_NAMED_EVENT(FTrueSkyPostOpaqueTask, FColor::Green);

		// MILESTONE TODO: optimize this code; don't allocate commandlist if not necessary.
		//				   Need to create the commandlist, since ContextContainer->SubmitAndFreeContextContainer expects a valid (maybe empty) commandlist.

		//Create a temporary commandlist.
		FRHICommandList* CmdList = new FRHICommandList(RenderParameters.RHICmdList->GetGPUMask());

		//Assign it the deferred context (GetContext() must be called in a workerthread).
		check(ContextContainer);
		CmdList->SetContext(ContextContainer->GetContext());

		if (TrueSkyPlugin)
		{
			//Set the active commandlist.
			RenderParameters.CurrentCmdList = CmdList;

			//Render TrueSky
			TrueSkyPlugin->RenderFrame(uid, RenderParameters, StereoPass, bMultiResEnabled, FrameNumber);
			if (bIncludePostTranslucent)
				TrueSkyPlugin->RenderPostTranslucent(uid, RenderParameters, StereoPass, bMultiResEnabled, FrameNumber, false);

			RenderParameters.CurrentCmdList = nullptr;
		}

		//Destroy the commandlist and finish the contextcontainer: it has to be called in the workerthread.
		delete CmdList;
		ContextContainer->FinishContext();
	}
};
DECLARE_CYCLE_STAT(TEXT("TrueSkyPostTranslucent"), STAT_CLP_TrueSkyPostTranslucent, STATGROUP_ParallelCommandListMarkers);

class FPostTranslucentAnyThreadTask : public FRenderTask
{

	FTrueSkyRenderDelegateParameters RenderParameters;
	FTrueSkyPlugin *TrueSkyPlugin;
	uint64_t uid;
	EStereoscopicPass StereoPass;
	bool bMultiResEnabled;
	int FrameNumber;
	bool bRenderInSeparateTranslucency;
	IRHICommandContextContainer* ContextContainer;
public:
	FORCEINLINE_DEBUGGABLE FPostTranslucentAnyThreadTask(const FRenderDelegateParameters& p,
		FTrueSkyPlugin *d, uint64_t u, EStereoscopicPass s, bool m, int f, IRHICommandContextContainer* InContextContainer, FRHITexture2D *RenderTexture)
		:RenderParameters(p, nullptr,RenderTexture)
		, TrueSkyPlugin(d)
		, uid(u)
		, StereoPass(s)
		, bMultiResEnabled(m)
		, FrameNumber(f)
		, ContextContainer(InContextContainer)
	{
	}
	virtual ~FPostTranslucentAnyThreadTask()
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FPostTranslucentAnyThreadTask, STATGROUP_TaskGraphTasks);
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SCOPE_CYCLE_COUNTER(STAT_CLP_TrueSkyPostTranslucent);
		SCOPED_NAMED_EVENT(FTrueSkyPostTranslucentTask, FColor::Green);

		// MILESTONE TODO: optimize this code; don't allocate commandlist if not necessary.
		//				   Need to create the commandlist, since ContextContainer->SubmitAndFreeContextContainer expects a valid (maybe empty) commandlist.
		
		//Create a temporary commandlist.
		FRHICommandList* CmdList = new FRHICommandList(RenderParameters.RHICmdList->GetGPUMask());

		//Assign it the deferred context (GetContext() must be called in a workerthread).
		check(ContextContainer);
		CmdList->SetContext(ContextContainer->GetContext());

		if (TrueSkyPlugin)
		{
			//Set the active commandlist.
			RenderParameters.CurrentCmdList = CmdList;

			//Render TrueSky
			TrueSkyPlugin->RenderPostTranslucent(uid, RenderParameters, StereoPass, bMultiResEnabled, FrameNumber,false);

			RenderParameters.CurrentCmdList = nullptr;
		}

		//Destroy the commandlist and finish the contextcontainer: it has to be called in the workerthread.
		delete CmdList;
		ContextContainer->FinishContext();
	};
};

bool IsTrueSkyParallelRenderingSupported()
{
	static IConsoleVariable* VarRHICmdUseDeferredContexts = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHICmdUseDeferredContexts"));
	return (UE_GAME && ParallelTrueSky && GRHICommandList.UseParallelAlgorithms() && GRHISupportsParallelRHIExecute && VarRHICmdUseDeferredContexts && (VarRHICmdUseDeferredContexts->GetInt() > 0));
}
//[Milestone] Parallel rendering: End

void FTrueSkyPlugin::DelegatedRenderPostTranslucent(FRenderDelegateParameters& RenderParameters)
{
	// We now render to texture for rain etc and composite in the UE materials system.
	return;
}
static crossplatform::ResourceState GetTextureResourceState(IRHICommandContext* cmdContext,FRHITexture* texture,GraphicsDeviceType api)
{
#if SIMUL_SUPPORT_D3D12
	if (api == GraphicsDeviceD3D12)
	{
		return GetD3D12TextureResourceState(*cmdContext,texture);
	}
#endif
#if SIMUL_SUPPORT_VULKAN
	if (api == GraphicsDeviceVulkan)
	{
		return GetVulkanTextureResourceState(cmdContext,texture);
	}
#endif
	return crossplatform::ResourceState::COMMON;
}

static void InitExternalTexture(IRHICommandContext* cmdContext, ExternalTexture& ext, FRHITexture* texture, GraphicsDeviceType api)
{
	void* native = GetPlatformTexturePtr(texture, api);
	ext.version = ext.static_version;
	ext.texturePtr = native;
	FIntVector size = texture ? texture->GetSizeXYZ() : FIntVector(0, 0, 0);
	ext.width = size.X;
	ext.height = size.Y;
	ext.depth = size.Z;
	ext.pixelFormat = texture ? ToSimulPixelFormat(texture->GetFormat(), api) : simul::crossplatform::PixelFormat::UNKNOWN;
	ext.sampleCount = texture ? texture->GetNumSamples():0;
	ext.resourceState=(texture&&cmdContext)?(unsigned)GetTextureResourceState(cmdContext,texture,api):0;
}

void FTrueSkyPlugin::InitRenderFrameStruct(RenderFrameStruct &f, FTrueSkyRenderDelegateParameters &RenderParameters
				, FRHITexture2D *TargetTexture
				, bool bMultiResEnabled
				, int view_id
				, int FrameNumber
				, PluginStyle style
				, bool isTranslucentPass) const
{
	
	static FMatrix viewMatrix;
	static FMatrix projMatrix;
	viewMatrix = RenderParameters.ViewMatrix;
	// We want the transform FROM worldspace TO trueskyspace
	AdaptViewMatrix(viewMatrix);
	projMatrix = RenderParameters.ProjMatrix;
	AdaptProjectionMatrix(projMatrix, actorCrossThreadProperties->MetresPerUnit);
	void *device = GetPlatformDevice(CurrentRHIType);
	void *pContext = GetPlatformContext(RenderParameters, CurrentRHIType);
	void* depthTexture = GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);
	void* colourTarget = GetColourTarget(RenderParameters, CurrentRHIType);
	void *colourTargetTexture = GetPlatformTexturePtr(TargetTexture, CurrentRHIType);
	colourTarget = (void*)GetPlatformRenderTarget(TargetTexture, CurrentRHIType);
	TArray<FVector2D>  MultiResConstants;
#ifdef NV_MULTIRES
	if (bMultiResEnabled)
	{
		SetMultiResConstants(MultiResConstants, View);
	}
#endif
	f.version = f.VERSION;
	f.device = device;
	f.pContext = pContext;
	f.view_id = view_id;
	f.viewMatrix4x4 = &(viewMatrix.M[0][0]);
	f.projMatrix4x4 = &(projMatrix.M[0][0]);
	f.depthTexture = depthTexture;
	f.colourTarget = colourTarget;
	// TODO: Implement colourSamples and depthSamples for non-D3D11 textures.
	f.colourSamples = 1;
	f.depthFormat = ToSimulPixelFormat(RenderParameters.DepthTexture->GetFormat(), CurrentRHIType);
	f.depthSamples = 1;
	f.colourFormat = simul::crossplatform::RGBA_32_FLOAT;
	if (TargetTexture)
		f.colourFormat = ToSimulPixelFormat(TargetTexture->GetFormat(), CurrentRHIType);
	f.colourTargetTexture = colourTargetTexture;
	if (colourTargetTexture != nullptr)
		f.isColourTargetTexture = true;
	else
		f.isColourTargetTexture = false;
	f.style = style;
	float exposure = actorCrossThreadProperties->Brightness;
	float gamma = actorCrossThreadProperties->Gamma;
	exposure *= actorCrossThreadProperties->SkyMultiplier; //Either  we have a seperate slider for sky brightess, Or we simply increase all birghtness values by a set amount.
	if (actorCrossThreadProperties->TrueSkyLightUnits == (int)ETrueSkyLightUnits::Photometric)
		exposure *= 12700.f; // lux watt conversion. See UpdateLights2 Function.
	f.exposure = exposure;
	f.gamma = gamma;
	f.framenumber = FrameNumber;
	f.nvMultiResConstants = (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr);
	colourTargetTexture = GetPlatformTexturePtr(TargetTexture, CurrentRHIType);
	colourTarget = (void*)GetPlatformRenderTarget(TargetTexture, CurrentRHIType);
	int X = RenderParameters.ViewportRect.Min.X;

	f.targetViewports[0].x = RenderParameters.ViewportRect.Min.X;
	f.targetViewports[0].y = RenderParameters.ViewportRect.Min.Y;
	f.targetViewports[0].w = RenderParameters.ViewportRect.Width();
	f.targetViewports[0].h = RenderParameters.ViewportRect.Height();
	if(isTranslucentPass)
	{
		f.targetViewports[0].x = 0;
		f.targetViewports[0].y = 0;
		if (f.targetViewports[0].w > int(TargetTexture->GetSizeX()))
		{
			f.targetViewports[0].h = int(float(TargetTexture->GetSizeX())*float(frameStruct.targetViewports[0].h) / float(frameStruct.targetViewports[0].w));
			f.targetViewports[0].w = int(TargetTexture->GetSizeX());
		}
		if (f.targetViewports[0].h > int(TargetTexture->GetSizeY()))
		{
			f.targetViewports[0].w = int(float(TargetTexture->GetSizeY())*float(frameStruct.targetViewports[0].w) / float(frameStruct.targetViewports[0].h));
			f.targetViewports[0].h = TargetTexture->GetSizeY();
		}
		if (f.targetViewports[0].w > int(TargetTexture->GetSizeX()))
			f.targetViewports[0].w = int(TargetTexture->GetSizeX());
	}

	f.depthViewports[0] = f.targetViewports[0];

	InitExternalTexture(&RenderParameters.RHICmdList->GetContext(),f.colour, TargetTexture, CurrentRHIType);
	InitExternalTexture(&RenderParameters.RHICmdList->GetContext(),f.depth, RenderParameters.DepthTexture, CurrentRHIType);

	{
	//	f.colour.resourceState=(unsigned short)GetTextureResourceState(&RenderParameters.RHICmdList->GetContext(),TargetTexture, CurrentRHIType);
	//	f.depth.resourceState=(unsigned short)GetTextureResourceState(&RenderParameters.RHICmdList->GetContext(),RenderParameters.DepthTexture, CurrentRHIType);
	}

}

void FTrueSkyPlugin::SetRenderTexture(FRHICommandListImmediate *RHICmdList,const char *name, FRHITexture* texture)
{
	void* native = GetPlatformTexturePtr(texture, CurrentRHIType);
	if (StaticSetRenderTexture2)
	{
		ExternalTexture ext;
		if(RHICmdList&&texture)
		{
			InitExternalTexture(&(RHICmdList->GetContext()),ext, texture, CurrentRHIType);
			ext.resourceState=(int)GetTextureResourceState(&(RHICmdList->GetContext()),texture,CurrentRHIType);
		}
		else
		{
			InitExternalTexture(nullptr,ext, nullptr, CurrentRHIType);
		}
		if(ext.resourceState==0xFFFF)
			return;
		StaticSetRenderTexture2(name, &ext);
	}
	else if (StaticSetRenderTexture)
	{
		StaticSetRenderTexture(name, native);
	}
}

void FTrueSkyPlugin::RenderPostTranslucent(uint64_t uid, FTrueSkyRenderDelegateParameters& RenderParameters
														,EStereoscopicPass StereoPass
														,bool bMultiResEnabled
														,int FrameNumber
														,bool clear)
{
#if TRUESKY_PLATFORM_SUPPORTED
	if(RenderParameters.Uid==0)
	{
		if(StaticShutDownInterface)
			StaticShutDownInterface();
		sequenceInUse=nullptr;
		return;
	}
	if(!actorCrossThreadProperties->Visible)
		return;
	ERRNO_CLEAR
	//check(IsInRenderingThread());
	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
	if(!RenderingEnabled)
		return;
	if(!StaticGetOrAddView)
		return;
    if (!GetActiveSequence())
        return;

	QUICK_SCOPE_CYCLE_COUNTER(TrueSkyPostTrans);

	void *device=GetPlatformDevice(CurrentRHIType);
	void *pContext=GetPlatformContext(RenderParameters, CurrentRHIType);
	PluginStyle style=UNREAL_STYLE;
	FSceneView *View=(FSceneView*)(RenderParameters.Uid);

	if(StereoPass==eSSP_LEFT_EYE)
		uid=29435;
	else if(StereoPass==eSSP_RIGHT_EYE)
		uid=29435;
    int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..

	FMatrix viewMatrix=RenderParameters.ViewMatrix;
	// We want the transform FROM worldspace TO trueskyspace
	AdaptViewMatrix(viewMatrix);
	FMatrix projMatrix	=RenderParameters.ProjMatrix;
	AdaptProjectionMatrix(projMatrix, actorCrossThreadProperties->MetresPerUnit);

	float exposure			= actorCrossThreadProperties->Brightness;
	float gamma				= actorCrossThreadProperties->Gamma;
	void* depthTexture		= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);
	void* colourTarget		= GetColourTarget(RenderParameters,CurrentRHIType);

	FRHITexture2D *TargetTexture=nullptr;
	TargetTexture = RenderParameters.RenderTargetTexture;
	void* colourTargetTexture = GetPlatformTexturePtr(TargetTexture, CurrentRHIType);

	//frameStruct.targetViewports[0].x = RenderParameters.ViewportRect.Min.X;
	//frameStruct.targetViewports[0].y = RenderParameters.ViewportRect.Min.Y;
	//frameStruct.targetViewports[0].w = RenderParameters.ViewportRect.Width();
	//frameStruct.targetViewports[0].h = RenderParameters.ViewportRect.Height();
	if (actorCrossThreadProperties&&actorCrossThreadProperties->TranslucentRT.GetReference())
	{ 
		TargetTexture = (FRHITexture2D*)actorCrossThreadProperties->TranslucentRT.GetReference();
	}
	frameStruct.depthViewports[0]=frameStruct.targetViewports[0];
	
	// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
	// NVCHANGE_END: TrueSky + VR MultiRes Support
	style=style|POST_TRANSLUCENT|(clear?CLEAR_SCREEN:DEFAULT_STYLE);
	// We need a real time timestamp for the RHI thread.
	if(actorCrossThreadProperties->Playing)
	{
		StaticTriggerAction("CalcRealTime"); 
	}

	//[Milestone]
	IRHICommandContext *CommandListContext = &RenderParameters.CurrentCmdList->GetContext();

	StoreUEState(RenderParameters, CurrentRHIType);

#if !UE_BUILD_SHIPPING
	CommandListContext->RHIPushEvent(TEXT("TrueSky::PostTranslucent"), FColor::Black);
#endif

#if SIMUL_SUPPORT_D3D12
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		if (!IsD3D12ContextOK(CommandListContext))
			return;
		PostTranslucentStoreStateD3D12(CommandListContext, TargetTexture, RenderParameters.DepthTexture);
	}
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
	//	return;
	}
#endif
	if (StaticRenderFrame2)
	{
		InitRenderFrameStruct
		(
			frameStruct,
			RenderParameters,
			TargetTexture,
			bMultiResEnabled,
			view_id,
			FrameNumber,
			style,
			true
		);

		StaticRenderFrame2(&frameStruct);
	}
	else
	{
		TArray<FVector2D>  MultiResConstants;
#ifdef NV_MULTIRES
		if (bMultiResEnabled)
		{
			SetMultiResConstants(MultiResConstants, View);
		}
#endif
		// Deprecated
		StaticRenderFrame(device, pContext, view_id, &(viewMatrix.M[0][0]), &(projMatrix.M[0][0])
			, depthTexture, colourTarget
			, frameStruct.depthViewports
			, frameStruct.targetViewports
			, style
			, exposure
			, gamma
			, FrameNumber
			// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
			, (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr)
			// NVCHANGE_END: TrueSky + VR MultiRes Support
		);
	}
#if SIMUL_SUPPORT_D3D12
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostTranslucentRestoreStateD3D12(CommandListContext, TargetTexture, RenderParameters.DepthTexture);
	}
#endif

#if !UE_BUILD_SHIPPING
	CommandListContext->RHIPopEvent();
#endif

	RestoreUEState(RenderParameters, CurrentRHIType);

	if(actorCrossThreadProperties->OverlayRT.GetReference())
		RenderOverlays(uid,RenderParameters,FrameNumber);
#if WITH_EDITOR
	// Draw tool windows etc.
	StaticTriggerAction("RenderToolViews");
#endif
#endif
}

void FTrueSkyPlugin::DelegatedRenderFrame(FRenderDelegateParameters& RenderParameters)
{
	if(!PostOpaqueEnabled)
		return;

	FSceneView *View = (FSceneView*)(RenderParameters.Uid);
	//Don't draw TrueSKY, if this is an editor preview window.
	if(View->Family->Scene->GetWorld()->WorldType == EWorldType::EditorPreview)
	{
		return;
	}

	//Has to be done here to prevent cross thread issues
	if (actorCrossThreadProperties->WaterUseGameTime)
	{
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_WaterGameTime, View->Family->Scene->GetWorld()->GetTimeSeconds()); //View->Family->CurrentWorldTime);//
	}

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(*RenderParameters.RHICmdList);
	FRHITexture2D *RenderTargetTexture= SceneContext.GetSceneColorSurface()->GetTexture2D();
	if (RenderTargetTexture && ((RenderTargetTexture->GetFlags()&TexCreate_RenderTargetable) != TexCreate_RenderTargetable))
		return;
	Viewport v;
	v.x=RenderParameters.ViewportRect.Min.X;
	v.y=RenderParameters.ViewportRect.Min.Y;
	v.w=RenderParameters.ViewportRect.Width();
	v.h=RenderParameters.ViewportRect.Height();
	bool bMultiResEnabled=false;
#ifdef NV_MULTIRES
	bMultiResEnabled=View->bVRProjectEnabled;
#endif
	const UTrueSkySettings *TrueSkySettings = GetDefault<UTrueSkySettings>();
	int ViewSize = std::min(v.h, v.w);
	if (ViewSize < TrueSkySettings->MinimumViewSize)
		return;
	if(View->Family)
	{
		switch(View->Family->SceneCaptureSource)
		{
		case SCS_SceneColorHDR:
		case SCS_SceneColorHDRNoAlpha:
		case SCS_FinalColorLDR:
		case SCS_BaseColor:
			break;
		default:
			return;
		};
	}
	bool bIncludePostTranslucent=(!post_trans_registered)&&(ViewSize>=TrueSkySettings->MinimumViewSizeForTranslucent);
	bIsInstancedStereoEnabled=View->bIsInstancedStereoEnabled;
	EStereoscopicPass StereoPass=View->StereoPass;
	FSceneViewState* ViewState = (FSceneViewState*)View->State;
	if(TrueSkySettings->RenderOnlyIdentifiedViews&&!ViewState)
		return;
	uint64_t uid=(uint64_t)(View);
	static uint64_t default_uid=10131101303;
	if(default_uid)
		uid=default_uid;
	if(ViewState)
		uid=ViewState->UniqueID;
	RenderParameters.DepthTexture->AddRef();
#if SIMUL_SUPPORT_VULKAN
	if (CurrentRHIType == GraphicsDeviceVulkan)
	{
		PostOpaqueStoreStateVulkan(&SceneContext, RenderParameters.RHICmdList, RenderTargetTexture, RenderParameters.DepthTexture);
	}
#endif
	if(RenderParameters.RHICmdList->Bypass())
	{
		FRHIPostOpaqueCommand Command(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber,bIncludePostTranslucent);
		Command.Execute(*RenderParameters.RHICmdList);
	}
	else
	{
		if (IsTrueSkyParallelRenderingSupported())
		{
			//MILESTONE TODO: verify splitscreen support: "LastTaskCompletionEvent" should be enough to sync (serialize) all opaque and translucent passes
			//				  and views, but, if more parallelism is required, add more completion events to sync them.
			//MILESTONE TODO: maybe UpdateFromActor() (in RenderFrame) should be called here and moved away from async task.
			//MILESTONE TODO: all "renderingenabled" checks, done by RenderFrame, could be moved here, in order to avoid useless task calls.

			//Prepare the deferred context container.
			int32 ThreadIndex = 0;
			int32 EffectiveThreads = 1;
			IRHICommandContextContainer* ContextContainer = RHIGetCommandContextContainer(ThreadIndex, EffectiveThreads, FRHIGPUMask::GPU0());
			check(ContextContainer);

			//Setup a new workerthread task, dependent on LastTaskCompletionEvent.
			FGraphEventArray Prerequisites;
			Prerequisites.Add(LastTaskCompletionEvent);
			const FGraphEventArray* PreReqs = LastTaskCompletionEvent.GetReference() ? &Prerequisites : nullptr;
			ENamedThreads::Type RenderThread = ENamedThreads::GetRenderThread();
			LastTaskCompletionEvent = TGraphTask<FPostOpaqueAnyThreadTask>::CreateTask(PreReqs, RenderThread)
				.ConstructAndDispatchWhenReady(RenderParameters, this, uid, StereoPass, bMultiResEnabled, GFrameNumber, bIncludePostTranslucent, ContextContainer, RenderTargetTexture);

			//Add an RHI command waiting for the workerthread task: it will submit the deferred context to the device.
			new (RenderParameters.RHICmdList->AllocCommand<FTrueSkyRHICommandWaitForAndSubmitSubListParallel>()) FTrueSkyRHICommandWaitForAndSubmitSubListParallel(LastTaskCompletionEvent,
				ContextContainer, EffectiveThreads, ThreadIndex, CurrentRHIType, TEXT("TrueSky::PostOpaqueParallel"), RenderTargetTexture);
		}
		else
		{
			new (RenderParameters.RHICmdList->AllocCommand<FRHIPostOpaqueCommand>()) FRHIPostOpaqueCommand(RenderParameters,this,uid,StereoPass,bMultiResEnabled,GFrameNumber,bIncludePostTranslucent);
		}
	}
#if SIMUL_SUPPORT_VULKAN
	if (CurrentRHIType == GraphicsDeviceVulkan)
	{
		PostOpaqueRestoreStateVulkan(&SceneContext, RenderParameters.RHICmdList, RenderTargetTexture, RenderParameters.DepthTexture);
	}
#endif
	RenderParameters.DepthTexture->Release();

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RenderParameters.RHICmdList->ApplyCachedRenderTargets(GraphicsPSOInit);
	
	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FSimpleElementVS> VertexShader(ShaderMap);
	
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GSimpleElementVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	
	SetGraphicsPipelineState(*RenderParameters.RHICmdList, GraphicsPSOInit, EApplyRendertargetOption::ForceApply);
}

void FTrueSkyPlugin::UpdateTrueSkyLights(FTrueSkyRenderDelegateParameters& RenderParameters,int FrameNumber)
{
	TArray<UTrueSkyLightComponent*> &components=UTrueSkyLightComponent::GetTrueSkyLightComponents();
	int cube_id=0;
	TArray<FTextureRHIRef> unrefTextures;
	UTrueSkyLightComponent::AllSkylightsValid=true;
	if(StaticCopySkylight2|| StaticCopySkylight3)
	for(int i=0;i<components.Num();i++)
	{
		UTrueSkyLightComponent* trueSkyLightComponent = components[i];
		FSkyLightSceneProxy *renderProxy=trueSkyLightComponent->GetSkyLightSceneProxy();
		if(renderProxy&&StaticCopySkylight)
		{
			//trueSkyLightComponent->SetIntensity(1.0f);
			copySkylightStruct.version	= copySkylightStruct.static_version;
			copySkylightStruct.pContext	=GetPlatformContext(RenderParameters, CurrentRHIType);
			const int nums				=renderProxy->IrradianceEnvironmentMap.R.MaxSHBasis;
			int NumTotalFloats			=renderProxy->IrradianceEnvironmentMap.R.NumSIMDVectors*renderProxy->IrradianceEnvironmentMap.R.NumComponentsPerSIMDVector;
            float shValues[16*3];
			copySkylightStruct.shValues=shValues;
			// We copy the skylight. The Skylight itself will be rendered with the specified SpecularMultiplier
			// because it will be copied directly to the renderProxy's texture.
			// returns 0 if successful:
			FTexture *t=renderProxy->ProcessedTexture;
			if(!t)
				continue;
			FLinearColor groundColour	= trueSkyLightComponent->LowerHemisphereColor;
			memcpy(&copySkylightStruct.ground_colour,(const float*)(&groundColour),3*sizeof(float));
			int result = 1;
			FMatrix transf=trueSkyLightComponent->GetComponentToWorld().ToMatrixWithScale();
			AdaptScaledMatrix(transf);
			memcpy((float*)(copySkylightStruct.engineToSimulMatrix4x4),&(transf.M[0][0]),16*sizeof(float));
			if(!trueSkyLightComponent->IsInitialized())
			{
				TriggerAction("ResetSkylights");
				trueSkyLightComponent->SetInitialized(true);
			}
			copySkylightStruct.updateFrequency	=trueSkyLightComponent->UpdateFrequency;
			{
				FTextureRHIRef *r=skylightTargetTextures.Find(cube_id);
				if(r&&r->IsValid())
				{
					if(r->GetReference()!=t->TextureRHI)
					{
						FTextureRHIRef u=*r;
						unrefTextures.Add(u);
						r->SafeRelease();
					}
				}
				FTextureRHIRef ref(t->TextureRHI);
				skylightTargetTextures.FindOrAdd(cube_id)=ref;
				if(actorCrossThreadProperties->CaptureCubemapRT != nullptr)
				{
					const int32 hires_cube_id=1835697618;
					copySkylightStruct.cube_id=hires_cube_id;
					auto r2=skylightTargetTextures.Find(copySkylightStruct.cube_id);
					if(r2&&r2->IsValid())
					{
						if(r2->GetReference()!=actorCrossThreadProperties->CaptureCubemapRT.GetReference())
							unrefTextures.Add(r2->GetReference());
					}
					skylightTargetTextures.FindOrAdd(hires_cube_id)=FTextureRHIRef(actorCrossThreadProperties->CaptureCubemapRT.GetReference());
					copySkylightStruct.targetTex=GetPlatformTexturePtr(actorCrossThreadProperties->CaptureCubemapRT.GetReference(), CurrentRHIType);
					if(copySkylightStruct.targetTex)
					{
						copySkylightStruct.shValues=nullptr;
						copySkylightStruct.updateFrequency=0;	// For now, Update Frequency of zero shall be taken to mean: do a full render of the cube, and force a full init.
						copySkylightStruct.blend			=0.0f;
						copySkylightStruct.exposure			=1.0f;
						copySkylightStruct.amortization		=1;
						copySkylightStruct.gamma			=1.0f;
						copySkylightStruct.allFaces			=true;
						copySkylightStruct.allMips			=true;
						copySkylightStruct.size				= actorCrossThreadProperties->CaptureCubemapRT->GetSizeXYZ().X;
						copySkylightStruct.pixelFormat		= ToSimulPixelFormat(actorCrossThreadProperties->CaptureCubemapRT->GetFormat(), CurrentRHIType);

						//Set the cubemap resolution to the desired resolution for the capture; this will get reset soon after.
						SetInt(ETrueSkyPropertiesInt::TSPROPINT_MaximumCubemapResolution, actorCrossThreadProperties->CaptureCubemapResolution);
						if(StaticCopySkylight3)
						{
							copySkylightStruct.version= copySkylightStruct.static_version;
							StaticCopySkylight3(&copySkylightStruct);
						}
						else
						{
							StaticCopySkylight2(copySkylightStruct.pContext
								, copySkylightStruct.cube_id
								, copySkylightStruct.shValues
								, copySkylightStruct.shOrder
								, copySkylightStruct.targetTex
								, copySkylightStruct.engineToSimulMatrix4x4
								, copySkylightStruct.updateFrequency
								, copySkylightStruct.blend
								, copySkylightStruct.exposure
								, copySkylightStruct.gamma
								, (const float*)&copySkylightStruct.ground_colour);
						}
						copySkylightStruct.updateFrequency=0;	// force full init of skylight.
					}
				}
				copySkylightStruct.cube_id		=cube_id;
				copySkylightStruct.allFaces		=trueSkyLightComponent->AllFaces;
				copySkylightStruct.allMips		=trueSkyLightComponent->AllMips;
				copySkylightStruct.amortization	=trueSkyLightComponent->Amortization;
				copySkylightStruct.blend		=trueSkyLightComponent->Blend;
				copySkylightStruct.exposure		=trueSkyLightComponent->SpecularMultiplier*actorCrossThreadProperties->Brightness;
				copySkylightStruct.gamma		=trueSkyLightComponent->Gamma;
				copySkylightStruct.shOrder		=renderProxy->IrradianceEnvironmentMap.R.MaxSHOrder;
				copySkylightStruct.targetTex	=GetPlatformTexturePtr(t, CurrentRHIType);
				if (!copySkylightStruct.targetTex)
					continue;
				copySkylightStruct.size = t->GetSizeX();
				copySkylightStruct.pixelFormat = ToSimulPixelFormat(ref->GetFormat(), CurrentRHIType);
				copySkylightStruct.mips = ref->GetNumMips();
#if SIMUL_SUPPORT_D3D12
				int state = 0;
				//D3D12_RESOURCE_STATES res;
				if (CurrentRHIType == GraphicsDeviceD3D12)
				{
					IRHICommandContext *ctx = &RenderParameters.CurrentCmdList->GetContext();
					state=(int)RHIResourceTransitionTextureCubeFromCurrentD3D12(ctx, t->TextureRHI.GetReference(), (D3D12_RESOURCE_STATES)0xC0, (D3D12_RESOURCE_STATES)0xAC3);
					//EnsureWriteStateD3D12(ctx, t->TextureRHI.GetReference());
				}
#endif
				if(StaticCopySkylight3)
				{
					result = StaticCopySkylight3(&copySkylightStruct);
				}
				else
					result = StaticCopySkylight2(copySkylightStruct.pContext
						,cube_id
						,shValues
						,copySkylightStruct.shOrder
						,copySkylightStruct.targetTex
						,&(transf.M[0][0])
						,copySkylightStruct.updateFrequency
						,copySkylightStruct.blend
						,copySkylightStruct.exposure
						,copySkylightStruct.gamma
						,(float*)&groundColour);
#if SIMUL_SUPPORT_D3D12
				if (CurrentRHIType == GraphicsDeviceD3D12)
				{
					IRHICommandContext *ctx = &RenderParameters.CurrentCmdList->GetContext(); 
					RHIResourceTransitionD3D12(ctx, t->TextureRHI.GetReference(), (D3D12_RESOURCE_STATES)0xAC3, (D3D12_RESOURCE_STATES)state);
					//EnsureWriteStateD3D12(ctx, t->TextureRHI.GetReference());
				}
#endif
			}
			if(!result)
			{
				// we apply the diffuse multiplier in:
				trueSkyLightComponent->UpdateDiffuse(shValues);
#ifdef SIMUL_ENLIGHTEN_SUPPORT
				// we want to probe into the cubemap and get values to be stored in the 16x16x6 CPU-side cubemap.
				FLinearColor lc(0,0,0,0);
				// Returns 0 if successful.
				int res=StaticProbeSkylight(pContext
					,cube_id
					,EnlightenEnvironmentDataSize
					,faceIndex
					,x
					,y
					,1
					,1
					,(float*) &lc);
				if(res==0)
				{
					trueSkyLightComponent->UpdateEnlighten(faceIndex,x,y,1,1,&lc);
				}
#endif
			}
			else
			{
				trueSkyLightComponent->UpdateDiffuse(nullptr);
				UTrueSkyLightComponent::AllSkylightsValid=false;
			}
		}
		cube_id++;
	}
	for(auto r:unrefTextures)
	{
		r.SafeRelease();
	}
#ifdef SIMUL_ENLIGHTEN_SUPPORT
	x++;
	if(x>=EnlightenEnvironmentResolution)
	{
		x=0;
		y++;
		if(y>=EnlightenEnvironmentResolution)
		{
			y=0;
			faceIndex++;
			faceIndex%=6;
		}
	}

#endif
}
extern simul::QueryMap truesky_queries;

void FTrueSkyPlugin::RenderFrame(uint64_t uid,FTrueSkyRenderDelegateParameters& RenderParameters
														,EStereoscopicPass StereoPass
														,bool bMultiResEnabled
														,int FrameNumber)
{
#if TRUESKY_PLATFORM_SUPPORTED
	if(!StaticSetRenderTexture&&!StaticSetRenderTexture2)
		return;
	errno=0;
	if(RenderParameters.Uid==0)
	{
		if(StaticShutDownInterface)
			StaticShutDownInterface();
		sequenceInUse=nullptr;
		return;
	}

	if(!RenderParameters.ViewportRect.Width()||!RenderParameters.ViewportRect.Height())
		return;
#if 1
	UpdateFromActor(RenderParameters);
	if(!actorCrossThreadProperties->Visible||!RenderingEnabled)
	{
		if(UTrueSkyLightComponent::AllSkylightsValid && actorCrossThreadProperties->CaptureCubemapRT == nullptr)
		{
			return;
		}
	}
    if (!GetActiveSequence())
        return;

	//[Milestone]
	IRHICommandContext *CommandListContext = &RenderParameters.CurrentCmdList->GetContext();

	FMatrix viewMatrix		= RenderParameters.ViewMatrix;
	void* colourTarget		= GetColourTarget(RenderParameters,CurrentRHIType);
	void* colourTargetTexture = GetPlatformTexturePtr(RenderParameters.RenderTargetTexture, CurrentRHIType);

	void* depthTexture		= GetPlatformTexturePtr(RenderParameters.DepthTexture, CurrentRHIType);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) 
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		if (!IsD3D12ContextOK(CommandListContext))
			return;
		PostOpaqueStoreStateD3D12(CommandListContext, RenderParameters.RenderTargetTexture,RenderParameters.DepthTexture, actorCrossThreadProperties->CloudShadowRT.GetReference());
	}
#endif
	SetRenderTexture(RenderParameters.RHICmdList,"Cubemap"			,actorCrossThreadProperties->RainCubemap			);
	SetRenderTexture(RenderParameters.RHICmdList,"Moon"				,actorCrossThreadProperties->MoonTexture			);
	SetRenderTexture(RenderParameters.RHICmdList,"Background"		,actorCrossThreadProperties->CosmicBackgroundTexture);
	SetRenderTexture(RenderParameters.RHICmdList,"Loss2D"			,actorCrossThreadProperties->LossRT					);
	SetRenderTexture(RenderParameters.RHICmdList,"Inscatter2D"		,actorCrossThreadProperties->InscatterRT			);
	SetRenderTexture(RenderParameters.RHICmdList,"CloudVisibilityRT",actorCrossThreadProperties->CloudVisibilityRT		);
	SetRenderTexture(RenderParameters.RHICmdList,"CloudShadowRT"	,actorCrossThreadProperties->CloudShadowRT			); 
	SetRenderTexture(RenderParameters.RHICmdList,"AltitudeLightRT"	,actorCrossThreadProperties->AltitudeLightRT		);
	if(actorCrossThreadProperties->SetTime)
		SetRenderFloat("time", actorCrossThreadProperties->Time);
	StaticSetRenderFloat("sky:MaxSunRadiance"		, actorCrossThreadProperties->MaxSunRadiance);
	StaticSetRenderBool("sky:AdjustSunRadius"		, actorCrossThreadProperties->AdjustSunRadius);
	
	// Apply the sub-component volumes:
	criticalSection.Lock();
	for(auto i=cloudVolumes.begin();i!=cloudVolumes.end();i++)
	{
		VariantPass params[20];
		params[0].intVal=i->first;
		FMatrix m=i->second.transform.ToMatrixWithScale();
		RescaleMatrix(m,actorCrossThreadProperties->MetresPerUnit);
		for(int j=0;j<16;j++)
		{
			params[j+1].floatVal=((const float*)m.M)[j];
		}
		FVector ext=i->second.extents*actorCrossThreadProperties->MetresPerUnit;// We want this in metres.
		for(int j=0;j<3;j++)
		{
			params[17+j].floatVal=ext[j];
		}
		StaticSetRender("CloudVolume",20,params);
	}
	criticalSection.Unlock();
	FMatrix cubemapMatrix;
	cubemapMatrix.SetIdentity();
	AdaptCubemapMatrix(cubemapMatrix);
	StaticSetMatrix4x4("CubemapTransform", &(cubemapMatrix.M[0][0]));
	
	if(actorCrossThreadProperties->RainDepthRT)
	{
		SetRenderTexture(RenderParameters.RHICmdList,"RainDepthTexture",actorCrossThreadProperties->RainDepthRT);
		if(actorCrossThreadProperties->RainDepthRT)
		{
			FMatrix rainMatrix=actorCrossThreadProperties->RainDepthMatrix;		// The bottom row of this matrix is the worldspace position
	
			StaticSetMatrix4x4("RainDepthTransform", &(actorCrossThreadProperties->RainDepthMatrix.M[0][0]));
			StaticSetMatrix4x4("RainDepthProjection", &(actorCrossThreadProperties->RainProjMatrix.M[0][0]));
			SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_RainDepthTextureScale,actorCrossThreadProperties->RainDepthTextureScale);
		}
	}

	float exposure  =actorCrossThreadProperties->Brightness;
	float gamma     =actorCrossThreadProperties->Gamma;

	FMatrix projMatrix = RenderParameters.ProjMatrix;
	AdaptProjectionMatrix(projMatrix,actorCrossThreadProperties->MetresPerUnit);
	void *device        = GetPlatformDevice(CurrentRHIType);
	void* pContext		= GetPlatformContext(RenderParameters, CurrentRHIType);
	AdaptViewMatrix(viewMatrix);
	PluginStyle style=UNREAL_STYLE;
	if(actorCrossThreadProperties->DepthBlending)
		style=style|DEPTH_BLENDING;
	frameStruct.targetViewports[0].x = RenderParameters.ViewportRect.Min.X;
	frameStruct.targetViewports[0].y = RenderParameters.ViewportRect.Min.Y;
	frameStruct.targetViewports[0].w = RenderParameters.ViewportRect.Width();
	frameStruct.targetViewports[0].h = RenderParameters.ViewportRect.Height();
	
	if(actorCrossThreadProperties->ShareBuffersForVR)
	{
		if(StereoPass==eSSP_LEFT_EYE)
		{
			//whichever one comes first, use it to fill the buffers, then: 
			style=style|VR_STYLE;
			uid=29435;
		}
		else if(StereoPass==eSSP_RIGHT_EYE)
		{
			//share the generated buffers for the other one.
			style=style|VR_STYLE;
			uid=29435;
		}
		if(StereoPass==eSSP_LEFT_EYE||StereoPass==eSSP_RIGHT_EYE)
		{
			if(FrameNumber==LastStereoFrameNumber&&LastStereoFrameNumber!=-1)
				style=style|VR_STYLE_RIGHT_EYE;
		}
	}
	if(StereoPass==eSSP_RIGHT_EYE&&FrameNumber!=LastFrameNumber)
	{
		style=style|VR_STYLE_RIGHT_EYE;
	}
	// If not visible, don't write to the screen.
	if(!actorCrossThreadProperties->Visible)
	{
		style=style|DONT_COMPOSITE;
	}
    int view_id = StaticGetOrAddView((void*)uid);		// RVK: really need a unique view ident to pass here..
	int Div=1;
	// VR Viewports:
	int vp_num=(StereoPass==eSSP_RIGHT_EYE)?2:1;
	{
		frameStruct.targetViewports[vp_num].x = RenderParameters.ViewportRect.Min.X;
		frameStruct.targetViewports[vp_num].y = RenderParameters.ViewportRect.Min.Y;
		frameStruct.targetViewports[vp_num].w = RenderParameters.ViewportRect.Width() ;
		frameStruct.targetViewports[vp_num].h = RenderParameters.ViewportRect.Height();
	}
	for (int i = 0; i<3; i++)
	{
		frameStruct.depthViewports[i]= frameStruct.targetViewports[i];
	}

	// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
	TArray<FVector2D>  MultiResConstants;
#ifdef NV_MULTIRES
	if (bMultiResEnabled)
	{
		FSceneView *View = (FSceneView*)(RenderParameters.Uid);
		SetMultiResConstants(MultiResConstants, View);
	}
#endif
	// NVCHANGE_END: TrueSky + VR MultiRes Support

	StaticTriggerAction("CalcRealTime");
	StoreUEState(RenderParameters, CurrentRHIType);
#if !UE_BUILD_SHIPPING
	CommandListContext->RHIPushEvent(TEXT("TrueSky::PostOpaque"), FColor::Black);
#endif

	if(actorCrossThreadProperties->CaptureCubemapRT != nullptr)
	{
		UpdateTrueSkyLights(RenderParameters,FrameNumber);
	}

	if (StaticRenderFrame2)
	{
		InitRenderFrameStruct(frameStruct, RenderParameters
			, RenderParameters.RenderTargetTexture
			, bMultiResEnabled
			, view_id
			, FrameNumber
			, style
			, false);
		frameStruct.colourTargetTexture = colourTargetTexture;
		if (colourTargetTexture != nullptr)
			frameStruct.isColourTargetTexture = true;
		else
			frameStruct.isColourTargetTexture = false;
		StaticRenderFrame2(&frameStruct);
	}
	else
	{
		// Deprecated
		StaticRenderFrame(device,pContext,view_id, &(viewMatrix.M[0][0]), &(projMatrix.M[0][0])
			,depthTexture,colourTarget
			,frameStruct.depthViewports
			,frameStruct.targetViewports
			,style
			,exposure
			,gamma
			,FrameNumber
			// NVCHANGE_BEGIN: TrueSky + VR MultiRes Support
			, (bMultiResEnabled ? (const float*)(MultiResConstants.GetData()) : nullptr)
			// NVCHANGE_END: TrueSky + VR MultiRes Support
			);
	}
	if(StaticProcessQueries)
	{
		for(auto &q:truesky_queries)
		{
			StaticProcessQueries(1,&q.Value);
		}
	}
	if(FrameNumber!=LastFrameNumber)
	{
		// TODO: This should only be done once per frame.
		UpdateTrueSkyLights(RenderParameters,FrameNumber);
		LastFrameNumber=FrameNumber;
		actorCrossThreadProperties->CaptureCubemapRT.SafeRelease();
		
#if WITH_EDITOR
		if(StaticExecuteDeferredRendering)
			StaticExecuteDeferredRendering();
#endif
	}
	
    // Fill in colours requested by the editor plugin.
	if(colourTableRequests.size())
	{
		for(auto i:colourTableRequests)
		{
			unsigned uidc=i.first;
			ColourTableRequest *req=i.second;
			if(req&&req->valid)
				continue;
			delete [] req->data;
			req->data=new float[4*req->x*req->y*req->z];
			if(StaticFillColourTable(uidc,req->x,req->y,req->z,req->data))
			{
				UE_LOG(TrueSky,Display, TEXT("Colour table filled!"));
				req->valid=true;
				break;
			}
			else
			{
				UE_LOG(TrueSky,Display, TEXT("Colour table not filled!"));
				req->valid=false;
				continue;
			}
		}

		// tell the editor that the work is done.
		if(TrueSkyEditorCallback)
			TrueSkyEditorCallback();
	}
	if(exportNext)
	{
		StaticExportCloudLayerToGeometry(FStringToUtf8(exportFilenameUtf8).c_str(),0);
		exportNext=false;
	}

	// Post-trans
	if (actorCrossThreadProperties&&actorCrossThreadProperties->TranslucentRT.GetReference())
	{
		RenderPostTranslucent(uid, RenderParameters, StereoPass, bMultiResEnabled, FrameNumber, true);
	}


	if (StereoPass == eSSP_LEFT_EYE || StereoPass == eSSP_RIGHT_EYE)
		LastStereoFrameNumber = FrameNumber;
	LastFrameNumber = FrameNumber;

	//Milestone
#if !UE_BUILD_SHIPPING
	CommandListContext->RHIPopEvent();
#endif
	
	RestoreUEState(RenderParameters, CurrentRHIType);

#if ( PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOXONE ) 
	if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		PostOpaqueRestoreStateD3D12(CommandListContext, RenderParameters.RenderTargetTexture, RenderParameters.DepthTexture, actorCrossThreadProperties->CloudShadowRT.GetReference());
	}
#endif

#endif
#endif // TRUESKY_PLATFORM_SUPPORTED
}

void FTrueSkyPlugin::OnDebugTrueSky(class UCanvas* Canvas, APlayerController*)
{
	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(false, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->DrawText(RenderFont, FString("trueSKY Debug Display"), 0.3f, 0.3f, 1.f, 1.f, FontRenderInfo);

	Canvas->SetDrawColor(OldDrawColor);
}

void FTrueSkyPlugin::PreUnloadCallback()
{
}

void FTrueSkyPlugin::ShutdownModule()
{
}
void FTrueSkyPlugin::PreExit()
{
	if(StaticShutDownInterface)
		StaticShutDownInterface();
#if WITH_EDITOR
	// unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "trueSKY");
	}
#endif //WITH_EDITOR
	if(StaticSetGraphicsDevice)
#if PLATFORM_PS4
		StaticSetGraphicsDevice(nullptr, GraphicsDevicePS4, GraphicsEventShutdown);
#else
		StaticSetGraphicsDevice(nullptr, CurrentRHIType, GraphicsEventShutdown);
#endif
	delete PathEnv;
	PathEnv = nullptr;
	RendererInitialized=false;
	sequenceInUse=nullptr;
	if(StaticSetDebugOutputCallback)
		StaticSetDebugOutputCallback(nullptr);
	for(auto t:skylightTargetTextures)
	{
		t.Value.SafeRelease();
	}
	skylightTargetTextures.Empty();
	// Delete ctp last because it has refs that the other shutdowns might expect to be valid.
	delete actorCrossThreadProperties;
	actorCrossThreadProperties=nullptr;
}

bool FTrueSkyPlugin::Exec( UWorld* WorldIn, const TCHAR* Cmd, FOutputDevice& Out )
{
	if( FParse::Command( &Cmd, TEXT( "TrueSkyProfileFrame" ) ) )
	{
		FString str = FParse::Token(Cmd, false);
		int32 level = !str.IsEmpty() ? FPlatformString::Atoi(*str) : 6;
		str = FParse::Token(Cmd, false);
		int32 cpu_level = !str.IsEmpty() ? FPlatformString::Atoi(*str) : 5;
		// Log profiling text.
		Out.Logf( TEXT( "[trueSKY Profiler]\n%s" ), *ATrueSkySequenceActor::GetProfilingText( cpu_level ,level) );
		return true;
	}

	return false;
}

#ifdef _MSC_VER
#if !PLATFORM_UWP
/** Returns environment variable value */
static wchar_t* GetEnvVariable( const wchar_t* const VariableName, int iEnvSize = 1024)
{
	wchar_t* Env = new wchar_t[iEnvSize];
	check( Env );
	memset(Env, 0, iEnvSize * sizeof(wchar_t));
	if ( (int)GetEnvironmentVariableW(VariableName, Env, iEnvSize) > iEnvSize )
	{
		delete [] Env;
		Env = nullptr;
	}
	else if ( wcslen(Env) == 0 )
	{
		return nullptr;
	}
	return Env;
}

#endif
#endif

bool CheckDllFunction(void *fn,FString &str,const char *fnName)
{
	bool res=(fn!=0);
	if(!res)
	{
		if(!str.IsEmpty())
			str+=", ";
		str+=fnName;
	}
	return res;
}
#if PLATFORM_PS4 || PLATFORM_SWITCH
#define MISSING_FUNCTION(fnName) (!CheckDllFunction((void*)fnName,failed_functions, #fnName))
#else
#define MISSING_FUNCTION(fnName) (!CheckDllFunction((void*)fnName,failed_functions, #fnName))
#endif
#define MISSING_BOTH(fn1,fn2) (MISSING_FUNCTION(fn1)&&MISSING_FUNCTION(fn2))

int FTrueSkyPlugin::StructSizeWrong(const char *structName, size_t thisSize) const
{
	FString str("sizeof:");
	str += structName;
	int dllSize = StaticGetRenderInt(FStringToUtf8(str).c_str(), 0, nullptr);
	int wrong = (dllSize != (int)thisSize) ? 1 : 0;
	if (wrong)
	{
		UE_LOG(TrueSky, Warning, TEXT("struct sizes do not match for %s. DLL size is %d, exe size is %d. Please update the trueSKY dll's"),*FString(structName), dllSize, thisSize);
	}
	return wrong;
}

#define STRUCT_SIZE_WRONG(structName) (StructSizeWrong(#structName,sizeof(structName)))
// Reimplement because it's not there in the Xbox One version
//FWindowsPlatformProcess::GetDllHandle
moduleHandle GetDllHandle( const TCHAR* Filename )
{
	check(Filename);
#if PLATFORM_PS4
//moduleHandle ImageFileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*ImageFullPath);
	moduleHandle m= sceKernelLoadStartModule(TCHAR_TO_ANSI(Filename), 0, 0, 0, nullptr, nullptr);
	switch(m)
	{
		case SCE_KERNEL_ERROR_EINVAL:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020016 flags or pOpt is invalid "));
			return 0;
		case SCE_KERNEL_ERROR_ENOENT:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020002 File specified in moduleFileName does not exist "));
			return 0;
		case SCE_KERNEL_ERROR_ENOEXEC:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error:  0x80020008 Cannot load because of abnormal file format "));
			return 0;
		case SCE_KERNEL_ERROR_ENOMEM:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x8002000c Cannot load because it is not possible to allocate memory "));
			return 0;
		case SCE_KERNEL_ERROR_EFAULT:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x8002000e moduleFileName points to invalid memory "));
			return 0;
		case SCE_KERNEL_ERROR_EAGAIN:
			UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllHandle error: 0x80020023 Cannot load because of insufficient resources "));
			return 0;
		default:
			break;
	};
	return m;
#else
#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_LINUX
	moduleHandle h=FPlatformProcess::GetDllHandle(Filename);
	if (!h)
	{
		auto err = FGenericPlatformMisc::GetLastError();
		const int size = 1024;
		TCHAR *buffer=nullptr;
		if (!FGenericPlatformMisc::GetSystemErrorMessage(buffer,size,err))
		{
			return nullptr;
		}
		TCHAR tbuffer[1000];
		memcpy(tbuffer, buffer, 100);
		UE_LOG(TrueSky, Warning,tbuffer);

	}
	return h;
#elif PLATFORM_SWITCH

    nn::Result res;
    moduleHandle module;

    FString FilenameNrr(Filename);
    FilenameNrr = FilenameNrr.Replace(CONST_UTF8_TO_TCHAR(".nro"), CONST_UTF8_TO_TCHAR(".nrr"));

    // Init the ro system:
    nn::ro::Initialize();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    // Load the nrr file:
    int64 nrrSize               = PlatformFile.FileSize((const TCHAR*)FilenameNrr.GetCharArray().GetData());
    if (nrrSize < 0)
    {
        UE_LOG(TrueSky, Warning, TEXT("Failed to load %s"), (const TCHAR*)FilenameNrr.GetCharArray().GetData());
	    return module;
    }
    NXPluginData.pNrr           = (uint8*)FMemory::Malloc(nrrSize, nn::os::MemoryPageSize);
    IFileHandle* FileHandle     = PlatformFile.OpenRead((const TCHAR*)FilenameNrr.GetCharArray().GetData());
    FileHandle->Read(NXPluginData.pNrr, nrrSize);
    res                         = nn::ro::RegisterModuleInfo(&NXPluginData.nrrInfo, NXPluginData.pNrr);
    delete FileHandle;

    // Load the nro file:
    int64 nroSize = PlatformFile.FileSize(Filename);
    if (nroSize < 0)
    {
        UE_LOG(TrueSky, Warning, TEXT("Failed to load %s"), Filename);
        return module;
    }
    NXPluginData.pNro   = (uint8*)FMemory::Malloc(nroSize, nn::os::MemoryPageSize);
    FileHandle          = PlatformFile.OpenRead(Filename);
    FileHandle->Read(NXPluginData.pNro, nroSize);
    
    // Query ammount of memory required to load the module:
    size_t bssSize  = 0;
    nn::ro::GetBufferSize(&bssSize, NXPluginData.pNro);
    if (bssSize != 0)
    {
        NXPluginData.bss = FMemory::Malloc(bssSize, nn::os::MemoryPageSize);
        FMemory::Memset(NXPluginData.bss, 0, bssSize);
    }
    res = nn::ro::LoadModule(&module, NXPluginData.pNro, NXPluginData.bss, bssSize, nn::ro::BindFlag_Now);
    delete FileHandle;

    return module;

#else
	return ::LoadLibraryW(Filename);
#endif
#endif
}

// And likewise for GetDllExport:
#if PLATFORM_SWITCH
uintptr_t GetDllExport(moduleHandle DllHandle, const TCHAR* ProcName)
#else
void* GetDllExport( moduleHandle DllHandle, const TCHAR* ProcName )
#endif
{
#if PLATFORM_SWITCH
    check(&DllHandle);
#else
	check(DllHandle);
#endif
	check(ProcName);
#if PLATFORM_PS4
	void *addr=nullptr;
	int result=sceKernelDlsym( DllHandle,
								TCHAR_TO_ANSI(ProcName),
								&addr);
	/*
        SCE_KERNEL_ERROR_ESRCH  0x80020003  handle is invalid, or symbol specified in symbol is not exported 
        SCE_KERNEL_ERROR_EFAULT 0x8002000e  symbol or addrp address is invalid 
	*/
	if(result==SCE_KERNEL_ERROR_ESRCH)
	{
		UE_LOG_ONCE(TrueSky, Display, TEXT("GetDllExport got error: For symbol %s, SCE_KERNEL_ERROR_ESRCH, which means 'handle is invalid, or symbol specified in symbol is not exported'"),ProcName);
	}
	else if(result==SCE_KERNEL_ERROR_EFAULT)
	{
		UE_LOG_ONCE(TrueSky, Warning, TEXT("GetDllExport got error: SCE_KERNEL_ERROR_EFAULT"));
	}
	return addr;
#elif PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_UWP
	return (void*)::GetProcAddress( (HMODULE)DllHandle, TCHAR_TO_ANSI(ProcName) );
#elif PLATFORM_LINUX || PLATFORM_MAC
    dlerror();
    void *res= dlsym( DllHandle, TCHAR_TO_ANSI(ProcName) );
    const char *er=dlerror();
    if(er)
       UE_LOG(TrueSky, Display, TEXT("dlsym error: %s"),er);
    return res;
   // return (void*)FPlatformProcess::GetDllExport( (void*) DllHandle, ProcName );
#elif PLATFORM_SWITCH
    nn::Result res;
    uintptr_t pAddr = 0;
    res             = nn::ro::LookupModuleSymbol(&pAddr, &DllHandle, TCHAR_TO_ANSI(ProcName));
    return *(&pAddr);
#else
	return nullptr;
#endif
}
#define GET_FUNCTION(fnName) {fnName= (F##fnName)GetDllExport(DllHandle, TEXT(#fnName) );}

namespace simul
{
	namespace unreal
	{
		static const TCHAR SimulUnrealPlatformName[] =
		#if PLATFORM_XBOXONE
			TEXT("XboxOne");
		#elif PLATFORM_PS4
			TEXT("PS4");
		#elif PLATFORM_SWITCH
			TEXT("Switch");
		#elif PLATFORM_UWP
			TEXT("UWP64");
		#elif PLATFORM_LINUX
			TEXT("Linux");
		#else
			TEXT("Win64");
		#endif
	}
}

bool FTrueSkyPlugin::InitRenderingInterface(  )
{
	//[Milestone]
#if !TRUESKY_PLATFORM_SUPPORTED
	return false;
#endif

	InitPaths();
	static bool failed_once = false;
	FString EnginePath=FPaths::EngineDir();
#if PLATFORM_XBOXONE
	FString pluginFilename=TEXT("TrueSkyPluginRender_MD");
	FString debugExtension=TEXT("d");
	FString dllExtension=TEXT(".dll");
#endif
#if PLATFORM_PS4
	FString pluginFilename=TEXT("trueskypluginrender");
	FString debugExtension=TEXT("-debug");
	FString dllExtension=TEXT(".prx");
#endif
#if PLATFORM_WINDOWS || PLATFORM_UWP
	FString pluginFilename=TEXT("TrueSkyPluginRender_MT");
	FString debugExtension=TEXT("d");
	FString dllExtension=TEXT(".dll");
#endif
#if PLATFORM_LINUX
    FString pluginFilename = TEXT("TrueSkyPluginRender_MT");
	FString debugExtension = TEXT("d");
	FString dllExtension = TEXT(".so");
#endif
#if PLATFORM_SWITCH
	FString pluginFilename = TEXT("TrueSkyPluginRender");
	FString debugExtension = TEXT("d");
	FString dllExtension = TEXT(".nro"); // .nro & .nrr
#endif
#ifdef DEBUG // UE_BUILD_DEBUG //doesn't work... why?
	pluginFilename+=debugExtension;
#else
    // why is NDEBUG defined in debug builds?
#endif
	pluginFilename+=dllExtension;
    FString DllPath(FPaths::Combine(*EnginePath,TEXT("Binaries/ThirdParty/Simul"),(const TCHAR *)SimulUnrealPlatformName));
	FString DllFilename(FPaths::Combine(*DllPath,*pluginFilename));
#if PLATFORM_PS4
	if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		//   ../../../engine/plugins/trueskyplugin/alternatebinariesdir/ps4/trueskypluginrender.prx
		// For SOME reason, UE moves all prx's to the root/prx folder...??
		DllPath=TEXT("prx/");
		DllFilename=FPaths::Combine(*DllPath,*pluginFilename);
		if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
		{
			DllPath=FPaths::Combine(*EnginePath,TEXT("../prx/"));
			DllFilename=FPaths::Combine(*DllPath,*pluginFilename);
		}
	}
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		DllPath = "/app0/prx/";
		DllFilename = FPaths::Combine(*DllPath, *pluginFilename);
	}
#elif PLATFORM_SWITCH
    DllPath     = FPaths::Combine(*EnginePath, TEXT("Binaries/ThirdParty/Simul/"), (const TCHAR *)SimulUnrealPlatformName);
    DllFilename = FPaths::Combine(*DllPath, *pluginFilename);
#endif
	if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*DllFilename))
	{
		if (!failed_once)
			UE_LOG(TrueSky, Warning, TEXT("%s not found."), *DllFilename);

		failed_once = true;
		return false;
	}
#if PLATFORM_PS4
	IFileHandle *fh=FPlatformFileManager::Get().GetPlatformFile().OpenRead(*DllFilename);
	delete fh;
	DllFilename	=FPlatformFileManager::Get().GetPlatformFile().ConvertToAbsolutePathForExternalAppForRead(*DllFilename);
#endif
	FPlatformProcess::PushDllDirectory(*DllPath);
	moduleHandle const DllHandle = GetDllHandle((const TCHAR*)DllFilename.GetCharArray().GetData() );
	FPlatformProcess::PopDllDirectory(*DllPath);
    
#if PLATFORM_SWITCH
    if (DllHandle._state == nn::ro::Module::State::State_Unloaded)
#else
	if(DllHandle==0)
#endif
	{
		if(!failed_once)
			UE_LOG(TrueSky, Warning, TEXT("Failed to load %s"), (const TCHAR*)DllFilename.GetCharArray().GetData());
		failed_once = true;
		return false;
	}

#if PLATFORM_SWITCH
    if (DllHandle._state == nn::ro::Module::State::State_Loaded)
#else
	if (DllHandle != 0)
#endif
	{
		StaticInitInterface				=(FStaticInitInterface)GetDllExport(DllHandle, TEXT("StaticInitInterface") );
		StaticSetMemoryInterface		=(FStaticSetMemoryInterface)GetDllExport(DllHandle, TEXT("StaticSetMemoryInterface") );
		StaticShutDownInterface			=(FStaticShutDownInterface)GetDllExport(DllHandle, TEXT("StaticShutDownInterface") );
		StaticSetDebugOutputCallback	=(FStaticSetDebugOutputCallback)GetDllExport(DllHandle,TEXT("StaticSetDebugOutputCallback"));
		StaticSetGraphicsDevice			=(FStaticSetGraphicsDevice)GetDllExport(DllHandle, TEXT("StaticSetGraphicsDevice") );
		StaticSetGraphicsDeviceAndContext=(FStaticSetGraphicsDeviceAndContext)GetDllExport(DllHandle, TEXT("StaticSetGraphicsDeviceAndContext") );
		StaticPushPath					=(FStaticPushPath)GetDllExport(DllHandle, TEXT("StaticPushPath") );

		GET_FUNCTION(StaticSetFileLoader);
		GET_FUNCTION(StaticGetLightningBolts);
		GET_FUNCTION(StaticSpawnLightning);
		GET_FUNCTION(StaticFillColourTable);
		GET_FUNCTION(StaticGetRenderingInterface);
		GET_FUNCTION(StaticExecuteDeferredRendering);
		GET_FUNCTION(GetSimulVersion);
		
		GET_FUNCTION(StaticGetOrAddView);
		GET_FUNCTION(StaticRenderFrame);
		GET_FUNCTION(StaticRenderFrame2);
		GET_FUNCTION(StaticCopySkylight);
		GET_FUNCTION(StaticCopySkylight2);
		GET_FUNCTION(StaticCopySkylight3);
		GET_FUNCTION(StaticProbeSkylight);
		GET_FUNCTION(StaticRenderOverlays);
		GET_FUNCTION(StaticRenderOverlays2);
			
		GET_FUNCTION(StaticOnDeviceChanged);
		GET_FUNCTION(StaticTick);
		GET_FUNCTION(StaticGetEnvironment);
		GET_FUNCTION(StaticSetSequence);
		GET_FUNCTION(StaticSetSequence2);
		GET_FUNCTION(StaticSetMoon);
		GET_FUNCTION(StaticGetRenderInterfaceInstance);

		GET_FUNCTION(StaticSetPointLight);
		GET_FUNCTION(StaticCloudPointQuery);
		GET_FUNCTION(StaticLightingQuery);
		GET_FUNCTION(StaticCloudLineQuery);
		GET_FUNCTION(StaticSetRenderTexture);
		GET_FUNCTION(StaticSetRenderTexture2);
		GET_FUNCTION(StaticSetMatrix4x4);
		GET_FUNCTION(StaticSetRender);
		GET_FUNCTION(StaticSetRenderBool);
		GET_FUNCTION(StaticGetRenderBool);
		GET_FUNCTION(StaticSetRenderFloat);
		GET_FUNCTION(StaticGetRenderFloat);
		GET_FUNCTION(StaticSetRenderInt);
		GET_FUNCTION(StaticGetRenderInt);
					
		GET_FUNCTION(StaticCreateBoundedWaterObject);
		GET_FUNCTION(StaticRemoveBoundedWaterObject);

		GET_FUNCTION(StaticAddWaterProbe);
		GET_FUNCTION(StaticRemoveWaterProbe);
		GET_FUNCTION(StaticUpdateWaterProbeValues);
		GET_FUNCTION(StaticGetWaterProbeValues);

		GET_FUNCTION(StaticAddWaterBuoyancyObject);
		GET_FUNCTION(StaticUpdateWaterBuoyancyObjectValues);
		GET_FUNCTION(StaticGetWaterBuoyancyObjectResults);
		GET_FUNCTION(StaticRemoveWaterBuoyancyObject);

		GET_FUNCTION(StaticAddWaterMaskObject);
		GET_FUNCTION(StaticUpdateWaterMaskObjectValues);
		GET_FUNCTION(StaticRemoveWaterMaskObject);

		GET_FUNCTION(StaticSetWaterFloat);
		GET_FUNCTION(StaticSetWaterInt);
		GET_FUNCTION(StaticSetWaterBool);
		GET_FUNCTION(StaticSetWaterVector);
		GET_FUNCTION(StaticGetWaterFloat);
		GET_FUNCTION(StaticGetWaterInt);
		GET_FUNCTION(StaticGetWaterBool);
		GET_FUNCTION(StaticGetWaterVector);

		GET_FUNCTION(StaticGetKeyframeFloat);
		GET_FUNCTION(StaticSetKeyframeFloat);
		GET_FUNCTION(StaticGetKeyframeInt);
		GET_FUNCTION(StaticSetKeyframeInt);

		GET_FUNCTION(StaticGetKeyframerInt);
		GET_FUNCTION(StaticSetKeyframerInt);
		GET_FUNCTION(StaticSetKeyframerFloat);
		GET_FUNCTION(StaticSetKeyframerFloat);

		GET_FUNCTION(StaticGetRenderString);		
		GET_FUNCTION(StaticSetRenderString);		

		GET_FUNCTION(StaticExportCloudLayerToGeometry);

		GET_FUNCTION(StaticTriggerAction);

		StaticSetKeyframeFloat			=(FStaticSetKeyframeFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframeSetFloat"));
		StaticGetKeyframeFloat			=(FStaticGetKeyframeFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframeGetFloat"));
		StaticSetKeyframeInt			=(FStaticSetKeyframeInt)GetDllExport(DllHandle,	TEXT("StaticRenderKeyframeSetInt"));
		StaticGetKeyframeInt			=(FStaticGetKeyframeInt)GetDllExport(DllHandle,	TEXT("StaticRenderKeyframeGetInt"));

		StaticSetKeyframerFloat = (FStaticSetKeyframerFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframerSetFloat"));
		StaticGetKeyframerFloat = (FStaticGetKeyframerFloat)GetDllExport(DllHandle, TEXT("StaticRenderKeyframerGetFloat"));
		StaticSetKeyframerInt = (FStaticSetKeyframerInt)GetDllExport(DllHandle, TEXT("StaticRenderKeyframerSetInt"));
		StaticGetKeyframerInt = (FStaticGetKeyframerInt)GetDllExport(DllHandle, TEXT("StaticRenderKeyframerGetInt"));

		GET_FUNCTION(StaticGetRenderFloatAtPosition);
		GET_FUNCTION(StaticGetFloatAtPosition);
		GET_FUNCTION(StaticGet);
		GET_FUNCTION(StaticSet);
		GET_FUNCTION(StaticWaterSet);
		GET_FUNCTION(StaticGetEnum);
		GET_FUNCTION(StaticProcessQueries);
		GET_FUNCTION(StaticSetMoon);
		if(STRUCT_SIZE_WRONG(ExternalMoon))
			StaticSetMoon=nullptr;
		FString failed_functions;
		int num_fails=MISSING_FUNCTION(StaticInitInterface)
#if PLATFORM_PS4
			+MISSING_FUNCTION(StaticSetMemoryInterface)
#endif
			+MISSING_FUNCTION(StaticShutDownInterface)
			+MISSING_FUNCTION(StaticSetDebugOutputCallback)
			+MISSING_FUNCTION(StaticPushPath)
			+MISSING_FUNCTION(StaticSetFileLoader)
			+MISSING_FUNCTION(StaticRenderFrame)
			+MISSING_FUNCTION(StaticGetLightningBolts)
			+MISSING_FUNCTION(StaticSpawnLightning)
			+MISSING_FUNCTION(StaticFillColourTable)
			+ MISSING_BOTH(StaticRenderOverlays, StaticRenderOverlays2)
			+MISSING_FUNCTION(StaticGetOrAddView)
			+MISSING_FUNCTION(StaticGetEnvironment)
			+MISSING_BOTH(StaticSetSequence, StaticSetSequence2)
			+MISSING_FUNCTION(StaticGetRenderInterfaceInstance)
			+MISSING_FUNCTION(StaticSetPointLight)
			+MISSING_FUNCTION(StaticCloudPointQuery)
			+MISSING_FUNCTION(StaticCloudLineQuery)
			+ MISSING_BOTH(StaticSetRenderTexture, StaticSetRenderTexture2)
			+MISSING_FUNCTION(StaticSetRenderBool)
			+MISSING_FUNCTION(StaticGetRenderBool)
			+MISSING_FUNCTION(StaticTriggerAction)
			+MISSING_FUNCTION(StaticSetRenderFloat)
			+MISSING_FUNCTION(StaticGetRenderFloat)
			+MISSING_FUNCTION(StaticSetRenderString)
			+MISSING_FUNCTION(StaticGetRenderString)
			+MISSING_FUNCTION(StaticExportCloudLayerToGeometry)
			+MISSING_FUNCTION(StaticSetRenderInt)
			+MISSING_FUNCTION(StaticGetRenderInt)
			+MISSING_FUNCTION(StaticSetKeyframeFloat)
			+MISSING_FUNCTION(StaticGetKeyframeFloat)
			+MISSING_FUNCTION(StaticSetKeyframeInt)
			+MISSING_FUNCTION(StaticGetKeyframeInt)
			//+MISSING_FUNCTION(StaticSetKeyframerFloat) SOMEBODY forgot that these don't exist in 4.1a
			//+MISSING_FUNCTION(StaticGetKeyframerFloat)
			//+MISSING_FUNCTION(StaticSetKeyframerInt)
			//+MISSING_FUNCTION(StaticGetKeyframerInt)
			+MISSING_FUNCTION(StaticSetMatrix4x4)
			+MISSING_FUNCTION(StaticGetRenderFloatAtPosition)
			+MISSING_BOTH(StaticCopySkylight2,StaticCopySkylight3)
			//+MISSING_FUNCTION(StaticGetFloatAtPosition)
			+(StaticSetGraphicsDevice==nullptr&&StaticSetGraphicsDeviceAndContext==nullptr);

		if(num_fails>0)
		{
			static bool reported=false;
			if(!reported)
			{
				UE_LOG(TrueSky, Error
					,TEXT("Can't initialize the trueSKY rendering plugin dll because %d functions were not found - please update TrueSkyPluginRender_MT.dll.\nThe missing functions are %s.")
					,num_fails
					,*failed_functions
					);
				reported=true;
			}
			//missing dll functions... cancel initialization
			SetRenderingEnabled(false);
			return false;
		}
		int water_fails=
			MISSING_FUNCTION(StaticCreateBoundedWaterObject)
			+MISSING_FUNCTION(StaticRemoveBoundedWaterObject)
			+MISSING_FUNCTION(StaticAddWaterProbe)
			+MISSING_FUNCTION(StaticRemoveWaterProbe)
			+MISSING_FUNCTION(StaticUpdateWaterProbeValues)
			+MISSING_FUNCTION(StaticAddWaterBuoyancyObject)
			+MISSING_FUNCTION(StaticUpdateWaterBuoyancyObjectValues)
			+MISSING_FUNCTION(StaticGetWaterBuoyancyObjectResults)	
			+MISSING_FUNCTION(StaticRemoveWaterBuoyancyObject)
			+MISSING_FUNCTION(StaticAddWaterMaskObject)
			+MISSING_FUNCTION(StaticUpdateWaterMaskObjectValues)
			+MISSING_FUNCTION(StaticRemoveWaterMaskObject)
			+MISSING_FUNCTION(StaticGetWaterProbeValues)
			+MISSING_FUNCTION(StaticSetWaterFloat)
			+MISSING_FUNCTION(StaticSetWaterInt)
			+MISSING_FUNCTION(StaticSetWaterBool)
			+MISSING_FUNCTION(StaticSetWaterVector)
			+MISSING_FUNCTION(StaticGetWaterFloat)
			+MISSING_FUNCTION(StaticGetWaterInt)
			+MISSING_FUNCTION(StaticGetWaterBool)
			+MISSING_FUNCTION(StaticGetWaterVector);

		UE_LOG(TrueSky, Display, TEXT("Loaded trueSKY dynamic library %s."), *DllFilename);
		int struct_fails = STRUCT_SIZE_WRONG(ExternalTexture) + STRUCT_SIZE_WRONG(RenderFrameStruct) + ((StaticCopySkylight3 != nullptr) ? STRUCT_SIZE_WRONG(CopySkylightStruct) : 0);
#if PLATFORM_PS4
		/*
		LightweightConstantUpdateEngine::kMaxResourceCount (defined in lwcue_base.h) was changed from 16 to 48. So if you build trueSky for UE4, customized gnmx should be used.
		*/
		struct_fails += STRUCT_SIZE_WRONG(sce::Gnmx::LightweightGraphicsConstantUpdateEngine);
#endif
		int major=4,minor=0,build=0;
		if(GetSimulVersion)
		{
			GetSimulVersion(&major,&minor,&build);
			simulVersion=ToSimulVersion(major,minor,build);
			UE_LOG(TrueSky, Display, TEXT("Simul version %d.%d build %d"), major,minor,build);
			if (simulVersion >= SIMUL_4_2 && struct_fails!=0)
			{
				UE_LOG(TrueSky, Error, TEXT("Struct sizes do not match."));
				SetRenderingEnabled(false);
				StaticSetRenderTexture = nullptr;
				StaticSetRenderTexture2 = nullptr;
				return false;
			}
		}
#ifdef _MSC_VER
#if !PLATFORM_UWP
#if !UE_BUILD_SHIPPING
		// IF there's a "SIMUL" env variable, we can build shaders direct from there:
		FString SimulPath = GetEnvVariable(L"SIMUL");
		if (SimulPath.Len() > 0)
		{
			simul::internal_logs = true;
		}
#endif
#endif
#endif
		if(water_fails==0)
		{
#if ENABLE_TRUESKY_WATER
			UE_LOG(TrueSky, Display, TEXT("Simul water is supported."));
			SetWaterRenderingEnabled(true);
			ATrueSkyWater::InitializeTrueSkyWaterPropertyMap();
#endif
		}
#if PLATFORM_PS4 /*|| PLATFORM_SWITCH*/
		StaticSetMemoryInterface(simul::ue4::getMemoryInterface());
#endif
		StaticSetDebugOutputCallback(LogCallback);
		
		ATrueSkySequenceActor::InitializeTrueSkyPropertyMap( );
		return true;
	}
	return false;
}

void FTrueSkyPlugin::PlatformSetGraphicsDevice()
{
	void* currentDevice = GetPlatformDevice(CurrentRHIType);
#if PLATFORM_PS4
	StaticSetGraphicsDevice(currentDevice, CurrentRHIType, GraphicsEventInitialize);
#elif PLATFORM_SWITCH
    // Find a way of retrieving a commandBuffer
    // GDynamicRHI?
    StaticSetGraphicsDeviceAndContext(currentDevice, nullptr, CurrentRHIType, GraphicsEventInitialize);
#else
	if (CurrentRHIType == GraphicsDeviceD3D11 || CurrentRHIType == GraphicsDeviceD3D11_FastSemantics)
	{
		StaticSetGraphicsDevice(currentDevice, CurrentRHIType, GraphicsEventInitialize);
	}
#if PLATFORM_WINDOWS || PLATFORM_XBOXONE
    if (CurrentRHIType == GraphicsDeviceD3D12)
	{
		StaticSetGraphicsDeviceAndContext(currentDevice, GetContextD3D12(GDynamicRHI->RHIGetDefaultContext()), CurrentRHIType, GraphicsEventInitialize);
	}
#endif
#endif
#if SIMUL_SUPPORT_VULKAN
	else if (CurrentRHIType == GraphicsDeviceVulkan)
	{
		StaticSetGraphicsDeviceAndContext(currentDevice, GetContextVulkan(GDynamicRHI->RHIGetDefaultContext()), CurrentRHIType, GraphicsEventInitialize);
	}
#endif
}

bool FTrueSkyPlugin::EnsureRendererIsInitialized()
{
	// Cache the current rendering API from the dynamic RHI
	if (CurrentRHIType == GraphicsDeviceUnknown)
	{
		const TCHAR* rhiName = GDynamicRHI->GetName();

		std::string ansiRhiName(TCHAR_TO_ANSI(rhiName));
		if (ansiRhiName == "D3D11")
		{
#if USE_FAST_SEMANTICS_RENDER_CONTEXTS
			CurrentRHIType = GraphicsDeviceD3D11_FastSemantics;
#else
			CurrentRHIType = GraphicsDeviceD3D11;
#endif
		}
		else if (ansiRhiName == "D3D12")
		{
			CurrentRHIType = GraphicsDeviceD3D12;
		}
		else if (ansiRhiName == "Vulkan")
		{
			CurrentRHIType = GraphicsDeviceVulkan;
		}
		else if (ansiRhiName == "GNM")
		{
			CurrentRHIType = GraphicsDevicePS4;
		}
        else if (ansiRhiName == "NVN")
        {
            CurrentRHIType = GraphicsDeviceNVN;
        }
        else if (ansiRhiName == "Null")
		{
            CurrentRHIType = GraphicsDeviceNull;
			return false;
		}
		else
		{
			UE_LOG(TrueSky, Error, TEXT("trueSKY does not support the %s rendering API."), rhiName);
			return false;
		}
		UE_LOG(TrueSky, Display, TEXT("trueSKY will use the %s rendering API."), rhiName);
	}

	ERRNO_CLEAR
	if(!RendererInitialized)
	{
		if(InitRenderingInterface())
			RendererInitialized=true;
		if(!RendererInitialized)
			return false;
		void *device=GetPlatformDevice(CurrentRHIType);
		
		if( device != nullptr )
		{
			FString EnginePath=FPaths::EngineDir();
			StaticSetFileLoader(&ue4SimulFileLoader);
			FString shaderbin(FPaths::Combine(*EnginePath, TEXT("plugins/trueskyplugin/shaderbin/"),SimulUnrealPlatformName));
			FString texpath(FPaths::EngineDir()+TEXT("/plugins/trueskyplugin/resources/media/textures"));
			StaticPushPath("TexturePath", FStringToUtf8(texpath).c_str());
			FString texpath2(FPaths::Combine(*EnginePath,TEXT("/plugins/trueskyplugin/resources/media/textures")));
			StaticPushPath("TexturePath", FStringToUtf8(texpath2).c_str());

#if PLATFORM_LINUX
            FString texpath3(FPaths::EngineDir()+TEXT("/Plugins/TrueSkyPlugin/Resources/Media/textures"));
            StaticPushPath("TexturePath", FStringToUtf8(texpath3).c_str());
            FString texpath4(FPaths::Combine(*EnginePath,TEXT("/Plugins/TrueSkyPlugin/Resources/Media/textures")));
            StaticPushPath("TexturePath", FStringToUtf8(texpath4).c_str());
#endif
#if PLATFORM_WINDOWS || PLATFORM_UWP
			if(haveEditor)
			{
				StaticPushPath("ShaderPath",FStringToUtf8(trueSkyPluginPath+"\\Resources\\Platform\\DirectX11\\HLSL").c_str());
				StaticPushPath("TexturePath",FStringToUtf8(trueSkyPluginPath+"\\Resources\\Media\\Textures").c_str());
			}
			else
			{
				static FString gamePath="../../";
				StaticPushPath("ShaderPath",FStringToUtf8(gamePath+L"\\content\\trueskyplugin\\platform\\directx11\\hlsl").c_str());
				StaticPushPath("TexturePath",FStringToUtf8(gamePath+L"\\content\\trueskyplugin\\resources\\media\\textures").c_str());
			}
#endif
			if (CurrentRHIType == GraphicsDeviceVulkan)
			{
				shaderbin += "/Vulkan";
				FString shaderBinFull = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*shaderbin);
				StaticPushPath("ShaderBinaryPath", FStringToUtf8(shaderBinFull).c_str());
			}
			else if (CurrentRHIType == GraphicsDeviceD3D12)
			{
				FString shaderBinFull = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*shaderbin);
				StaticPushPath("ShaderBinaryPath", FStringToUtf8(shaderBinFull).c_str());
			}
			else
			{
				StaticPushPath("ShaderBinaryPath",FStringToUtf8(shaderbin).c_str());
			}
		}
		PlatformSetGraphicsDevice();
		if( device != nullptr )
		{
			RendererInitialized = true;
		}
		else
			RendererInitialized=false;
	}
	if(RendererInitialized)
	{
		StaticInitInterface();
		RenderingEnabled=actorCrossThreadProperties->Visible;
	}
	return RendererInitialized;
}

void FTrueSkyPlugin::SetPointLight(int id,FLinearColor c,FVector ue_pos,float min_radius,float max_radius)
{
	if(!StaticSetPointLight)
		return;
	criticalSection.Lock();
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FVector ts_pos=UEToTrueSkyPosition(A->Transform,A->MetresPerUnit,ue_pos);
	StaticSetPointLight(id,(const float*)&ue_pos,min_radius,max_radius,(const float*)&c);
	criticalSection.Unlock();
}

ITrueSkyPlugin::VolumeQueryResult FTrueSkyPlugin::GetStateAtPosition(int32 queryId,FVector ue_pos)
{
	VolumeQueryResult res;

	if(!StaticCloudPointQuery)
	{
		memset(&res,0,sizeof(res));
		return res;
	}
	criticalSection.Lock();
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticCloudPointQuery(queryId,(const float*)&ts_pos,&res);
	criticalSection.Unlock();
	return res;
}

ITrueSkyPlugin::LightingQueryResult FTrueSkyPlugin::GetLightingAtPosition(int32 queryId,FVector ue_pos)
{
	LightingQueryResult res;

	if(!StaticLightingQuery)
	{
		memset(&res,0,sizeof(res));
		return res;
	}
	criticalSection.Lock();
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticLightingQuery(queryId,(const float*)&ts_pos,&res);
	// Adjust for Brightness, so that light and rendered sky are consistent.
	for(int i=0;i<3;i++)
	{
		res.sunlight[i]	*=actorCrossThreadProperties->Brightness;
		res.moonlight[i]*=actorCrossThreadProperties->Brightness;
		res.ambient[i]	*=actorCrossThreadProperties->Brightness;
	}
	criticalSection.Unlock();
	// TODO: Deprecated function
	return res;
}

float FTrueSkyPlugin::GetCloudinessAtPosition(int32 queryId,FVector ue_pos)
{
	if(!StaticCloudPointQuery)
		return 0.f;
	criticalSection.Lock();
	VolumeQueryResult res;
	FVector ts_pos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_pos);
	StaticCloudPointQuery(queryId,(const float*)&ts_pos,&res);
	criticalSection.Unlock();
	return FMath::Clamp(res.density, 0.0f, 1.0f);
}

float FTrueSkyPlugin::CloudLineTest(int32 queryId,FVector ue_StartPos,FVector ue_EndPos)
{
	if(!StaticCloudLineQuery)
		return 0.f;
	criticalSection.Lock();
	LineQueryResult res;
	FVector StartPos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_StartPos);
	FVector EndPos=UEToTrueSkyPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,ue_EndPos);
	StaticCloudLineQuery(queryId,(const float*)&StartPos,(const float*)&EndPos,&res);
	criticalSection.Unlock();
	return FMath::Clamp(res.density, 0.0f, 1.0f);
}

void FTrueSkyPlugin::InitPaths()
{
#ifdef _MSC_VER

	static bool doOnce = false;
	if (!doOnce) {
		FModuleManager::Get().AddBinariesDirectory(FPaths::Combine((const TCHAR*)*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Simul/Win64")).GetCharArray().GetData(), false);
		doOnce = true;
	}
#endif
}

void FTrueSkyPlugin::OnToggleRendering()
{
	SetRenderingEnabled(!RenderingEnabled);
	if(RenderingEnabled)
	{
		SequenceChanged();
	}
}

void FTrueSkyPlugin::OnUIChangedSequence()
{
	SequenceChanged();
	// Make update instant, instead of gradual, if it's a change the user made.
	// For now, let's not do this in Real Time mode.
	if(actorCrossThreadProperties&&actorCrossThreadProperties->InterpolationMode!=(int)EInterpolationMode::RealTime)
		TriggerAction("Reset");
}

void FTrueSkyPlugin::OnUIChangedTime(float t)
{
	UTrueSkySequenceAsset* const ActiveSequence = GetActiveSequence();
	if(ActiveSequence)
	{
		actorCrossThreadProperties->Time=t;
		actorCrossThreadProperties->GetTime=true;
		actorCrossThreadProperties->SetTime=false;
		SetRenderFloat("time",actorCrossThreadProperties->Time);
	}
}

void FTrueSkyPlugin::ExportCloudLayer(const FString& fname,int index)
{
	exportNext=true;
	exportFilenameUtf8=fname;
}

void FTrueSkyPlugin::SequenceChanged()
{
	if(!StaticSetSequence && !StaticSetSequence2) return;
	
	UTrueSkySequenceAsset* const ActiveSequence = GetActiveSequence();
	if(ActiveSequence && ActiveSequence->SequenceText.Num() != 0)
	{
		if(StaticSetSequence2)
		{
			StaticSetSequence2((const char*)ActiveSequence->SequenceText.GetData());
		}
		else
		{
			std::string SequenceInputText;
			SequenceInputText = std::string((const char*)ActiveSequence->SequenceText.GetData());
			StaticSetSequence(SequenceInputText);
		}
	}
	else
	{
		if (StaticSetSequence2) StaticSetSequence2("");
		else if (StaticSetSequence) StaticSetSequence("");

		{
			SetRenderTexture(nullptr,"Cubemap", nullptr);
			SetRenderTexture(nullptr,"Moon", nullptr);
			SetRenderTexture(nullptr,"Background", nullptr);
			SetRenderTexture(nullptr,"Loss2D", nullptr);
			SetRenderTexture(nullptr,"Inscatter2D", nullptr);
			SetRenderTexture(nullptr,"CloudVisibilityRT", nullptr);
			SetRenderTexture(nullptr,"RainDepthTexture", nullptr);
			SetRenderTexture(nullptr,"CloudShadowRT", nullptr);
		}
	}

	sequenceInUse = ActiveSequence;
}

IMPLEMENT_TOGGLE(ShowFades)
IMPLEMENT_TOGGLE(ShowCompositing)
IMPLEMENT_TOGGLE(ShowWaterTextures)
IMPLEMENT_TOGGLE(ShowFlowRays)
IMPLEMENT_TOGGLE(Show3DCloudTextures)
IMPLEMENT_TOGGLE(Show2DCloudTextures)

bool FTrueSkyPlugin::IsToggleRenderingEnabled()
{
	if(GetActiveSequence())
	{
		return true;
	}
	// No active sequence found!
	SetRenderingEnabled(false);
	return false;
}

bool FTrueSkyPlugin::IsToggleRenderingChecked()
{
	return RenderingEnabled;
}

bool FTrueSkyPlugin::IsAddSequenceEnabled()
{
	// Returns false if TrueSkySequenceActor already exists!
	ULevel* const Level = GWorld->PersistentLevel;
	for(int i=0;i<Level->Actors.Num();i++)
	{
		if ( Cast<ATrueSkySequenceActor>(Level->Actors[i]) )
			return false;
	}
	return true;
}

void FTrueSkyPlugin::UpdateFromActor(FTrueSkyRenderDelegateParameters& RenderParameters)
{
	if(sequenceInUse!=GetActiveSequence() || actorCrossThreadProperties->Reset)
		SequenceChanged();
	if (!sequenceInUse || sequenceInUse->SequenceText.Num() == 0) //Make sure there is a valid sequence actor
	{
		//return;
		if(!actorCrossThreadProperties->Destroyed)
		{	
			// Destroyed.
			actorCrossThreadProperties->Visible=false;
		}
	}

	if(actorCrossThreadProperties->Visible!=RenderingEnabled)
	{
		OnToggleRendering();
	}
	if(RenderingEnabled&&RendererInitialized)
	{
		actorCrossThreadProperties->CriticalSection.Lock();

		SetInt(ETrueSkyPropertiesInt::TSPROPINT_WorleyTextureSize, actorCrossThreadProperties->WorleyNoiseTextureSize);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_InterpolationMode,(int)actorCrossThreadProperties->InterpolationMode);
		//SetInt(ETrueSkyPropertiesInt::TSPROPINT_InstantUpdate,(int)actorCrossThreadProperties->InstantUpdate);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_CloudSteps, actorCrossThreadProperties->DefaultCloudSteps);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_MaxViewAgeFrames, (int)GetDefault<UTrueSkySettings>()->MaxFramesBetweenViewUpdates);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_InterpolationSubdivisions, actorCrossThreadProperties->InterpolationSubdivisions);
		//SetInt(ETrueSkyPropertiesInt::TSPROPINT_PrecipitationOptions, actorCrossThreadProperties->PrecipitationOptions);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_MinimumStarPixelSize,actorCrossThreadProperties->MinimumStarPixelSize);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_NearestRainMetres, actorCrossThreadProperties->RainNearThreshold*actorCrossThreadProperties->MetresPerUnit);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_InterpolationTimeSeconds, actorCrossThreadProperties->InterpolationTimeSeconds);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_InterpolationTimeDays, actorCrossThreadProperties->InterpolationTimeHours / 24.0f );

#if ENABLE_TRUESKY_WATER
		if((simulVersion>=SIMUL_4_2) && WaterRenderingEnabled)
		{
			SetInt(ETrueSkyPropertiesInt::TSPROPINT_RenderWater, actorCrossThreadProperties->RenderWater);
			if (actorCrossThreadProperties->RenderWater && ((int)LastFrameNumber > 1)) // Make sure that the renderplatform has been initialised, so don't pass values on the first frame.
			{
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_EnableWaterReflection, actorCrossThreadProperties->EnableReflection);
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_WaterFullResolution, actorCrossThreadProperties->WaterFullResolution);
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_ReflectionFullResolution, actorCrossThreadProperties->ReflectionFullResolution);
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_ReflectionSteps, actorCrossThreadProperties->ReflectionSteps);
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_ReflectionPixelStep, actorCrossThreadProperties->ReflectionPixelStep);
				SetInt(ETrueSkyPropertiesInt::TSPROPINT_WaterUseGameTime, actorCrossThreadProperties->WaterUseGameTime);

				int ID = 0;
				bool boundlessOcean = false;
				for (auto propertyItr = waterActorsCrossThreadProperties.CreateIterator(); propertyItr; ++propertyItr)
				{
					if (propertyItr.Value() != nullptr)
					{
						WaterActorCrossThreadProperties* pProperty = propertyItr.Value();
						ID = propertyItr.Key();
						if (pProperty->Render && !pProperty->Destroyed)
						{
							if (pProperty->Updated)
							{
								FVector2D offsetAngles = { 0.f, 0.f };
								FMath::SinCos(&offsetAngles.X, &offsetAngles.Y, FMath::DegreesToRadians(-actorCrossThreadProperties->Rotation));

								if (pProperty->BoundlessOcean && !boundlessOcean)
								{
									boundlessOcean = true;
									FVector tempLocation = (actorCrossThreadProperties->WaterOffset + FVector(0.f, 0.f, pProperty->Location.Z + pProperty->Dimension.Z / (2.f * actorCrossThreadProperties->MetresPerUnit))) * actorCrossThreadProperties->MetresPerUnit;
									tempLocation = FVector(tempLocation.X * offsetAngles.Y - tempLocation.Y * offsetAngles.X, tempLocation.X * offsetAngles.X + tempLocation.Y * offsetAngles.Y, tempLocation.Z);
									
									SetInt(ETrueSkyPropertiesInt::TSPROPINT_EnableBoundlessOcean, pProperty->BoundlessOcean);
									SetWaterInt(ETrueSkyWaterPropertiesInt::TSPROPINT_Water_Render, ID, false);
									SetWaterVector(ETrueSkyWaterPropertiesVector::TSPROPVECTOR_Water_Location, -1, tempLocation);
									//SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_WindDirection, -1, (pProperty->WindDirection* 6.28f) - FMath::DegreesToRadians(actorCrossThreadProperties->Rotation));
									for (auto &t : pProperty->SetWaterUpdateValues)
									{
										int64 TrueSkyEnum = t.Key;
										if (TrueSkyEnum != 0)
										{
											StaticWaterSet(TrueSkyEnum, -1, (const VariantPass*)&t.Value);
										}
									}
									for (auto &t : pProperty->SetWaterUpdateVectors)
									{
										int64 TrueSkyEnum = t.Key;
										if (TrueSkyEnum != 0)
										{
											StaticWaterSet(TrueSkyEnum, -1, (const VariantPass*)&t.Value);
										}
									}
									SetWaterInt(ETrueSkyWaterPropertiesInt::TSPROPINT_Water_EnableFoam, -1, pProperty->EnableFoam);
									if (pProperty->EnableFoam && pProperty->AdvancedSurfaceOptions)
									{
										SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_FoamStrength, -1, pProperty->FoamStrength);
									}
									SetWaterInt(ETrueSkyWaterPropertiesInt::TSPROPINT_Water_EnableShoreEffects, -1, pProperty->EnableShoreEffects);

									if (pProperty->ShoreDepthRT && pProperty->EnableShoreEffects)
									{
										FVector tempLocation2 = (pProperty->ShoreDepthTextureLocation + actorCrossThreadProperties->WaterOffset)* actorCrossThreadProperties->MetresPerUnit;
										StaticSetRenderTexture("ShoreDepthTexture", GetPlatformTexturePtr(pProperty->ShoreDepthRT, CurrentRHIType));
										SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_ShoreDepthExtent, -1, pProperty->ShoreDepthExtent * actorCrossThreadProperties->MetresPerUnit);
										SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_ShoreDepthWidth, -1, pProperty->ShoreDepthWidth * actorCrossThreadProperties->MetresPerUnit);
										SetWaterVector(ETrueSkyWaterPropertiesVector::TSPROPVECTOR_Water_ShoreDepthTextureLocation, -1, tempLocation2);
									}

									pProperty->SetWaterUpdateValues.Empty();
									pProperty->SetWaterUpdateVectors.Empty();
									pProperty->Updated = false;
								}
								else
								{
									FVector tempLocation = (pProperty->Location + actorCrossThreadProperties->WaterOffset + FVector(0.f, 0.f, pProperty->Dimension.Z / (2.f * actorCrossThreadProperties->MetresPerUnit))) * actorCrossThreadProperties->MetresPerUnit;
									tempLocation = FVector(tempLocation.X * offsetAngles.Y - tempLocation.Y * offsetAngles.X, tempLocation.X * offsetAngles.X + tempLocation.Y * offsetAngles.Y, tempLocation.Z);

									SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_WindDirection, ID, (pProperty->WindDirection* 6.28f) - FMath::DegreesToRadians(actorCrossThreadProperties->Rotation));
									SetWaterVector(ETrueSkyWaterPropertiesVector::TSPROPVECTOR_Water_Location, ID, tempLocation);
									for (auto &t : pProperty->SetWaterUpdateValues)
									{
										int64 TrueSkyEnum = t.Key;
										if (TrueSkyEnum != 0)
										{
											StaticWaterSet(TrueSkyEnum, ID, (const VariantPass*)&t.Value);
										}
									}
									for (auto &t : pProperty->SetWaterUpdateVectors)
									{
										int64 TrueSkyEnum = t.Key;
										if (TrueSkyEnum != 0)
										{
											StaticWaterSet(TrueSkyEnum, ID, (const VariantPass*)&t.Value);
										}
									}
									pProperty->SetWaterUpdateValues.Empty();
									pProperty->SetWaterUpdateVectors.Empty();
									pProperty->Updated = false;
								}
							}
							else if (pProperty->BoundlessOcean)
								boundlessOcean = true;
						}
						else 
						{
							if (pProperty->BoundlessOcean)
								SetInt(ETrueSkyPropertiesInt::TSPROPINT_EnableBoundlessOcean, false);
							SetWaterInt(ETrueSkyWaterPropertiesInt::TSPROPINT_Water_Render, ID, false);
						}
							

						if (actorCrossThreadProperties->alteredPosition)
						{
							pProperty->Reset = true;
						}
						
					}
				}
				if (boundlessOcean == false)
					SetInt(ETrueSkyPropertiesInt::TSPROPINT_EnableBoundlessOcean, false);
			}
		}
#endif
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_CloudThresholdDistanceKm, actorCrossThreadProperties->CloudThresholdDistanceKm);
		//SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_CloudshadowRangeKm, actorCrossThreadProperties->CloudShadowRangeKm);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_MaximumCubemapResolution, actorCrossThreadProperties->MaximumResolution);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_GridRendering, (int)actorCrossThreadProperties->GridRendering);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_GodraysGridX, actorCrossThreadProperties->GodraysGrid[0]);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_GodraysGridY, actorCrossThreadProperties->GodraysGrid[1]);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_GodraysGridZ, actorCrossThreadProperties->GodraysGrid[2]);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_DepthSamplingPixelRange,actorCrossThreadProperties->DepthSamplingPixelRange);
		SetFloat(ETrueSkyPropertiesFloat::TSPROPFLOAT_DepthTemporalAlpha,actorCrossThreadProperties->DepthTemporalAlpha);
		if(!GlobalOverlayFlag)
			GlobalOverlayFlag = StaticGetRenderBool("AnyOverlays");
		else
		{
			SetRenderInt("Downscale",std::min(8,std::max(1,1920/actorCrossThreadProperties->MaximumResolution)));
		}

		for (const auto &q : actorCrossThreadProperties->LightingQueries)
		{
			ITrueSkyPlugin::LightingQueryResult *res = actorCrossThreadProperties->LightingQueries.Find(q.Key);
			StaticLightingQuery(q.Key, (const float*)&q.Value.pos, res);
			// Adjust for Brightness, so that light and rendered sky are consistent.
			for (int i = 0; i < 3; i++)
			{
				res->sunlight[i] *= actorCrossThreadProperties->Brightness;
				res->moonlight[i] *= actorCrossThreadProperties->Brightness;
				res->ambient[i] *= actorCrossThreadProperties->Brightness;
			}
		}
		if(StaticSetMoon)
		{
			for (auto &q : actorCrossThreadProperties->Moons)
			{
				ExternalMoonProxy &Moon=q.Value;
				if(!Moon.AddDelete)
					StaticSetMoon(q.Key,nullptr);
				else
				{
					ExternalMoon moon;
					InitExternalTexture(&RenderParameters.RHICmdList->GetContext(),moon.texture, Moon.Texture.GetReference(),CurrentRHIType);
					moon.illuminated=Moon.Illuminated;
					moon.orbit=Moon.orbit;
					moon.radiusArcMinutes=Moon.RadiusArcMinutes;
					std::string name=FStringToUtf8(Moon.Name);
					moon.name=name.c_str();
					StaticSetMoon(q.Key,&moon);
				}
			}
		}
		actorCrossThreadProperties->Moons.Empty();
		for(const auto &t:actorCrossThreadProperties->SetVectors)
		{
			int64 TrueSkyEnum=t.Key;
			const vec3 &v=t.Value;
			StaticSet(TrueSkyEnum,(const VariantPass*)&v);
		}
		for(const auto &t:actorCrossThreadProperties->SetFloats)
		{
			int64 TrueSkyEnum = t.Key;
			StaticSet(TrueSkyEnum,(const VariantPass*)&t.Value);
		}
		for(const auto &t:actorCrossThreadProperties->SetInts)
		{
			int64 TrueSkyEnum = t.Key;
			StaticSet(TrueSkyEnum,(const VariantPass*)&t.Value);
		}
		if(ATrueSkySequenceActor::TrueSkyPropertyFloatMaps.Num())
		{
			for(const auto &t:actorCrossThreadProperties->SetRenderingValues)
			{
				int64 TrueSkyEnum = t.Key;
				StaticSet(TrueSkyEnum,(const VariantPass*)&t.Value);
			}
			actorCrossThreadProperties->SetRenderingValues.Empty();
		}

		actorCrossThreadProperties->SetVectors.Empty();
		actorCrossThreadProperties->SetFloats.Empty();
		actorCrossThreadProperties->SetInts.Empty();
		actorCrossThreadProperties->CriticalSection.Unlock();
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_CloudShadowTextureSize, actorCrossThreadProperties->CloudShadowTextureSize);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_Amortization,actorCrossThreadProperties->Amortization);
		SetInt(ETrueSkyPropertiesInt::TSPROPINT_AtmosphericsAmortization, actorCrossThreadProperties->AtmosphericsAmortization);
		// Query active strikes:
        int num_l=StaticGetLightningBolts(actorCrossThreadProperties->lightningBolts,4);
        for (int i = 0; i < num_l; i++)
		{
			LightningBolt* l= &actorCrossThreadProperties->lightningBolts[i];
			
            FVector u       = TrueSkyToUEPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,FVector(l->pos[0],l->pos[1],l->pos[2]));
			FVector v       = TrueSkyToUEPosition(actorCrossThreadProperties->Transform,actorCrossThreadProperties->MetresPerUnit,FVector(l->endpos[0],l->endpos[1],l->endpos[2]));
			
            l->pos[0]       = u.X;
			l->pos[1]       = u.Y;
			l->pos[2]       = u.Z;

			l->endpos[0]    = v.X;
			l->endpos[1]    = v.Y;
			l->endpos[2]    = v.Z;
		}

		if (actorCrossThreadProperties->alteredPosition)
		{
			actorCrossThreadProperties->alteredPosition = false;
		}
	}

	if(actorCrossThreadProperties->Reset)
	{
		StaticTriggerAction("Reset");
		actorCrossThreadProperties->Reset = false;
	}
}

UTrueSkySequenceAsset* FTrueSkyPlugin::GetActiveSequence()
{
	if(!actorCrossThreadProperties)
		return nullptr;
	UTrueSkySequenceAsset *ActiveSequence = actorCrossThreadProperties->activeSequence.GetEvenIfUnreachable();
	if (ActiveSequence && !ActiveSequence->IsPendingKill() && ActiveSequence->IsValidLowLevel())
	{
		return ActiveSequence;
	}
	return nullptr;
}

FMatrix FTrueSkyPlugin::UEToTrueSkyMatrix(bool apply_scale) const
{
	FMatrix TsToUe	=actorCrossThreadProperties->Transform.ToMatrixWithScale();
	FMatrix UeToTs	=TsToUe.InverseFast();
	for(int i=apply_scale?0:3;i<4;i++)
	{
		for(int j=0;j<3;j++)
		{
			UeToTs.M[i][j]			*=actorCrossThreadProperties->MetresPerUnit;
		}
	}

	for(int i=0;i<4;i++)
		std::swap(UeToTs.M[i][0],UeToTs.M[i][1]);
	return UeToTs;
}

FMatrix FTrueSkyPlugin::TrueSkyToUEMatrix(bool apply_scale) const
{
	FMatrix TsToUe	=actorCrossThreadProperties->Transform.ToMatrixWithScale();
	for(int i=0;i<4;i++)
		std::swap(TsToUe.M[i][0],TsToUe.M[i][1]);
	for(int i=apply_scale?0:3;i<4;i++)
	{
		for(int j=0;j<3;j++)
		{
			TsToUe.M[i][j]			/=actorCrossThreadProperties->MetresPerUnit;
		}
	}
	return TsToUe;
}
namespace simul
{
	namespace unreal
	{
		FVector UEToTrueSkyPosition(const FTransform &tr,float MetresPerUnit,FVector ue_pos) 
		{
			FMatrix TsToUe	=tr.ToMatrixWithScale();
			FVector ts_pos	=tr.InverseTransformPosition(ue_pos);
			ts_pos			*=MetresPerUnit;
			std::swap(ts_pos.Y,ts_pos.X);
			return ts_pos;
		}

		FVector TrueSkyToUEPosition(const FTransform &tr,float MetresPerUnit,FVector ts_pos) 
		{
			ts_pos			/=actorCrossThreadProperties->MetresPerUnit;
			std::swap(ts_pos.Y,ts_pos.X);
			FVector ue_pos	=tr.TransformPosition(ts_pos);
			return ue_pos;
		}

		FVector UEToTrueSkyDirection(const FTransform &tr,FVector ue_dir) 
		{
			FMatrix TsToUe	=tr.ToMatrixNoScale();
			FVector ts_dir	=tr.InverseTransformVectorNoScale(ue_dir);
			std::swap(ts_dir.Y,ts_dir.X);
			return ts_dir;
		}

		FVector TrueSkyToUEDirection(const FTransform &tr,FVector ts_dir) 
		{
			std::swap(ts_dir.Y,ts_dir.X);
			FVector ue_dir	=tr.TransformVectorNoScale(ts_dir);
			return ue_dir;
		}
	}
}

#undef LOCTEXT_NAMESPACE
#undef LLOCTEXT_NAMESPACE
