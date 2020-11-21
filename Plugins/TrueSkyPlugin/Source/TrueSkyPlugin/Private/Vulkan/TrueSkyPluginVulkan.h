/*
	TrueSkyPluginVulkan.h
*/

#pragma once
#if SIMUL_SUPPORT_VULKAN 
//#define LogVulkanRHI TrueSky1
//#define LogVulkan TrueSky2
#include "CoreMinimal.h"

#include "ModifyDefinitions.h"
#include "TrueSkyPluginPrivatePCH.h"

#ifndef PLATFORM_UWP
	#define PLATFORM_UWP 0
#endif

#if (PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_LINUX )  && !PLATFORM_SWITCH
#define VULKAN_SUPPORT 1
#include "VulkanPlatformDefines.h"
//struct CD3DX12_CPU_DESCRIPTOR_HANDLE;
class FRHITexture2D;
class IRHICommandContext;
class FSceneRenderTargets;
class FRHITexture2D;
namespace simul
{
	void **GetDeviceVulkan(FDynamicRHI *rhi);
	//! Returns the graphical command list referenced in the provided command context
	void *GetContextVulkan(IRHICommandContext* cmdContext);
	
	//! Returns the render target handle of the provided texture
	VkImage GetRenderTargetVulkan(FRHITexture2D* tex);
	
	//! Restores UE4 descriptor heaps
	void RestoreHeapsVulkan(IRHICommandContext* cmdContext);

	bool EnsureWriteStateVulkan(IRHICommandContext* cmdContext, FRHITexture2D* res);

	bool EnsureReadStateVulkan(IRHICommandContext* cmdContext, FRHITexture2D* res);

	void PostOpaqueStoreStateVulkan(FSceneRenderTargets *SceneContext, FRHICommandList* RHICmdList,FRHITexture2D* colour, FRHITexture2D* depth);

	void PostOpaqueRestoreStateVulkan(FSceneRenderTargets *SceneContext, FRHICommandList* RHICmdList,FRHITexture2D* colour, FRHITexture2D* depth);

	void StoreStateVulkan(IRHICommandContext* ctx);

	void RestoreStateVulkan(IRHICommandContext* ctx);

	void OverlaysStoreStateVulkan(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void OverlaysRestoreStateVulkan(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	crossplatform::ResourceState GetVulkanTextureResourceState(IRHICommandContext* ctx, FRHITexture* t);
	//! Unreal's barrier system is WIP (and the Vulkan rhi barriers force you to use a resource with tracking enabled) this
	//! method basically performs a Vulkan barrier on the IVulkanGraphicsCommandList
	//void RHIResourceTransitionVulkan(IRHICommandContext* ctx, Vulkan_RESOURCE_STATES before, Vulkan_RESOURCE_STATES after, FRHITexture2D* texture);
}
#undef LogVulkanRHI
#else
#define VULKAN_SUPPORT 0
#endif
#endif