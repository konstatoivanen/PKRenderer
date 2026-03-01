#pragma once
#include <stdint.h>

namespace PKAssets
{
    constexpr static const uint32_t PK_ASSET_ENCODE_CODE_COUNT = 256u;
    constexpr static const uint32_t PK_ASSET_ENCODE_CODE_LENGTH = 11u;
    constexpr static const uint32_t PK_ASSET_ENCODE_CODE_BIT_COUNT = 4u;

    struct PKEncodeTable
    {
        uint8_t lengths[PK_ASSET_ENCODE_CODE_COUNT]{};
        uint16_t codes[PK_ASSET_ENCODE_CODE_COUNT]{};
        size_t size;
    };

    void EncodeBuffer(const void* in_data, size_t in_data_size, PKEncodeTable* table, uint8_t* out_data);
    void EncodeBuffer(const void* in_data, size_t in_data_size, uint8_t** out_data, size_t* out_data_size);
    void DecodeBuffer(const void* in_data, uint8_t* write_data, size_t write_size);
}