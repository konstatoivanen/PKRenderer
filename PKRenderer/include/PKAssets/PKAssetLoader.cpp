#include "PKAssetLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>

namespace PK::Assets
{
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

        struct stat filestat;
        int fileNumber = _fileno(file);
        if (fstat(fileNumber, &filestat) != 0)
        {
            fclose(file);
            return nullptr;
        }

        *size = filestat.st_size;

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
        constexpr auto headerSize = sizeof(PKAssetHeader);

        if (file == nullptr || size < headerSize)
        {
            return -1;
        }

        PKAssetHeader header;
        fread(&header, headerSize, 1, file);

        if (header.magicNumber != PK_ASSET_MAGIC_NUMBER)
        {
            fclose(file);
            return -1;
        }

        char* buffer = nullptr;

        if (!header.isCompressed)
        {
            buffer = reinterpret_cast<char*>(malloc(size));
            fread(buffer + headerSize, sizeof(char), size - headerSize, file);
        }
        else
        {
            buffer = reinterpret_cast<char*>(calloc(header.uncompressedSize, sizeof(char)));

            auto nodesSize = header.compressedOffset - sizeof(PKAssetHeader);
            auto nodeCount = nodesSize / sizeof(PKEncNode);
            auto nodes = reinterpret_cast<PKEncNode*>(alloca(nodesSize));

            fread(nodes, sizeof(PKEncNode), nodeCount, file);

            auto root = nodes;
            auto curr = nodes;
            auto head = buffer + sizeof(PKAssetHeader);

            uint8_t block[64];
            constexpr auto blockSize = sizeof(block) * 8ull;
            auto blockCount = (header.compressedBitCount + blockSize - 1ull) / blockSize;

            for (auto blockIdx = 0ull; blockIdx < blockCount; ++blockIdx)
            {
                auto blockBitCount = header.compressedBitCount - blockIdx * blockSize;
                blockBitCount = blockBitCount > blockSize ? blockSize : blockBitCount;
                fread(block, sizeof(uint8_t), (blockBitCount + 7ull) >> 3ull, file);

                for (auto i = 0ull; i < blockBitCount; ++i)
                {
                    if (curr->isLeaf)
                    {
                        *head = curr->value;
                        ++head;
                        curr = root;
                    }

                    curr = root + ((block[i >> 3ull] & (1ull << (i & 0x7ull))) ? curr->right : curr->left);
                }
            }

            header.isCompressed = false;
        }

        fclose(file);
            
        asset->rawData = buffer;
        *(asset->header) = header;

        return 0;
    }

    void CloseAsset(PKAsset* asset)
    {
        if (asset->rawData == nullptr)
        {
            return;
        }

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
