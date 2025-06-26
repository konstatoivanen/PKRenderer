#pragma once
#include "PKAsset.h"

namespace PKAssets
{
    int OpenAsset(const char* filepath, PKAsset* asset);
    void CloseAsset(PKAsset* asset);

    PKShader* ReadAsShader(PKAsset* asset);
    PKMesh* ReadAsMesh(PKAsset* asset);
    PKFont* ReadAsFont(PKAsset* asset);
    PKTexture* ReadAsTexture(PKAsset* asset);

    PKAssetMeta OpenAssetMeta(const char* filepath);
    void CloseAssetMeta(PKAssetMeta* meta);

    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, uint32_t* outValue);
    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, bool* outValue);
}