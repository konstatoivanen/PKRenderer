#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Layout.h"

namespace PK::Rendering::RHI::Objects
{
    typedef Utilities::Ref<class Buffer> BufferRef;

    class Buffer : public Utilities::NoCopy, public Utilities::NativeInterface<Buffer>
    {
        public:
            static BufferRef Create(size_t size, BufferUsage usage, const char* name);

            template<typename T>
            inline static BufferRef Create(size_t count, BufferUsage usage, const char* name) { return Create(sizeof(T) * count, usage, name); }

            virtual ~Buffer() = 0;

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
                return { reinterpret_cast<const T*>(BeginRead(offset * sizeof(T), count * sizeof(T))), count };
            }

            virtual size_t SparseAllocate(const size_t size, QueueType type) = 0;
            virtual void SparseAllocateRange(const IndexRange& range, QueueType type) = 0;
            virtual void SparseDeallocate(const IndexRange& range) = 0;

            virtual bool Validate(size_t size) = 0;
            virtual size_t GetCapacity() const = 0;
            
            template<typename T>
            inline bool Validate(size_t count) { return Validate(sizeof(T) * count); }

            constexpr size_t GetSize() const { return m_size; }
            template<typename T>
            constexpr size_t GetCount() const { return m_size / sizeof(T); }
            constexpr bool IsSparse() const { return (m_usage & BufferUsage::Sparse) != 0; }
            constexpr bool IsConcurrent() const { return (m_usage & BufferUsage::Concurrent) != 0u; }
            constexpr const BufferUsage GetUsage() const { return m_usage; }
            constexpr IndexRange GetFullRange() const { return { 0ull, m_size }; }

        protected:
            Buffer(size_t size, BufferUsage usage) : m_usage(usage), m_size(size) {}
            
            BufferUsage m_usage = BufferUsage::None;
            size_t m_size = 0;
    };
}