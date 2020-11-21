#include "TrueSkyPluginD3D12.h"

#if ((PLATFORM_WINDOWS) || (PLATFORM_UWP) || (PLATFORM_XBOXONE) ) && !PLATFORM_SWITCH

	/*
		We define these macros here to prevent collisions with method names
		between dx11 and dx12
	*/
	#define FindShaderResourceDXGIFormat	FindShaderResourceDXGIFormat_D3D12
	#define FindUnorderedAccessDXGIFormat	FindUnorderedAccessDXGIFormat_D3D12
	#define FindDepthStencilDXGIFormat		FindDepthStencilDXGIFormat_D3D12
	#define HasStencilBits					HasStencilBits_D3D12
	#define GetRenderTargetFormat			GetRenderTargetFormat_D3D12
	#define FD3DGPUProfiler					FD3DGPUProfiler_D3D12
	#define FVector4VertexDeclaration		FVector4VertexDeclaration_D3D12

	#define Initialize						Initialize_D3D12_Custom
	#define CheckResourceState				CheckResourceState_D3D12_Custom
	#define ConditionalInitalize			ConditionalInitalize_D3D12_Custom 
	#define	AreAllSubresourcesSame			AreAllSubresourcesSame_D3D12_Custom
	#define	CheckResourceStateInitalized	CheckResourceStateInitalized_D3D12_Custom
	#define	GetSubresourceState				GetSubresourceState_D3D12_Custom
	#define	SetResourceState				SetResourceState_D3D12_Custom
	#define	SetResourceState				SetResourceState_D3D12_Custom
	#define	SetSubresourceState				SetSubresourceState_D3D12_Custom
	
#if !WITH_EDITOR
	#define	UpdateResidency					UpdateResidency_D3D12_Custom
#endif

	#define	GetResourceState				GetResourceState_D3D12_Custom
	//#define	AddTransitionBarrier			AddTransitionBarrier_D3D12_Custom

	#include "D3D12RHI.h"
	#include "D3D12RHIPrivate.h"
	#include "D3D12StateCachePrivate.h"
	#include "D3D12CommandList.h"

#if PLATFORM_WINDOWS && UE_EDITOR
bool GEnableResidencyManagement = true;
#endif

#include "TrueSkyD3D12Util.h"
#include "TrueSkyD3D12Resources.h"
#include "TrueSkyD3D12CommandList.h"
#include "TrueSkyD3D12StateCache.h"

void* simul::GetContextD3D12(IRHICommandContext* cmdContext)
{
	FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)cmdContext;
	if (cmdContext12->CommandListHandle.IsClosed())
	{
		return nullptr;
	}
	return cmdContext12->CommandListHandle.CommandList();
}

TArray<CD3DX12_CPU_DESCRIPTOR_HANDLE> cachedHandles;
int curHandleIdx = 0;
CD3DX12_CPU_DESCRIPTOR_HANDLE* simul::GetRenderTargetD3D12(FRHITexture2D* tex)
{
	if (cachedHandles.Num() == 0)
	{
		cachedHandles.Init(CD3DX12_CPU_DESCRIPTOR_HANDLE(),10);
		curHandleIdx = 0;
	}
	curHandleIdx = (curHandleIdx + 1) % 10;
	auto texD12		= (FD3D12Texture2D*)tex;
	cachedHandles[curHandleIdx] = texD12->GetRenderTargetView(0, 0)->GetView();
	return &cachedHandles[curHandleIdx];
}
simul::crossplatform::ResourceState simul::GetD3D12TextureResourceState(IRHICommandContext& cmdContext,FRHITexture* texture)
{
	simul::crossplatform::ResourceState resourceState=simul::crossplatform::ResourceState::UNKNOWN;
	D3D12_RESOURCE_STATES state;
	FD3D12Texture2D *t2d =static_cast<FD3D12Texture2D*>(texture);
	FD3D12Resource *res = t2d->GetResource();
	if (res&&res->RequiresResourceStateTracking())
	{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)&cmdContext;
		CResourceState& ResourceState = cmdContext12->CommandListHandle.GetResourceState(res);
		state=ResourceState.GetSubresourceState_D3D12_Custom(0);
		if((int)state==-1)
		{
			// Wild guess, because UE won't tell us:
			if(t2d->HasRenderTargetViews())
			{
				state=D3D12_RESOURCE_STATE_RENDER_TARGET;
				EnsureWriteStateD3D12(&cmdContext, texture);
				cmdContext12->CommandListHandle.FlushResourceBarriers();
			}
			//It's uninitialized. We will assume GENERIC_READ - AJR.
			else
			{
				state=D3D12_RESOURCE_STATE_GENERIC_READ;
			}
			ResourceState.SetResourceState(state);
		}
		resourceState=(simul::crossplatform::ResourceState)state;
	}
	else if(res)
		resourceState=simul::crossplatform::ResourceState::SHADER_RESOURCE;
	return resourceState;
}

