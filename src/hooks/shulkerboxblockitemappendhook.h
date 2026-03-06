#pragma once
#include "item/item.h"
#include "shulkerenderer/colors.h"
#include "item/itemstackbase.h"
#include "util/scache.h"
#include "util/shulkerglobals.h"
#include "nbt/nbt.h"
#include <string>

class ShulkerBoxBlockItem;

using Shulker_appendHover_t = void (*)(void*, ItemStackBase*, void*, std::string&, bool);

inline Shulker_appendHover_t ShulkerBoxBlockItem_appendFormattedHovertext_orig = nullptr;

inline uint16_t Item_getId_direct(Item* item) {
    return *reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(item) + 0x8A);
}

inline void ShulkerBoxBlockItem_appendFormattedHovertext_hook(
    ShulkerBoxBlockItem* self,
    ItemStackBase* stack,
    void* level,
    std::string& out,
    bool flag)
{
    if (ShulkerBoxBlockItem_appendFormattedHovertext_orig)
        ShulkerBoxBlockItem_appendFormattedHovertext_orig(self, stack, level, out, flag);

    if (auto pos = out.find('\n'); pos != std::string::npos)
        out.erase(pos);

    if (!stack || !stack->mUserData || !ItemStackBase_loadItem) {
        g_hasShulkerData = false;
        return;
    }

    int index = (reinterpret_cast<uintptr_t>(stack) >> 4) & (SHULKER_CACHE_SIZE - 1);

    for (int i = 0; i < SHULKER_SLOT_COUNT; ++i) {
        ShulkerCache[index][i].valid     = false;
        ShulkerCache[index][i].enchanted = false;
        ShulkerCache[index][i].count     = 0;
    }

    auto* list = reinterpret_cast<ListTagLayout*>(getListTag(stack->mUserData, "Items"));
    if (!list)
        list = reinterpret_cast<ListTagLayout*>(getListTag(stack->mUserData, "items"));

    if (list) {
        int size = listSize(list);
        for (int i = 0; i < size; ++i) {
            void* tag = listAt(list, i);
            if (!tag) continue;

            int slotValue = 0;
            if (!readIntTag(tag, "Slot", slotValue) && !readIntTag(tag, "slot", slotValue)) continue;

            int countValue = 1;
            if (!readIntTag(tag, "Count", countValue) && !readIntTag(tag, "count", countValue))
                countValue = 1;

            if (countValue < 0)   countValue = 0;
            if (countValue > 255) countValue = 255;

            uint8_t slot  = (uint8_t)slotValue;
            uint8_t count = (uint8_t)countValue;
            if (slot >= SHULKER_SLOT_COUNT) continue;

            ShulkerSlotCache& sc = ShulkerCache[index][slot];
            ItemStackBase_loadItem(asISB(sc.isb), tag);
            sc.count     = count;
            ItemStackBase* loaded = asISB(sc.isb);
            sc.enchanted = hasEnchantmentData(tag) ||
                           (loaded && loaded->mUserData && hasEnchantmentData(loaded->mUserData));
            sc.valid = true;
        }
    }

    char color = '0';
    if (Item* item = stack->mItem.get())
        color = getShulkerColorCodeFromItemId(Item_getId_direct(item));

    g_shulkerCacheIndex = index;
    g_shulkerColorCode  = color;
    g_hasShulkerData    = true;
}
