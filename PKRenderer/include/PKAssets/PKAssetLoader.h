#pragma once
#include "PKAsset.h"

namespace PK::Assets
{
    int OpenAsset(const char* filepath, PKAsset* asset);
    void CloseAsset(PKAsset* asset);

    Shader::PKShader* ReadAsShader(PKAsset* asset);
    Mesh::PKMesh* ReadAsMesh(PKAsset* asset);
}