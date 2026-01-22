#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include "PKAssetLoader.h"
#include "PKAssetEncoding.h"

namespace PKAssets
{
    FILE* OpenFile(const char* filepath, const char* option, size_t* size)
    {
        if (filepath == nullptr || option == nullptr)
        {
            return nullptr;
        }

        FILE* file = fopen(filepath, option);

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

        uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(header.uncompressedSize));

        if (header.isCompressed)
        {
            // Write uncompressed data to the end of the asset buffer & decode in place.
            uint8_t* compressed = buffer + (header.uncompressedSize - (size - headerSize));
            fread(compressed, sizeof(uint8_t), size - headerSize, file);
            DecodeBuffer(compressed, buffer + headerSize, header.uncompressedSize - headerSize);
            header.isCompressed = false;
        }
        else
        {
            fread(buffer + headerSize, sizeof(uint8_t), header.uncompressedSize - headerSize, file);
        }

        fclose(file);
            
        asset->rawData = buffer;
        *(asset->header) = header;

        return 0;
    }

    void CloseAsset(PKAsset* asset)
    {
        if (asset->rawData != nullptr)
        {
            free(asset->rawData);
        }
    }


    int OpenAssetStream(const char* filepath, PKAssetStream* stream)
    {
        size_t size = 0ull;
        FILE* file = OpenFile(filepath, "rb", &size);
        constexpr auto headerSize = sizeof(PKAssetHeader);

        if (file == nullptr || size < headerSize)
        {
            return -1;
        }

        fread(&stream->header, headerSize, 1, file);

        if (stream->header.magicNumber != PK_ASSET_MAGIC_NUMBER)
        {
            fclose(file);
            return -1;
        }

        // Cant stream compressed files. yet.
        if (stream->header.isCompressed)
        {
            fclose(file);
            return -1;
        }

        stream->stream = file;
        return 0;
    }

    void CloseAssetStream(PKAssetStream* stream)
    {
        if (stream && stream->stream)
        {
            fclose(reinterpret_cast<FILE*>(stream->stream));
        }
    }


    PKShader* ReadAsShader(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Shader)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<PKShader*>(assetPtr);
    }

    PKMesh* ReadAsMesh(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Mesh)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<PKMesh*>(assetPtr);
    }

    PKFont* ReadAsFont(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Font)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<PKFont*>(assetPtr);
    }

    PKTexture* ReadAsTexture(PKAsset* asset)
    {
        if (asset->header == nullptr || asset->header->type != PKAssetType::Texture)
        {
            return nullptr;
        }

        auto assetPtr = reinterpret_cast<char*>(asset->rawData) + sizeof(PKAssetHeader);
        return reinterpret_cast<PKTexture*>(assetPtr);
    }


    int StreamData(PKAssetStream* stream, void* dst, size_t offset, size_t size)
    {
        auto seekret = fseek(reinterpret_cast<FILE*>(stream->stream), offset, SEEK_SET);
        auto readret = fread(dst, size, 1u, reinterpret_cast<FILE*>(stream->stream));
        return seekret == 0 && readret != 0 ? 0 : -1;
    }

    int StreamAsShader(PKAssetStream* stream, PKShader* outvalue)
    {
        if (stream->stream == nullptr || stream->header.type != PKAssetType::Shader)
        {
            return -1;
        }

        return StreamData(stream, outvalue, sizeof(PKAssetHeader), sizeof(PKShader));
    }

    int StreamAsMesh(PKAssetStream* stream, PKMesh* outvalue)
    {
        if (stream->stream == nullptr || stream->header.type != PKAssetType::Mesh)
        {
            return -1;
        }

        return StreamData(stream, outvalue, sizeof(PKAssetHeader), sizeof(PKMesh));
    }

    int StreamAsFont(PKAssetStream* stream, PKFont* outvalue)
    {
        if (stream->stream == nullptr || stream->header.type != PKAssetType::Font)
        {
            return -1;
        }

        return StreamData(stream, outvalue, sizeof(PKAssetHeader), sizeof(PKFont));
    }

    int StreamAsTexture(PKAssetStream* stream, PKTexture* outvalue)
    {
        if (stream->stream == nullptr || stream->header.type != PKAssetType::Texture)
        {
            return -1;
        }

        return StreamData(stream, outvalue, sizeof(PKAssetHeader), sizeof(PKTexture));
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

        if (meta.optionNames == nullptr || meta.optionValues == nullptr)
        {
            fclose(file);
            free(buffer);
            free(meta.optionNames);
            free(meta.optionValues);
            return {};
        }

        while (head != nullptr && (size_t)head < (size_t)(buffer + size))
        {
            if (*head == '\n')
            {
                head++;
                continue;
            }

            auto comma = strchr(head, ':');

            if (comma == nullptr || (size_t)(comma - head) <= 1)
            {
                break;
            }

            strncpy(meta.optionNames + PK_ASSET_NAME_MAX_LENGTH * lineIndex, head, (size_t)(comma - head));

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
