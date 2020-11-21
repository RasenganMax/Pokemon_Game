/*
	TrueSkyPluginD3D12.h
*/

#pragma once

#include "CoreMinimal.h"

#include "ModifyDefinitions.h"
#include "TrueSkyPluginPrivatePCH.h"

#ifndef PLATFORM_UWP
	#define PLATFORM_UWP 0
#endif

#if (( PLATFORM_WINDOWS) || (PLATFORM_UWP) || (PLATFORM_XBOXONE )) && !PLATFORM_SWITCH

struct CD3DX12_CPU_DESCRIPTOR_HANDLE;
class FRHITexture2D;
class IRHICommandContext;
enum D3D12_RESOURCE_STATES;

namespace simul
{
	bool IsD3D12ContextOK(IRHICommandContext* ctx);
	//! Returns the graphical command list referenced in the provided command context
	void* GetContextD3D12(IRHICommandContext* cmdContext);
	
	//! Returns the render target handle of the provided texture
	CD3DX12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetD3D12(FRHITexture2D* tex);
	
	//! Restores UE4 descriptor heaps
	void RestoreHeapsD3D12(IRHICommandContext* cmdContext);

	bool EnsureWriteStateD3D12(IRHICommandContext* cmdContext, FRHITexture* res);

	bool EnsureReadStateD3D12(IRHICommandContext* cmdContext, FRHITexture* res);

	void PostOpaqueStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth, FRHITexture* cloudShadow);

	void PostOpaqueRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth, FRHITexture* cloudShadow);

	void PostTranslucentStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depthr);

	void PostTranslucentRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void OverlaysStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	void OverlaysRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth);

	D3D12_RESOURCE_STATES RHIResourceTransitionTexture2DFromCurrentD3D12(IRHICommandContext* ctx, FRHITexture* texture, D3D12_RESOURCE_STATES default_before, D3D12_RESOURCE_STATES after);
	D3D12_RESOURCE_STATES RHIResourceTransitionTextureCubeFromCurrentD3D12(IRHICommandContext* ctx, FRHITexture* texture, D3D12_RESOURCE_STATES default_before, D3D12_RESOURCE_STATES after);
	//! Unreal's barrier system is WIP (and the d3d12rhi barriers force you to use a resource with tracking enabled) this
	//! method basically performs a d3d12 barrier on the ID3D12GraphicsCommandList
	void RHIResourceTransitionD3D12(IRHICommandContext* ctx, FRHITexture* texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	
	simul::crossplatform::ResourceState GetD3D12TextureResourceState(IRHICommandContext &cmdContext,FRHITexture* tex);
}

#endif