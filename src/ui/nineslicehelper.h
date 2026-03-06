#pragma once

#include <array>

#include "minecraftuirendercontext.h"

class NinesliceHelper {
public:
    NinesliceHelper(float textureWidth, float textureHeight, float sliceWidth, float sliceHeight);

    void draw(
        MinecraftUIRenderContext& ctx,
        const RectangleArea& rect,
        const mce::ClientTexture& texture
    ) const;

private:
    struct SliceUV {
        glm::vec2 uvPos;
        glm::vec2 uvSize;
    };

    SliceUV buildSlice(float x, float y, float width, float height) const;

    float mTextureWidth;
    float mTextureHeight;
    float mSliceWidth;
    float mSliceHeight;
    std::array<SliceUV, 9> mSlices;
};
