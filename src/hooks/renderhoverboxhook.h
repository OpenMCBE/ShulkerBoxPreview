#pragma once
#include "ui/hoverrenderer.h"
#include "shulkerenderer/shulkerrenderer.h"
#include "hooks/minecraftuirendercontexthook.h"
#include "util/shulkerglobals.h"

using RenderHoverBoxFn = void (*)(void*, MinecraftUIRenderContext*, void*, void*, float);

inline RenderHoverBoxFn HoverRenderer_renderHoverBox_orig = nullptr;

inline void HoverRenderer_renderHoverBox_hook(
    void* selfPtr,
    MinecraftUIRenderContext* ctx,
    void* client,
    void* aabb,
    float someFloat)
{
    HoverRenderer_renderHoverBox_orig(selfPtr, ctx, client, aabb, someFloat);

    if (!ctx || !g_hasShulkerData || g_shulkerCacheIndex < 0)
        return;

    ActiveUIContext = ctx;

    HoverRenderer* self = reinterpret_cast<HoverRenderer*>(selfPtr);
    float px = self ? self->mCursorX + self->mOffsetX : 0.0f;
    float py = self ? self->mCursorY + self->mOffsetY + self->mBoxHeight : 0.0f;

    ShulkerRenderer::render(ctx, px, py, g_shulkerCacheIndex, g_shulkerColorCode);
}
