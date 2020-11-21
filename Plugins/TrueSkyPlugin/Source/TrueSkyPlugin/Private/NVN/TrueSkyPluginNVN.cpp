#include "TrueSkyPluginNVN.h"

#if PLATFORM_SWITCH

#include "NVNRHI.h"
#include <NVN/nvn_Cpp.h>

struct simul::RenderTargetHolder
{
    nvn::Texture*       Texture;
    nvn::TextureView    View;
};

nvn::TextureView rtView             = {};
simul::RenderTargetHolder holder    = {};

simul::RenderTargetHolder* simul::GetRenderTargetHolderNVN(FRHITexture2D* texture)
{
    rtView.SetDefaults();
    rtView.SetLevels(0, 1);
    rtView.SetLayers(0, 1);

    FNVNSurface* surface    = GetNVNSurfaceFromRHITexture(texture);
    holder.Texture          = surface->GetTexture();
    holder.View             = rtView;;
    return &holder;
}

#endif //PLATFORM_SWITCH