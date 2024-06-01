#pragma once
#include "PKAsset.h"

namespace PK::Assets
{
    int OpenAsset(const char* filepath, PKAsset* asset);
    void CloseAsset(PKAsset* asset);

    Shader::PKShader* ReadAsShader(PKAsset* asset);
    Mesh::PKMesh* ReadAsMesh(PKAsset* asset);

    PKAssetMeta OpenAssetMeta(const char* filepath);
    void CloseAssetMeta(PKAssetMeta* meta);

    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, uint32_t* outValue);
    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, bool* outValue);
}