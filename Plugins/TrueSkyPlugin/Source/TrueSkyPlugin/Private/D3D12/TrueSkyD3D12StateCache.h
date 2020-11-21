

#if PLATFORM_WINDOWS && UE_EDITOR
void FD3D12StateCacheBase::DirtyState()
{
	// Mark bits dirty so the next call to ApplyState will set all this state again
	PipelineState.Common.bNeedSetPSO = true;
	PipelineState.Compute.bNeedSetRootSignature = true;
	PipelineState.Graphics.bNeedSetRootSignature = true;
	bNeedSetVB = true;
	bNeedSetIB = true;
	bNeedSetSOs = true;
	bNeedSetRTs = true;
	bNeedSetViewports = true;
	bNeedSetScissorRects = true;
	bNeedSetPrimitiveTopology = true;
	bNeedSetBlendFactor = true;
	bNeedSetStencilRef = true;
	bNeedSetDepthBounds = GSupportsDepthBoundsTest;
	PipelineState.Common.SRVCache.DirtyAll();
	PipelineState.Common.UAVCache.DirtyAll();
	PipelineState.Common.CBVCache.DirtyAll();
	PipelineState.Common.SamplerCache.DirtyAll();
}
#endif