void simul::RestoreHeapsD3D12(IRHICommandContext* cmdContext)
{
	// This can be replaced by using DirtyState() only?
	FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
	if (!cmdContext12->CommandListHandle || cmdContext12->CommandListHandle.IsClosed())
		return;
	auto desCache						= cmdContext12->StateCache.GetDescriptorCache();

	// It has to be in the same order as in: D3D12DescriptorCache.cpp FD3D12DescriptorCache::SetDescriptorHeaps()
	ID3D12DescriptorHeap* pHeaps[]		= { desCache->GetViewDescriptorHeap(),desCache->GetSamplerDescriptorHeap() };
	if(pHeaps[0]!=nullptr&&pHeaps[1]!=nullptr)
	{
		cmdContext12->CommandListHandle->SetDescriptorHeaps(_countof(pHeaps), pHeaps);
	}
	else
	{
		cmdContext12->CommandListHandle->SetDescriptorHeaps(0, nullptr);
	}
	cmdContext12->StateCache.ForceSetGraphicsRootSignature();
	cmdContext12->StateCache.ForceSetComputeRootSignature();
	// Mark as dirty rendering state
	// cmdContext12->StateCache.ForceRebuildGraphicsPSO();
	// cmdContext12->StateCache.ForceRebuildComputePSO();
	cmdContext12->StateCache.ForceSetVB();
	cmdContext12->StateCache.ForceSetIB();
	cmdContext12->StateCache.ForceSetRTs();
	cmdContext12->StateCache.ForceSetSOs();
	cmdContext12->StateCache.ForceSetViewports();
	cmdContext12->StateCache.ForceSetScissorRects();
	cmdContext12->StateCache.ForceSetPrimitiveTopology();
	cmdContext12->StateCache.ForceSetBlendFactor();
	cmdContext12->StateCache.ForceSetStencilRef();
	cmdContext12->StateCache.DirtyState();
	return;
}

bool simul::EnsureWriteStateD3D12(IRHICommandContext* cmdContext, FRHITexture* res)
{
	FD3D12TextureBase* base = GetD3D12TextureFromRHITexture(res);
	if (base)
	{
		FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
		auto writeState						= base->GetResource()->GetWritableState();
		FD3D12DynamicRHI::TransitionResourceWithTracking(cmdContext12->CommandListHandle, base->GetResource(), writeState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		return true;
}
	return false;
}

bool simul::EnsureReadStateD3D12(IRHICommandContext* cmdContext, FRHITexture* res)
{
	FD3D12TextureBase* base = GetD3D12TextureFromRHITexture(res);
	if (base)
	{
		FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)cmdContext;
		auto readState						= base->GetResource()->GetReadableState();
		FD3D12DynamicRHI::TransitionResourceWithTracking(cmdContext12->CommandListHandle, base->GetResource(), readState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		return true;
	}
	return false;
}

bool simul::IsD3D12ContextOK(IRHICommandContext* ctx)
{
	FD3D12CommandContext* cmdContext12 = static_cast<FD3D12CommandContext*>(ctx);
	if (!cmdContext12)
		return false;
	return !cmdContext12->CommandListHandle.IsClosed();
}
static D3D12_RESOURCE_STATES cloudShadowState= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
void simul::PostOpaqueStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth, FRHITexture *cloudShadow)
{
#if 1
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
		EnsureReadStateD3D12(ctx, depth);
		cmdContext12->CommandListHandle.FlushResourceBarriers();
#else
	return;
	FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;

	EnsureWriteStateD3D12(ctx, colour);
	EnsureReadStateD3D12(ctx, depth);

	if (cloudShadow)
	{
		cloudShadowState=RHIResourceTransitionFromCurrentD3D12(ctx, cloudShadow, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
		//		RHIResourceTransitionFromCurrentD3D12(CommandListContext, shadowRtRef, (D3D12_RESOURCE_STATES)0x4);
		//			RHIResourceTransitionD3D12(ctx, (D3D12_RESOURCE_STATES)(((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800), (D3D12_RESOURCE_STATES)0x4, cloudShadow);
	}
	cmdContext12->CommandListHandle.FlushResourceBarriers();

	cmdContext12->CommandListHandle->OMSetRenderTargets(1, (D3D12_CPU_DESCRIPTOR_HANDLE*)GetRenderTargetD3D12(colour), false, nullptr);

	// Parallel rendering: dummy "work" in order to correctly close the context (see HasDoneWork implementation, called by FD3D12CommandContext::Finish)
	cmdContext12->otherWorkCounter++;
#endif
}

void simul::PostOpaqueRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth, FRHITexture* cloudShadow)
{
#if 1
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
		cmdContext12->CommandListHandle.FlushResourceBarriers();
#else
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
		EnsureWriteStateD3D12(ctx, colour);
		EnsureReadStateD3D12(ctx, depth);
		if (cloudShadow)
			RHIResourceTransitionD3D12(ctx, cloudShadow, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, cloudShadowState/*(D3D12_RESOURCE_STATES)(((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800)*/);
		cmdContext12->CommandListHandle.FlushResourceBarriers();
		cmdContext12->CommandListHandle->OMSetRenderTargets(1, (D3D12_CPU_DESCRIPTOR_HANDLE*)GetRenderTargetD3D12(colour), false, nullptr);
		// Parallel rendering: dummy "work" in order to correctly close the context (see HasDoneWork implementation, called by FD3D12CommandContext::Finish)
		cmdContext12->otherWorkCounter++;
#endif
}

