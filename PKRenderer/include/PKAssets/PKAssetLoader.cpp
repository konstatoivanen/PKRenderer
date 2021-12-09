#pragma once
#include "PKAssetLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace PK::Assets
{
    void Decompress(PKAsset* asset)
    {
        auto base = reinterpret_cast<char*>(asset->rawData);
        auto head = base + sizeof(PKAssetHeader);

        uint_t osize = *reinterpret_cast<uint_t*>(head);
        head += sizeof(uint_t);

        uint_t binOffs = *reinterpret_cast<uint_t*>(head);
        head += sizeof(uint_t);

        size_t binSize = *reinterpret_cast<size_t*>(head);
        head += sizeof(size_t);

        auto decomp = reinterpret_cast<char*>(calloc(osize, sizeof(char)));
        memcpy(decomp, base, sizeof(PKAssetHeader));

        auto dh = 0ull;
        auto dl = binSize;
        auto rn = reinterpret_cast<PKEncNode*>(head);
        auto cn = rn;
        auto comp = base + binOffs;
        head = decomp + sizeof(PKAssetHeader);

        for (auto i = 0ull; i < dl; ++i)
        {
            if (cn->isLeaf)
            {
                head[dh++] = cn->value;
                cn = rn;
            }

            auto ch = i / 8;
            auto co = i - (ch * 8);
            auto lr = comp[ch] & (1 << co);

            if (lr == 0)
            {
                cn = cn->left.Get(base);
            }
            else
            {
                cn = cn->right.Get(base);
            }
        }

        free(asset->rawData);
        asset->rawData = decomp;
        asset->header = reinterpret_cast<PKAssetHeader*>(decomp);
        asset->header->isCompressed = false;
    }

    int OpenAsset(const char* filepath, PKAsset* asset)
    {
        FILE* file = nullptr;

#if _WIN32
        auto error = fopen_s(&file, filepath, "rb");

        if (error != 0)
        {
            return -1;
        }
#else
        file = fopen(filepath, "rb");
#endif

        if (file == nullptr)
        {
            return -1;
        }

        fseek(file, 0, SEEK_END);
        auto size = ftell(file);
        rewind(file);

        if (size == 0)
        {
            fclose(file);
            return -1;
        }

        auto buffer = malloc(size);

        if (buffer == nullptr)
        {
            fclose(file);
            return -1;
        }

        asset->rawData = buffer;
        fread(buffer, sizeof(char), size, file);
        fclose(file);

        asset->header = reinterpret_cast<PKAssetHeader*>(asset->rawData);

        if (asset->header->isCompressed)
        {
            Decompress(asset);
        }

        return 0;
    }

    void CloseAsset(PKAsset* asset)
    {
        if (asset->rawData == nullptr)
        {
            return;
        }

        asset->header = nullptr;
        free(asset->rawData);
    }

    Shader::PKShader* ReadAsShader(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Shader)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<Shader::PKShader*>(assetPtr);
    }

    Mesh::PKMesh* ReadAsMesh(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Mesh)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<Mesh::PKMesh*>(assetPtr);
    }
}