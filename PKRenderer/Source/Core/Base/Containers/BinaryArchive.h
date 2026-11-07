#pragma once
// @TODO BAD DEPENDENCY!
#include "Core/Platform/Platform.h"
#include "FixedString.h"

namespace PK
{
    struct BinaryArchive
    {
        constexpr static const uint32_t MODE_INACTIVE = 0u;
        constexpr static const uint32_t MODE_READ = 1u;
        constexpr static const uint32_t MODE_WRITE = 2u;
        
        struct alignas(16) FileHeader
        {
            uint64_t magic = 0x504B415243485631ull;
            uint64_t hash = 0ull;
            uint64_t padding = 0ull;
            uint64_t size = 0ull;
        };

        struct alignas(16) EntryHeader
        {
            uint64_t hash = 0ull;
            uint64_t size = 0ull;
        };

        BinaryArchive(const char* path, const void* contextId, size_t contextIdSize);
        ~BinaryArchive();

        void Close();
        void Invalidate();

        inline bool IsWriteEnabled() const { return m_mode == MODE_WRITE; }

        const uint8_t* Read(uint64_t hash, size_t* outSize);
        uint8_t* Write(uint64_t hash, size_t writeSize);

    private:

        FixedString256 m_path;
        MappedFile m_file{};
        FileHeader m_header{};
        size_t m_cursor = 0ull;
        uint32_t m_mode = MODE_INACTIVE;
    };
}