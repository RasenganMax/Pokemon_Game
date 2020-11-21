
// Start Duplicated from D3D12CommandList.cpp -------------------
#if WITH_EDITOR
void FD3D12CommandListHandle::AddTransitionBarrier(FD3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
{
	check(CommandListData);
	if (Before != After)
	{
		int32 NumAdded = CommandListData->ResourceBarrierBatcher.AddTransition(pResource->GetResource(), Before, After, Subresource);
		CommandListData->CurrentOwningContext->numBarriers += NumAdded;

		pResource->UpdateResidency(*this);
	}
	else
	{
		ensureMsgf(0, TEXT("AddTransitionBarrier: Before == After (%d)"), (uint32)Before);
	}
}
#endif

void inline FD3D12CommandListHandle::FD3D12CommandListData::FCommandListResourceState::ConditionalInitalize(FD3D12Resource* pResource, CResourceState& ResourceState)
{
	// If there is no entry, all subresources should be in the resource's TBD state.
	// This means we need to have pending resource barrier(s).
	if (!ResourceState.CheckResourceStateInitalized())
	{
		ResourceState.Initialize(pResource->GetSubresourceCount());
		check(ResourceState.CheckResourceState(D3D12_RESOURCE_STATE_TBD));
	}

	check(ResourceState.CheckResourceStateInitalized());
}

CResourceState& FD3D12CommandListHandle::FD3D12CommandListData::FCommandListResourceState::GetResourceState(FD3D12Resource* pResource)
{
	// Only certain resources should use this
	check(pResource->RequiresResourceStateTracking());

	CResourceState& ResourceState = ResourceStates.FindOrAdd(pResource);
	ConditionalInitalize(pResource, ResourceState);
	return ResourceState;
}
// End Duplicated from D3D12CommandList.cpp -------------------
