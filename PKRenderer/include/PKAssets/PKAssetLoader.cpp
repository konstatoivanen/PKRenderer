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

        uint32_t osize = *reinterpret_cast<uint32_t*>(head);
        head += sizeof(uint32_t);

        uint32_t binOffs = *reinterpret_cast<uint32_t*>(head);
        head += sizeof(uint32_t);

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

    FILE* OpenFile(const char* filepath, const char* option, size_t* size)
    {
        FILE* file = nullptr;

#if _WIN32
        auto error = fopen_s(&file, filepath, option);

        if (error != 0)
        {
            return nullptr;
        }
#else
        file = fopen(filepath, option);
#endif

        if (file == nullptr)
        {
            return nullptr;
        }

        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        rewind(file);

        if (*size == 0)
        {
            fclose(file);
            return nullptr;
        }

        return file;
    }


    int OpenAsset(const char* filepath, PKAsset* asset)
    {
        size_t size = 0ull;
        FILE* file = OpenFile(filepath, "rb", &size);

        if (file == nullptr)
        {
            return -1;
        }

        uint64_t magicNumber = 0ull;
        fread(&magicNumber, sizeof(decltype(magicNumber)), 1, file);

        if (magicNumber != PK_ASSET_MAGIC_NUMBER)
        {
            fclose(file);
            return -1;
        }

        rewind(file);

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


    PKAssetMeta OpenAssetMeta(const char* filepath)
    {
        size_t size = 0ull;
        FILE* file = OpenFile(filepath, "r", &size);

        if (file == nullptr)
        {
            return {};
        }

        char* buffer = (char*)calloc(size + 1, sizeof(char));

        if (buffer == nullptr)
        {
            fclose(file);
            return {};
        }

        buffer[size] = '\0';

        fread(buffer, sizeof(char), size, file);

        auto lineCount = 0ull;
        auto lineIndex = 0ull;
        auto head = buffer;

        for (size_t i = 0ull; i < size; ++i)
        {
            if (buffer[i] == ':')
            {
                ++lineCount;
            }
        }

        if (lineCount == 0ull)
        {
            fclose(file);
            free(buffer);
            return {};
        }

        PKAssetMeta meta{};
        meta.optionNames = (char*)calloc(PK_ASSET_NAME_MAX_LENGTH * lineCount, sizeof(char));
        meta.optionValues = (uint32_t*)calloc(lineCount, sizeof(uint32_t));

        if (meta.optionNames == nullptr)
        {
            fclose(file);
            free(buffer);
            return {};
        }

        if (meta.optionValues == nullptr)
        {
            fclose(file);
            free(buffer);
            return {};
        }

        while (head != nullptr && (size_t)head < (size_t)(buffer + size))
        {
            auto comma = strchr(head, ':');

            if (comma == nullptr || (size_t)(comma - head) <= 1)
            {
                break;
            }

#if _WIN32
            auto error = strncpy_s(meta.optionNames + PK_ASSET_NAME_MAX_LENGTH * lineIndex, PK_ASSET_NAME_MAX_LENGTH, head, (size_t)(comma - head));

            if (error != 0)
            {
                break;
            }
#else
            strncpy(meta->optionNames + PK_ASSET_NAME_MAX_LENGTH * lineIndex, head, (size_t)(comma - head));
#endif

            meta.optionValues[lineIndex++] = (uint32_t)strtoull(comma + 1, &head, 10);
        }

        meta.optionCount = (uint32_t)lineIndex;

        fclose(file);
        free(buffer);

        return meta;
    }

    void CloseAssetMeta(PKAssetMeta* meta)
    {
        if (meta->optionNames != nullptr)
        {
            free(meta->optionNames);
            meta->optionNames = nullptr;
        }

        if (meta->optionValues != nullptr)
        {
            free(meta->optionValues);
            meta->optionValues = nullptr;
        }
    }

    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, uint32_t* outValue)
    {
        if (meta.optionCount == 0 || meta.optionNames == nullptr || meta.optionValues == nullptr)
        {
            return false;
        }

        auto nameLength = strlen(name);
        nameLength = nameLength < PK_ASSET_NAME_MAX_LENGTH ? nameLength : PK_ASSET_NAME_MAX_LENGTH;

        for (uint32_t i = 0u; i < meta.optionCount; ++i)
        {
            const char* optionName = meta.optionNames + PK_ASSET_NAME_MAX_LENGTH * i;

            if (strncmp(optionName, name, nameLength) == 0)
            {
                *outValue = (uint32_t)meta.optionValues[i];
                return true;
            }
        }

        return false;
    }

    bool GetAssetMetaOption(const PKAssetMeta& meta, const char* name, bool* outValue)
    {
        uint32_t metaValue = 0u;
        auto result = GetAssetMetaOption(meta, name, &metaValue);
        *outValue = metaValue != 0;
        return result;
    }
}