#include "nineslicehelper.h"

#include <algorithm>

NinesliceHelper::NinesliceHelper(float textureWidth, float textureHeight, float sliceWidth, float sliceHeight)
    : mTextureWidth(textureWidth),
      mTextureHeight(textureHeight),
      mSliceWidth(sliceWidth),
      mSliceHeight(sliceHeight) {
    const float middleWidth  = std::max(0.0f, textureWidth  - sliceWidth  * 2.0f);
    const float middleHeight = std::max(0.0f, textureHeight - sliceHeight * 2.0f);

    mSlices[0] = buildSlice(0.0f,                       0.0f,                       sliceWidth,   sliceHeight);
    mSlices[1] = buildSlice(sliceWidth,                 0.0f,                       middleWidth,  sliceHeight);
    mSlices[2] = buildSlice(textureWidth - sliceWidth,  0.0f,                       sliceWidth,   sliceHeight);
    mSlices[3] = buildSlice(0.0f,                       sliceHeight,                sliceWidth,   middleHeight);
    mSlices[4] = buildSlice(sliceWidth,                 sliceHeight,                middleWidth,  middleHeight);
    mSlices[5] = buildSlice(textureWidth - sliceWidth,  sliceHeight,                sliceWidth,   middleHeight);
    mSlices[6] = buildSlice(0.0f,                       textureHeight - sliceHeight,sliceWidth,   sliceHeight);
    mSlices[7] = buildSlice(sliceWidth,                 textureHeight - sliceHeight,middleWidth,  sliceHeight);
    mSlices[8] = buildSlice(textureWidth - sliceWidth,  textureHeight - sliceHeight,sliceWidth,   sliceHeight);
}

void NinesliceHelper::draw(
    MinecraftUIRenderContext& ctx,
    const RectangleArea& rect,
    const mce::ClientTexture& texture
) const {
    const float x            = rect._x0;
    const float y            = rect._y0;
    const float width        = std::max(0.0f, rect._x1 - rect._x0);
    const float height       = std::max(0.0f, rect._y1 - rect._y0);
    const float middleWidth  = std::max(0.0f, width  - 2.0f * mSliceWidth);
    const float middleHeight = std::max(0.0f, height - 2.0f * mSliceHeight);

    const glm::vec2 positions[9] = {
        {x,                      y},
        {x + mSliceWidth,        y},
        {x + width - mSliceWidth,y},
        {x,                      y + mSliceHeight},
        {x + mSliceWidth,        y + mSliceHeight},
        {x + width - mSliceWidth,y + mSliceHeight},
        {x,                      y + height - mSliceHeight},
        {x + mSliceWidth,        y + height - mSliceHeight},
        {x + width - mSliceWidth,y + height - mSliceHeight}
    };

    const glm::vec2 sizes[9] = {
        {mSliceWidth,  mSliceHeight},
        {middleWidth,  mSliceHeight},
        {mSliceWidth,  mSliceHeight},
        {mSliceWidth,  middleHeight},
        {middleWidth,  middleHeight},
        {mSliceWidth,  middleHeight},
        {mSliceWidth,  mSliceHeight},
        {middleWidth,  mSliceHeight},
        {mSliceWidth,  mSliceHeight}
    };

    for (int i = 0; i < 9; ++i) {
        if (sizes[i].x <= 0.0f || sizes[i].y <= 0.0f)
            continue;
        ctx.drawImage(texture, positions[i], sizes[i], mSlices[i].uvPos, mSlices[i].uvSize, false);
    }
}

NinesliceHelper::SliceUV NinesliceHelper::buildSlice(float x, float y, float width, float height) const {
    SliceUV slice{};
    slice.uvPos  = {x / mTextureWidth,  y / mTextureHeight};
    slice.uvSize = {width / mTextureWidth, height / mTextureHeight};
    return slice;
}
