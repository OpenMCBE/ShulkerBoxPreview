#pragma once
#include <cstdint>
#include "ui/minecraftuirendercontext.h"

struct ShulkerIdMap {
    uint16_t id;
    char code;
};

static inline char getShulkerColorCodeFromItemId(uint16_t id)
{
    static constexpr ShulkerIdMap table[] = {
        {205,   '0'},
        {218,   '1'},
        {64923, '7'},
        {64922, 'f'},
        {64921, 'c'},
        {64920, '8'},
        {64919, '9'},
        {64918, 'g'},
        {64917, '3'},
        {64916, '2'},
        {64915, 'b'},
        {64914, 'e'},
        {64913, 'd'},
        {64912, '5'},
        {64911, 'a'},
        {64910, '6'},
        {64909, '4'}
    };

    for (const auto& e : table)
        if (e.id == id)
            return e.code;

    return '0';
}

static inline mce::Color getShulkerTint(char code)
{
    switch (code)
    {
        case '0': return {0.45f,0.42f,0.40f,1.0f};
        case '1': return {0.78f,0.76f,0.74f,1.0f};
        case '2': return {0.55f,0.53f,0.52f,1.0f};
        case '3': return {0.32f,0.31f,0.30f,1.0f};
        case '4': return {0.06f,0.05f,0.05f,1.0f};
        case '5': return {0.33f,0.25f,0.14f,1.0f};
        case '6': return {0.55f,0.20f,0.18f,1.0f};
        case '7': return {0.70f,0.42f,0.18f,1.0f};
        case '8': return {0.78f,0.72f,0.22f,1.0f};
        case '9': return {0.42f,0.65f,0.22f,1.0f};
        case 'a': return {0.18f,0.40f,0.18f,1.0f};
        case 'b': return {0.18f,0.55f,0.55f,1.0f};
        case 'c': return {0.28f,0.46f,0.62f,1.0f};
        case 'd': return {0.18f,0.24f,0.58f,1.0f};
        case 'e': return {0.45f,0.26f,0.60f,1.0f};
        case 'f': return {0.65f,0.34f,0.58f,1.0f};
        case 'g': return {0.78f,0.52f,0.62f,1.0f};
    }

    return {0.55f,0.55f,0.55f,1.0f};
}
