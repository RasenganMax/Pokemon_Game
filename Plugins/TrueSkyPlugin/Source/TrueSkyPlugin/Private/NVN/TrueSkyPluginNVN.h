/*
    TrueSkyPluginNVN.h
*/

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_SWITCH

namespace nvn
{
    class Texture;
    class TextureView;
}
class FRHITexture2D;

namespace simul
{
    struct RenderTargetHolder;

    RenderTargetHolder* GetRenderTargetHolderNVN(FRHITexture2D* texture);
    void RestoreStateNVN();
}

#endif	//	PLATFORM_SWITCH.