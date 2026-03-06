#pragma once

#include <cstddef>
#include <cstdint>

using BaseActorRenderContext_ctor_t = void (*)(void* barc, void* screenContext, void* clientInstance, void* minecraftGame);
extern BaseActorRenderContext_ctor_t BaseActorRenderContext_ctor;

using ItemRenderer_renderGuiItemNew_t = std::uint64_t (*)(
    void* rendererCtx,
    void* barc,
    void* itemStack,
    unsigned int aux,
    unsigned char layer,
    std::uint64_t flags,
    float posX,
    float posY,
    float width,
    float height,
    float scale);
extern ItemRenderer_renderGuiItemNew_t ItemRenderer_renderGuiItemNew;

inline constexpr std::size_t kBarcStorageSize = 0x400;
inline constexpr std::size_t kBarcItemRendererOffset = 0x58;
inline constexpr std::size_t kClientMinecraftGameOffset = 0xA8;
inline constexpr std::size_t kClientGetMinecraftGameVfIndex = 83;
