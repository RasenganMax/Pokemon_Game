#if SIMUL_SUPPORT_VULKAN 
#include "TrueSkyPluginVulkan.h"
#include "Logging/LogMacros.h"
#undef UE_LOG
#define UE_LOG(...)
#define LogVulkanRHI TrueSky1
#define LogVulkan TrueSky2
#if (PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_LINUX ) && !PLATFORM_SWITCH
	
#include "VulkanRHIPrivate.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "Logging/LogMacros.h"
#include "PostProcess/SceneRenderTargets.h"

//DEFINE_LOG_CATEGORY(TrueSky1);
//DEFINE_LOG_CATEGORY(TrueSky2);


void **simul::GetDeviceVulkan(FDynamicRHI *rhi)
{
	static void *vkDevice_vkInstance_gpu[3] = { 0,0,0 };
	FVulkanDynamicRHI *VulkanDynamicRHI=((FVulkanDynamicRHI*)rhi);
	FVulkanDevice *VulkanDevice = VulkanDynamicRHI->GetDevice();
	static VkPhysicalDevice gpu;
	static VkDevice dev;
	static VkInstance inst;
	gpu= VulkanDevice->GetPhysicalHandle();
	dev= VulkanDevice->GetInstanceHandle();
	inst= VulkanDynamicRHI->GetInstance();
	vkDevice_vkInstance_gpu[0] = &dev;
	vkDevice_vkInstance_gpu[1] = &inst;
	vkDevice_vkInstance_gpu[2] = &gpu;
	return vkDevice_vkInstance_gpu;
}

void *simul::GetContextVulkan(IRHICommandContext* cmdContext)
{
	FVulkanCommandListContext* VulkanCommandListContext = (FVulkanCommandListContext*)cmdContext;
	FVulkanCmdBuffer *b = VulkanCommandListContext->GetCommandBufferManager()->GetActiveCmdBuffer();
	// TODO: not ideal, what about multithread cases?
	static VkCommandBuffer vkCommandBuffer;
	if (b)
	{
		vkCommandBuffer= b->GetHandle();
		return (void*)&vkCommandBuffer;
	}
	else
		return nullptr;
}

VkImage simul::GetRenderTargetVulkan(FRHITexture2D* tex)
{
	FVulkanTexture2D *t = static_cast<FVulkanTexture2D*>(tex);

	VkImage im=(VkImage)tex->GetNativeResource();
	return im;
}

void simul::RestoreHeapsVulkan(IRHICommandContext* cmdContext)
{
	// This can be replaced by using DirtyState() only?
	FVulkanCommandListContext* VulkanCommandListContext	= (FVulkanCommandListContext*)cmdContext;

}

bool simul::EnsureWriteStateVulkan(IRHICommandContext* cmdContext, FRHITexture2D* res)
{
	FVulkanTextureBase* base = GetVulkanTextureFromRHITexture(res);
	if (base)
	{
		/*FVulkanCommandListContext* VulkanCommandListContext	= (FVulkanCommandListContext*)cmdContext;
		auto writeState						= base->GetResource()->GetWritableState();
		FVulkanDynamicRHI::Transit(VulkanCommandListContext->CommandListHandle, base->GetResource(), writeState, Vulkan_RESOURCE_BARRIER_ALL_SUBRESOURCES);*/
		return true;
	}
	return false;
}

bool simul::EnsureReadStateVulkan(IRHICommandContext* cmdContext, FRHITexture2D* res)
{
	FVulkanTextureBase* base = GetVulkanTextureFromRHITexture(res);
	if (base)
	{
		return true;
	}
	return false;
}

void simul::PostOpaqueStoreStateVulkan(FSceneRenderTargets *SceneContext, FRHICommandList* RHICmdList, FRHITexture2D* colour, FRHITexture2D* depth)
{
	SceneContext->FinishRenderingSceneColor(*RHICmdList);
}

void simul::PostOpaqueRestoreStateVulkan(FSceneRenderTargets *SceneContext, FRHICommandList* RHICmdList, FRHITexture2D* colour, FRHITexture2D* depth)
{
	SceneContext->BeginRenderingSceneColor(*RHICmdList);
}

void simul::StoreStateVulkan(IRHICommandContext* ctx)
{
	ctx->RHISubmitCommandsHint();
}

void simul::RestoreStateVulkan(IRHICommandContext* ctx)
{
}

void simul::OverlaysStoreStateVulkan(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
}

void simul::OverlaysRestoreStateVulkan(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
}

simul::crossplatform::ResourceState VulkanLayoutToResourceState(VkImageLayout l)
{
	switch(l)
	{
	case VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return simul::crossplatform::ResourceState::RENDER_TARGET;
	case VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return simul::crossplatform::ResourceState::DEPTH_WRITE;
	case VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return simul::crossplatform::ResourceState::SHADER_RESOURCE;
	default:
		return simul::crossplatform::ResourceState::UNKNOWN;
	};
}

simul::crossplatform::ResourceState simul::GetVulkanTextureResourceState(IRHICommandContext* ctx, FRHITexture* t)
{
	FVulkanTextureBase* base = GetVulkanTextureFromRHITexture(t);
	if (base)
	{
		VkImage &res=base->Surface.Image;
		
		{
			FVulkanCommandListContext* VulkanCommandListContext	= (FVulkanCommandListContext*)ctx;
			VkImageLayout l=VulkanCommandListContext->GetLayoutForDescriptor(base->Surface);
			return VulkanLayoutToResourceState(l);
		}
	}
	return simul::crossplatform::ResourceState::COMMON;
}
#endif
#endif