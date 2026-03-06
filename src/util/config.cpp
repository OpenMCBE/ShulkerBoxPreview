#include "config.h"

#include <cstdio>
#include <SimpleIni.h>

int spPreviewKey = 'H';
bool spChangingPreviewKey = false;
float spTintIntensity = 1.0f;

int SP_clampPercent(int value) {
    if (value < 0)   return 0;
    if (value > 200) return 200;
    return value;
}

std::string SP_getConfigPath() {
    return "../shulkerpreview.conf";
}

void SP_loadConfig() {
    CSimpleIniA ini;
    ini.SetUnicode(false);
    ini.LoadFile(SP_getConfigPath().c_str());

    int key = static_cast<int>(ini.GetLongValue("", "previewKey", 'H'));
    spPreviewKey = (key <= 0) ? 'H' : key;

    int intensityPercent = SP_clampPercent(static_cast<int>(ini.GetLongValue("", "tintIntensity", 100)));
    spTintIntensity = static_cast<float>(intensityPercent) / 100.0f;
}

void SP_saveConfig() {
    CSimpleIniA ini;
    ini.SetUnicode(false);
    ini.LoadFile(SP_getConfigPath().c_str());

    ini.SetLongValue("", "previewKey", spPreviewKey);
    ini.SetLongValue("", "tintIntensity", SP_clampPercent(static_cast<int>(spTintIntensity * 100.0f + 0.5f)));

    ini.SaveFile(SP_getConfigPath().c_str());
}
