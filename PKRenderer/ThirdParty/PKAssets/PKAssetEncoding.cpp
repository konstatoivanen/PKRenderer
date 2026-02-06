#include <malloc.h>
#include <stdlib.h>
#include <memory>
#include "PKAssetEncoding.h"

namespace PKAssets
{
    struct EncodeNode
    {
        uint32_t symbol;
        uint32_t freq;
        EncodeNode* children[2];
    };

    static int32_t EncodeKeyCompare(const void* a, const void* b)
    {
        auto freqA = *reinterpret_cast<const uint64_t*>(a) & 0xFFFFFFFFull;
        auto freqB = *reinterpret_cast<const uint64_t*>(b) & 0xFFFFFFFFull;
        return freqA != freqB ? freqA < freqB ? -1 : 1 : 0;
    }
    
    static void BuildNodeTreeRecursive(EncodeNode* node, uint8_t* out_lengths)
    {
        if (node->symbol == UINT32_MAX)
        {
            BuildNodeTreeRecursive(node->children[0], out_lengths);
            BuildNodeTreeRecursive(node->children[1], out_lengths);
        }
        else
        {
            out_lengths[node->symbol]++;
        }
    }
    
    static void GenerateCodes(uint16_t* out_codes, const uint8_t* lengths)
    {
        uint32_t histogram[PK_ASSET_ENCODE_CODE_LENGTH + 1]{};			
        uint8_t list[(PK_ASSET_ENCODE_CODE_LENGTH + 1) * PK_ASSET_ENCODE_CODE_COUNT]{};	
    
        for (auto i = 0u; i < PK_ASSET_ENCODE_CODE_COUNT; ++i)
        {
            auto length = lengths[i];
            list[length * PK_ASSET_ENCODE_CODE_COUNT + histogram[length]++] = i;
        }
    
        auto next_code = 0u;
    
        for (auto length = 1u; length <= PK_ASSET_ENCODE_CODE_LENGTH; ++length)
        {
            auto count = histogram[length];
    
            for (auto i = 0u; i < count; ++i)
            {
                auto symbol = list[length * PK_ASSET_ENCODE_CODE_COUNT + i];
                auto bits = next_code++;
                bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
                bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
                bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
                bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
                bits = (bits << 16u) | (bits >> 16u);
                out_codes[symbol] = static_cast<uint16_t>(bits >> (32u - length));
            }
    
            next_code <<= 1u;
        }
    }
    
    void EncodeBuffer(const void* in_data, size_t in_data_size, PKEncodeTable* table, uint8_t* out_data)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(in_data);
    
