#pragma once
#include "Core/CLI/Log.h"
#include "Utilities/Ref.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Layout.h"

namespace PK::Rendering::RHI::Objects
{
    typedef Utilities::Ref<class Buffer> BufferRef;

    class Buffer : public Utilities::NoCopy, public Utilities::NativeInterface<Buffer>
    {
        public:
            static BufferRef Create(const BufferLayout& layout, size_t count, BufferUsage usage, const char* name);

            inline static BufferRef Create(ElementType type, size_t count, BufferUsage usage, const char* name)
            {
                return Create(BufferLayout({ { type, "DATA" } }), count, usage, name);
            }

            inline static BufferRef Create(const BufferLayout& layout, BufferUsage usage, const char* name)
            {
                return Create(layout, 1, usage, name);
            }

            virtual ~Buffer() = default;

            // This will likely throw for memory types that cannot be host read (i.e. GPU only).
            virtual const void* BeginRead(size_t offset, size_t size) = 0;
            virtual void EndRead() = 0;

            template<typename T>
            Utilities::ConstBufferView<T> BeginRead()
            {
                return { reinterpret_cast<const T*>(BeginRead(0, GetCapacity())), GetCapacity() / sizeof(T) };
            }

            template<typename T>
            Utilities::ConstBufferView<T> BeginRead(size_t offset, size_t count)
            {
                auto tsize = sizeof(T);
                auto mapSize = tsize * count + tsize * offset;
                auto bufSize = GetCapacity();

                PK_THROW_ASSERT(mapSize <= bufSize, "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", mapSize, bufSize);

                return { reinterpret_cast<const T*>(BeginRead(offset * tsize, count * tsize)), count };
            }

            virtual size_t SparseAllocate(const size_t size, QueueType type) = 0;
            virtual void SparseAllocateRange(const IndexRange& range, QueueType type) = 0;
            virtual void SparseDeallocate(const IndexRange& range) = 0;

            virtual bool Validate(size_t count) = 0;
            virtual size_t GetCapacity() const = 0;

            constexpr size_t GetCount() const { return m_count; }
            constexpr bool IsSparse() const { return (m_usage & BufferUsage::Sparse) != 0; }
            constexpr bool IsConcurrent() const { return (m_usage & BufferUsage::Concurrent) != 0u; }
            constexpr const BufferUsage GetUsage() const { return m_usage; }
            constexpr const BufferLayout& GetLayout() const { return m_layout; }
            constexpr IndexRange GetFullRange() const { return { 0ull, m_count }; }

        protected:
            Buffer(const BufferLayout& layout, size_t count, BufferUsage usage) : m_usage(usage), m_layout(layout), m_count(count) {}
            
            BufferLayout m_layout{};
            BufferUsage m_usage = BufferUsage::None;
            InputRate m_inputRate = InputRate::PerVertex;
            size_t m_count = 0;
    };
}