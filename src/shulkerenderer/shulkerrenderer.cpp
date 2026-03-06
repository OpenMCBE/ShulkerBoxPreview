#include "shulkerrenderer.h"

#include <cstdio>

namespace {
constexpr int   kColumns      = 9;
constexpr int   kRows         = 3;
constexpr float kSlotStride   = 18.0f;
constexpr float kSlotDrawSize = 17.5f;
constexpr float kPanelPadding = 6.0f;
constexpr float kItemDrawSize = 16.0f;
constexpr float kItemInset    = (kSlotStride - kItemDrawSize) * 0.5f;
constexpr float kCountTextH   = 6.0f;

const HashedString  kFlushMaterial("ui_flush");
const NinesliceHelper kPanelNineSlice(16.0f, 16.0f, 4.0f, 4.0f);
const mce::Color    kWhite{1.0f,1.0f,1.0f,1.0f};

mce::Color applyTint(const mce::Color& base) {
    auto c = [](float v){ return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
    return { c(base.r * 2.0f), c(base.g * 2.0f), c(base.b * 2.0f), base.a };
}

struct CachedUiTextures { bool loaded=false; mce::TexturePtr panel, slot; };

inline bool hasTex(const mce::TexturePtr& t) { return (bool)t.mClientTexture; }

template<typename Fn>
void forSlots(float ox, float oy, Fn&& fn) {
    for (int r = 0; r < kRows; r++)
        for (int c = 0; c < kColumns; c++)
            fn(r*kColumns+c, ox+c*kSlotStride, oy+r*kSlotStride);
}

ItemStackBase* getStack(ShulkerSlotCache& sc) {
    if (!sc.valid) return nullptr;
    ItemStackBase* s = asISB(sc.isb);
    return (s && s->mItem.get()) ? s : nullptr;
}

bool hasEnchanted(int idx) {
    for (int i = 0; i < SHULKER_SLOT_COUNT; i++)
        if (ShulkerCache[idx][i].valid && ShulkerCache[idx][i].enchanted) return true;
    return false;
}

void* getMCGame(void* client) {
    if (!client) return nullptr;
    auto** vt = *(void***)client;
    if (vt && vt[kClientGetMinecraftGameVfIndex]) {
        auto fn = (void*(*)(void*))vt[kClientGetMinecraftGameVfIndex];
        if (void* r = fn(client)) return r;
    }
    return *(void**)((char*)client + kClientMinecraftGameOffset);
}

void destroyBarc(void* barc) {
    if (!barc) return;
    auto** vt = *(void***)barc;
    if (vt && vt[0]) ((void(*)(void*))vt[0])(barc);
}

void drawDurBar(MinecraftUIRenderContext& ctx, ItemStackBase* s, float sx, float sy) {
    Item* item = s->mItem.get();
    if (!item) return;
    short maxDmg = item->getMaxDamage();
    if (maxDmg <= 0) return;
    short dmg = ItemStackBase_getDamageValue(s);
    if (dmg <= 0) return;
    float ratio = (float)(maxDmg - dmg) / (float)maxDmg;
    float bx = sx + 2.0f, by = sy + 13.0f;
    ctx.fillRectangle({bx, bx+13.0f, by, by+2.0f}, {0,0,0,1}, 1.0f);
    ctx.fillRectangle({bx, bx+13.0f*ratio, by, by+1.0f}, {1.0f-ratio, ratio, 0, 1}, 1.0f);
}

CachedUiTextures& getTextures(MinecraftUIRenderContext& ctx) {
    static CachedUiTextures t;
    if (!t.loaded) {
        t.panel = ctx.getTexture(ResourceLocation("textures/ui/dialog_background_opaque"), false);
        t.slot  = ctx.getTexture(ResourceLocation("textures/ui/item_cell"), false);
        t.loaded = true;
    }
    return t;
}

void drawPanel(MinecraftUIRenderContext& ctx, const CachedUiTextures& t, const RectangleArea& r) {
    if (hasTex(t.panel)) kPanelNineSlice.draw(ctx, r, t.panel.getClientTexture());
}

void drawSlot(MinecraftUIRenderContext& ctx, const CachedUiTextures& t, float sx, float sy) {
    if (!hasTex(t.slot)) return;
    ctx.drawImage(t.slot.getClientTexture(), {sx,sy}, {kSlotDrawSize,kSlotDrawSize}, {0,0}, {1,1}, false);
}

void drawIcons(MinecraftUIRenderContext& ctx, int idx, float ox, float oy) {
    if (!BaseActorRenderContext_ctor || !ItemRenderer_renderGuiItemNew) return;
    if (!ctx.mClient || !ctx.mScreenContext) return;

    void* game = getMCGame(ctx.mClient);
    if (!game) return;

    alignas(16) std::byte barcBuf[kBarcStorageSize]{};
    BaseActorRenderContext_ctor(barcBuf, ctx.mScreenContext, ctx.mClient, game);
    void* ir = *(void**)((std::byte*)barcBuf + kBarcItemRendererOffset);
    if (!ir) { destroyBarc(barcBuf); return; }

    forSlots(ox, oy, [&](int slot, float x, float y) {
        if (ItemStackBase* s = getStack(ShulkerCache[idx][slot]))
            ItemRenderer_renderGuiItemNew(ir, barcBuf, s, 0, 0, 0, x+kItemInset, y+kItemInset, 1,1,1);
    });

    if (hasEnchanted(idx)) {
        forSlots(ox, oy, [&](int slot, float x, float y) {
            ShulkerSlotCache& sc = ShulkerCache[idx][slot];
            if (sc.enchanted)
                if (ItemStackBase* s = getStack(sc))
                    ItemRenderer_renderGuiItemNew(ir, barcBuf, s, 0, 1, 0, x+kItemInset, y+kItemInset, 1,1,1);
        });
    }

    forSlots(ox, oy, [&](int slot, float x, float y) {
        if (ItemStackBase* s = getStack(ShulkerCache[idx][slot]))
            drawDurBar(ctx, s, x, y);
    });

    destroyBarc(barcBuf);
    ctx.flushImages(kWhite, 1.0f, kFlushMaterial);
}
} // namespace

void ShulkerRenderer::render(MinecraftUIRenderContext* ctx, float x, float y, int index, char colorCode) {
    if (!ctx || !ActiveUIContext) return;

    const mce::Color tint = applyTint(getShulkerTint(colorCode));
    const CachedUiTextures& tex = getTextures(*ctx);

    RectangleArea panel{x, x+kColumns*kSlotStride+kPanelPadding*2, y, y+kRows*kSlotStride+kPanelPadding*2};
    drawPanel(*ctx, tex, panel);
    ctx->flushImages(tint, 1.0f, kFlushMaterial);

    float ox = x + kPanelPadding, oy = y + kPanelPadding;

    forSlots(ox, oy, [&](int, float sx, float sy) {
        drawSlot(*ctx, tex, sx, sy);
    });
    ctx->flushImages(tint, 1.0f, kFlushMaterial);

    drawIcons(*ctx, index, ox, oy);

    if (Font* fnt = ActiveUIFont) {
        TextMeasureData measure{}; measure.fontSize = 1.0f;
        CaretMeasureData caret{};
        forSlots(ox, oy, [&](int slot, float sx, float sy) {
            ShulkerSlotCache& sc = ShulkerCache[index][slot];
            if (!sc.valid || sc.count <= 1) return;
            char txt[8];
            std::snprintf(txt, sizeof(txt), "%u", sc.count);
            float w = ActiveUIContext->getLineLength(*fnt, txt, 1.0f, false);
            float ax = sx+kSlotDrawSize-0.5f, ay = sy+kSlotDrawSize-1.5f;
            ActiveUIContext->drawText(*fnt, {ax-w, ax, ay-kCountTextH, ay}, txt, {1,1,1,1},
                ui::TextAlignment::Right, 1.0f, measure, caret);
        });
    }

    ctx->flushText(0.0f, std::nullopt);
}