        if (!out_data)
        {
            memset(table, 0, sizeof(PKEncodeTable));
            uint32_t frequencies[PK_ASSET_ENCODE_CODE_COUNT + 1u]{};
            uint64_t sorted[PK_ASSET_ENCODE_CODE_COUNT]{};
            EncodeNode nodes[PK_ASSET_ENCODE_CODE_COUNT * PK_ASSET_ENCODE_CODE_LENGTH * 2]{};
    
            auto sorted_count = 0u;
            auto curr_node_idx = 0u;
            auto prev_node_start_idx = 0u;
            auto prev_node_count = 0u;
    
            for (auto i = 0u; i < in_data_size; ++i)
            {
                frequencies[bytes[i]]++;
            }
            
            for (auto i = 0u; i < PK_ASSET_ENCODE_CODE_COUNT; ++i)
            {
                if (frequencies[i] > 0)
                {
                    sorted[sorted_count++] = ((uint64_t)frequencies[i]) | ((uint64_t)i << 32ull);
                }
            }
    
            qsort(sorted, sorted_count, sizeof(uint64_t), EncodeKeyCompare);
    
            if (sorted_count == 1u)
            {
                table->lengths[sorted[0] >> 32ull] = 1;
            }
            else
            {
                for (auto i = 1u; i <= PK_ASSET_ENCODE_CODE_LENGTH; ++i)
                {
                    auto node_count = 0u;
                    auto curr_node_count = sorted_count;
                    auto prev_node_idx = prev_node_start_idx;
                    prev_node_start_idx = curr_node_idx;
    
                    while (prev_node_count >= 2u || curr_node_count > 0u)
                    {
                        const auto freq = sorted[sorted_count - curr_node_count] & 0xFFFFFFFFull;
                        const auto symbol = sorted[sorted_count - curr_node_count] >> 32ull;
                        const auto children = nodes + prev_node_idx;
    
                        if (curr_node_count > 0u && (prev_node_count < 2u || freq <= children[0u].freq + children[1u].freq))
                        {
                            auto& node = nodes[curr_node_idx++];
                            node.freq = (uint32_t)freq;
                            node.symbol = (uint32_t)symbol;
                            node.children[0u] = nullptr;
                            node.children[1u] = nullptr;
                            curr_node_count--;
                        }
                        else
                        {
                            EncodeNode& Node = nodes[curr_node_idx++];
                            Node.freq = children[0].freq + children[1].freq;
                            Node.symbol = UINT32_MAX;
                            Node.children[0u] = children + 0u;
                            Node.children[1u] = children + 1u;
                            prev_node_count -= 2u;
                            prev_node_idx += 2u;
                        }
    
                        node_count++;
                    }
    
                    prev_node_count = node_count;
                }
    
                for (auto i = 0u; i < 2u * sorted_count - 2u; ++i)
                {
                    BuildNodeTreeRecursive(&nodes[prev_node_start_idx + i], table->lengths);
                }
            }
    
            GenerateCodes(table->codes, table->lengths);
    
            table->size += PK_ASSET_ENCODE_CODE_COUNT * PK_ASSET_ENCODE_CODE_BIT_COUNT;
    
            for (auto i = 0u; i < in_data_size; ++i)
            {
                table->size += table->lengths[bytes[i]];
            }
    
            // bits to bytes
            table->size = (table->size + 7ull) / 8ull;
        }
        else if (table)
        {
            auto stream_bitbuffer = 0ull;
            auto stream_bitcount = 0u;
            auto stream_bytecount = 0u;
    
            for (auto i = 0u; i < PK_ASSET_ENCODE_CODE_COUNT; ++i)
            {
                stream_bitbuffer |= (uint64_t)table->lengths[i] << stream_bitcount;
                stream_bitcount += PK_ASSET_ENCODE_CODE_BIT_COUNT;
                while (stream_bitcount >= 8u)
                {
                    out_data[stream_bytecount++] = stream_bitbuffer & 0xFFu;
                    stream_bitbuffer >>= 8u;
                    stream_bitcount -= 8u;
                }
            }
    
            for (auto i = 0u; i < in_data_size; ++i)
            {
                stream_bitbuffer |= (uint64_t)table->codes[bytes[i]] << stream_bitcount;
                stream_bitcount += table->lengths[bytes[i]];
                while (stream_bitcount >= 8u)
                {
                    out_data[stream_bytecount++] = stream_bitbuffer & 0xFFu;
                    stream_bitbuffer >>= 8u;
                    stream_bitcount -= 8u;
                }
            }
    
            if (stream_bitcount)
            {
                stream_bitcount += 8u - stream_bitcount;
                while (stream_bitcount >= 8u)
                {
                    out_data[stream_bytecount++] = stream_bitbuffer & 0xFFu;
                    stream_bitbuffer >>= 8u;
                    stream_bitcount -= 8u;
                }
            }
        }
    }
    
    void EncodeBuffer(const void* in_data, size_t in_data_size, uint8_t** out_data, size_t* out_data_size)
    {
        PKEncodeTable table{};
        EncodeBuffer(in_data, in_data_size, &table, nullptr);
        auto compressed_buff = reinterpret_cast<uint8_t*>(malloc(table.size));
        EncodeBuffer(in_data, in_data_size, &table, compressed_buff);
        *out_data = compressed_buff;
        *out_data_size = table.size;
    }
    
    void DecodeBuffer(const void* in_data, uint8_t* write_data, size_t write_size)
    {
        constexpr uint32_t table_size = 1u << PK_ASSET_ENCODE_CODE_LENGTH;
    
        uint8_t lengths[PK_ASSET_ENCODE_CODE_COUNT]{};
        uint16_t codes[PK_ASSET_ENCODE_CODE_COUNT]{};
        uint16_t table[table_size]{};
    
        auto stream_bytes = reinterpret_cast<const uint8_t*>(in_data);
        auto stream_bitbuffer = 0ull;
        auto stream_bitcount = 0u;
        auto stream_bytecount = 0u;

        for (auto i = 0u; i < PK_ASSET_ENCODE_CODE_COUNT; ++i)
        {
            stream_bitbuffer |= *(const uint64_t*)(stream_bytes + stream_bytecount) << stream_bitcount;
            stream_bytecount += (63u - stream_bitcount) >> 3u;
            stream_bitcount |= 56u;
            lengths[i] = stream_bitbuffer & ((1ull << PK_ASSET_ENCODE_CODE_BIT_COUNT) - 1ull); 
            stream_bitbuffer >>= PK_ASSET_ENCODE_CODE_BIT_COUNT;
            stream_bitcount -= PK_ASSET_ENCODE_CODE_BIT_COUNT;
        }
    
        GenerateCodes(codes, lengths);
    
        for (auto i = 0u; i < PK_ASSET_ENCODE_CODE_COUNT; ++i)
        {
            if (lengths[i] > 0)
            {
                auto step = 1u << lengths[i];
                auto key = (uint16_t)(i | lengths[i] << 8u);

                for (auto j = codes[i]; j < table_size; j += step)
                {
                    table[j] = key;
                }
            }
        }
    
        auto buffer_uint32 = reinterpret_cast<uint32_t*>(write_data);
        auto uint32_count = write_size / sizeof(uint32_t);
        auto byte_offset = uint32_count * sizeof(uint32_t);
    
        for (auto i = 0u; i < uint32_count; ++i)
        {
            stream_bitbuffer |= *(const uint64_t*)(stream_bytes + stream_bytecount) << stream_bitcount;
            stream_bytecount += (63u - stream_bitcount) >> 3u;
            stream_bitcount |= 56u;
            uint32_t upacked = 0u;
    
            for (auto j = 0u; j < 4u; ++j)
            {
                const auto key = table[stream_bitbuffer & ((1ull << PK_ASSET_ENCODE_CODE_LENGTH) - 1ull)];
                upacked |= (key & 0xFFu) << (8u * j);
                stream_bitbuffer >>= key >> 8u;
                stream_bitcount -= key >> 8u;
            }
    
            #if PK_DEBUG // In case we are using inplace decoding. we want to make sure that the write buffer doesn't overrun the read buffer.
            if (static_cast<void*>(buffer_uint32 + i) >= static_cast<const void*>(stream_bytes + stream_bytecount))
            {
                throw std::exception("In place decode buffer overrun!");
            }
            #endif

            buffer_uint32[i] = upacked;
        }
    
        // If not aligned to 32 bits lets decode the remaining 1-3 bytes separately
        for (auto i = byte_offset; i < write_size; ++i)
        {
            stream_bitbuffer |= *(const uint64_t*)(stream_bytes + stream_bytecount) << stream_bitcount;
            stream_bytecount += (63u - stream_bitcount) >> 3u;
            stream_bitcount |= 56u;
            const auto key = table[stream_bitbuffer & ((1ull << PK_ASSET_ENCODE_CODE_LENGTH) - 1ull)];
            write_data[i] = (key & 0xFFu);
            stream_bitbuffer >>= key >> 8u;
            stream_bitcount -= key >> 8u;
        }
    }
}