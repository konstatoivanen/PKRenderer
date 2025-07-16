#pragma once
#include "PKAsset.h"

namespace PKAssets
{
    int OpenAsset(const char* filepath, PKAsset* asset);
    void CloseAsset(PKAsset* asset);

    int OpenAssetStream(const char* filepath, PKAssetStream* stream);
    void CloseAssetStream(PKAssetStream* stream);

    PKShader* ReadAsShader(PKAsset* asset);
    PKMesh* ReadAsMesh(PKAsset* asset);
    PKFont* ReadAsFont(PKAsset* asset);
    PKTexture* ReadAsTexture(PKAsset* asset);

    int StreamData(PKAssetStream* stream, void* dst, size_t offset, size_t size);
    int StreamAsShader(PKAssetStream* stream, PKShader* outvalue);
    int StreamAsMesh(PKAssetStream* stream, PKMesh* outvalue);
    int StreamAsFont(PKAssetStream* stream, PKFont* outvalue);
    int StreamAsTexture(PKAssetStream* stream, PKTexture* outvalue);

    PKAssetMeta OpenAssetMeta(const char* filepath);
    void CloseAssetMeta(PKAssetMeta* meta);

    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, uint32_t* outValue);
    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, bool* outValue);
}
