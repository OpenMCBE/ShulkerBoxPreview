#pragma once
#include <cstddef>
#include <cstdint>

class ItemStackBase;

struct ItemStackBaseOpaque {
    alignas(16) std::byte data[0x800];
};

static inline ItemStackBase* asISB(ItemStackBaseOpaque& o) {
    return reinterpret_cast<ItemStackBase*>(o.data);
}

#define SHULKER_CACHE_SIZE 16
#define SHULKER_SLOT_COUNT 27

struct ShulkerSlotCache {
    ItemStackBaseOpaque isb;
    uint8_t count;
    bool valid;
    bool enchanted;
};

extern ShulkerSlotCache ShulkerCache[SHULKER_CACHE_SIZE][SHULKER_SLOT_COUNT];
