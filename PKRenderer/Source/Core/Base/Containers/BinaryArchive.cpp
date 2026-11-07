#include "PrecompiledHeader.h"
#include "Core/Base/Hash.h"
#include "BinaryArchive.h"

namespace PK
{
    constexpr static const size_t INITIAL_WRITE_CAPACITY = 4ull * 1024ull * 1024ull;

    BinaryArchive::BinaryArchive(const char* path, const void* contextId, size_t contextIdSize)
    {
        m_path = FixedString256(path);
        m_header.hash = Hash::FNV1a(contextId, contextIdSize, 14695981039346656037ull);
        m_header.size = sizeof(FileHeader);

        if (Platform::MappedFileRead(path, &m_file))
        {
            if (m_file.size > sizeof(FileHeader))
            {
                const auto* header = reinterpret_cast<const FileHeader*>(m_file.data);

                if (header->magic == m_header.magic &&
                    header->hash == m_header.hash &&
                    header->size > sizeof(FileHeader) &&
                    header->size <= m_file.size)
                {
                    m_file.size = header->size;
                    m_cursor = sizeof(FileHeader);
                    m_mode = MODE_READ;
                    return;
                }
            }

            Platform::MappedFileClose(&m_file);
        }

        if (Platform::MappedFileWrite(path, INITIAL_WRITE_CAPACITY, &m_file))
        {
            memcpy(m_file.data, &m_header, sizeof(FileHeader));
            m_cursor = sizeof(FileHeader);
            m_mode = MODE_WRITE;
        }
    }

    BinaryArchive::~BinaryArchive()
    {
        Close();
    }

    void BinaryArchive::Close()
    {
        auto writeSize = 0ull;

        if (m_mode == MODE_WRITE && m_file.data && m_cursor > sizeof(FileHeader))
        {
            m_header.size = m_cursor;
            writeSize = m_cursor;
            memcpy(m_file.data, &m_header, sizeof(FileHeader));
        }

        Platform::MappedFileClose(&m_file, writeSize);
        m_cursor = 0ull;
        m_mode = MODE_INACTIVE;
    }

    void BinaryArchive::Invalidate()
    {
        m_mode = MODE_INACTIVE;
        Close();
        Platform::RemoveFile(m_path.c_str());
    }

    const uint8_t* BinaryArchive::Read(uint64_t hash, size_t* outSize)
    {
        if (m_mode != MODE_READ)
        {
            return nullptr;
        }

        if (m_file.data && m_cursor + sizeof(EntryHeader) <= m_file.size)
        {
            const auto* entry = reinterpret_cast<const EntryHeader*>(m_file.data + m_cursor);
            const auto size = ((entry->size + 15ull) / 16ull) * 16ull;
            const auto advance = sizeof(EntryHeader) + size;

            if (entry->hash == hash && entry->size && m_cursor + advance <= m_file.size)
            {
                const auto* payload = m_file.data + m_cursor + sizeof(EntryHeader);
                *outSize = entry->size;
                m_cursor += advance;
                return payload;
            }
        }

        Invalidate();
        return nullptr;
    }

    uint8_t* BinaryArchive::Write(uint64_t hash, size_t writeSize)
    {
        if (m_mode == MODE_READ)
        {
            Invalidate();
            return nullptr;
        }

        if (m_mode != MODE_WRITE || !m_file.data || !writeSize)
        {
            return nullptr;
        }

        const auto size = ((writeSize + 15ull) / 16ull) * 16ull;
        const auto advance = sizeof(EntryHeader) + size;

        if (m_cursor + advance > m_file.size)
        {
            const auto sizePadded = m_file.size * 2ull;
            const auto sizeAdvance = m_cursor + advance;
            const auto sizeResize = sizePadded > sizeAdvance ? sizePadded : sizeAdvance;

            if (!Platform::MappedFileResize(&m_file, sizeResize))
            {
                Invalidate();
                return nullptr;
            }
        }

        if (size > writeSize)
        {
            memset(m_file.data + m_cursor + sizeof(EntryHeader) + writeSize, 0, size - writeSize);
        }

        auto* entry = reinterpret_cast<EntryHeader*>(m_file.data + m_cursor);
        auto* payload = m_file.data + m_cursor + sizeof(EntryHeader);
        entry->hash = hash;
        entry->size = writeSize;
        m_cursor += advance;
        return payload;
    }
}