void simul::PostTranslucentStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
	FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;

	EnsureWriteStateD3D12(ctx, colour);
	EnsureReadStateD3D12(ctx, depth);
	cmdContext12->CommandListHandle.FlushResourceBarriers();

	cmdContext12->CommandListHandle->OMSetRenderTargets(1, (D3D12_CPU_DESCRIPTOR_HANDLE*)GetRenderTargetD3D12(colour), false, nullptr);

	// Parallel rendering: dummy "work" in order to correctly close the context (see HasDoneWork implementation, called by FD3D12CommandContext::Finish)
	cmdContext12->otherWorkCounter++;
}

void simul::PostTranslucentRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
	FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
	cmdContext12->CommandListHandle.FlushResourceBarriers();
}

void simul::OverlaysStoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
	EnsureWriteStateD3D12(ctx, colour);
	EnsureReadStateD3D12(ctx, depth);

	// Parallel rendering: dummy "work" in order to correctly close the context (see HasDoneWork implementation, called by FD3D12CommandContext::Finish)
	FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
	cmdContext12->otherWorkCounter++;
}

void simul::OverlaysRestoreStateD3D12(IRHICommandContext* ctx, FRHITexture2D* colour, FRHITexture2D* depth)
{
}

static D3D12_RESOURCE_STATES RHIResourceTransitionFromCurrentD3D12(IRHICommandContext* ctx, FRHITexture* rhit,FD3D12Resource* res, D3D12_RESOURCE_STATES default_before, D3D12_RESOURCE_STATES after)
{
	if (!res)
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES before;
	if (res->RequiresResourceStateTracking())
	{
		FD3D12CommandContext* cmdContext12 = (FD3D12CommandContext*)ctx;
		CResourceState& ResourceState = cmdContext12->CommandListHandle.GetResourceState(res);
		before = ResourceState.GetSubresourceState_D3D12_Custom(0);
	}
	else
	{
		before = default_before;
	}
	simul::RHIResourceTransitionD3D12(ctx, rhit, before, after);
	return before;
}

D3D12_RESOURCE_STATES simul::RHIResourceTransitionTexture2DFromCurrentD3D12(IRHICommandContext* ctx, FRHITexture* rhit, D3D12_RESOURCE_STATES default_before, D3D12_RESOURCE_STATES after)
{
	auto *t2d =(FD3D12Texture2D*) rhit;
	FD3D12Resource* res = t2d->GetResource();
	return RHIResourceTransitionFromCurrentD3D12(ctx, rhit,res, default_before, after);
}

D3D12_RESOURCE_STATES simul::RHIResourceTransitionTextureCubeFromCurrentD3D12(IRHICommandContext* ctx, FRHITexture* rhit, D3D12_RESOURCE_STATES default_before, D3D12_RESOURCE_STATES after)
{
	auto *tc= (FD3D12TextureCube*)rhit;
	FD3D12Resource *res = tc->GetResource();
	return RHIResourceTransitionFromCurrentD3D12(ctx, rhit, res, default_before, after);
}

void simul::RHIResourceTransitionD3D12(IRHICommandContext* ctx, FRHITexture* texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	if (!texture)
		return;
	FD3D12CommandContext* cmdContext12	= (FD3D12CommandContext*)ctx;
	FD3D12Texture2D *t2d = static_cast<FD3D12Texture2D*>(texture);
	FD3D12Resource *res = t2d->GetResource();
	if (before == after)
		return;
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition((ID3D12Resource*)texture->GetNativeResource(), before, after);
	cmdContext12->CommandListHandle->ResourceBarrier(1, &barrier);
}

#undef Initialize
#undef CheckResourceState	
#undef ConditionalInitalize
#undef AreAllSubresourcesSame			
#undef CheckResourceStateInitalized	
#undef GetSubresourceState				
#undef SetResourceState				
#undef SetResourceState				
#undef SetSubresourceState			

#ifdef UpdateResidency
	#undef UpdateResidency					
#endif
#undef GetResourceState				
#undef AddTransitionBarrier			

#undef FindShaderResourceDXGIFormat
#undef FindUnorderedAccessDXGIFormat
#undef FindDepthStencilDXGIFormat
#undef HasStencilBits
#undef GetRenderTargetFormat
#undef FD3DGPUProfiler
#undef FVector4VertexDeclaration

#endif