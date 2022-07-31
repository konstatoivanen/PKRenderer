#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Utilities/BufferView.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/Structs/Layout.h"
#include "Rendering/Structs/StructsCommon.h"

namespace PK::Rendering::Objects
{
    class Buffer : public Utilities::NoCopy, public Utilities::NativeInterface<Buffer>
    {
        public:
            static Utilities::Ref<Buffer> Create(const Structs::BufferLayout& layout, const void* data, size_t count, Structs::BufferUsage usage, const char* name);

            inline static Utilities::Ref<Buffer> CreateVertex(const Structs::BufferLayout& layout,
                                                   const void* data, 
                                                   size_t count, 
                                                   Structs::BufferUsage extraFlags,
                                                   const char* name)
            { 
                return Create(layout, data, count, Structs::BufferUsage::DefaultVertex | extraFlags, name);
            }

            inline static Utilities::Ref<Buffer> CreateIndex(Structs::ElementType type,
                                                  const void* data, 
                                                  size_t count,
                                                  Structs::BufferUsage extraFlags,
                                                  const char* name)
            {
                return Create(Structs::BufferLayout({{ type, "INDEX" }}), data, count, Structs::BufferUsage::DefaultIndex | extraFlags, name);
            }

            inline static Utilities::Ref<Buffer> CreateConstant(const Structs::BufferLayout& layout, 
                                                                Structs::BufferUsage extraFlags,
                                                                const char* name)
            {
                return Create(layout, nullptr, 1, Structs::BufferUsage::DefaultConstant | extraFlags, name);
            }

            inline static Utilities::Ref<Buffer> CreateStorage(const Structs::BufferLayout& layout, 
                                                               size_t count, 
                                                               Structs::BufferUsage extraFlags,
                                                               const char* name)
            {
                return Create(layout, nullptr, count, Structs::BufferUsage::DefaultStorage | extraFlags, name);
            }

            virtual ~Buffer() = default;
            virtual void SetData(const void* data, size_t offset, size_t size) = 0;
            virtual void SetSubData(const void* data, size_t offset, size_t size) = 0;
            virtual void* BeginWrite(size_t offset, size_t size) = 0;
            virtual void EndWrite() = 0;

            // This will likely throw for memory types that cannot be host read (i.e. GPU only).
            virtual const void* BeginRead(size_t offset, size_t size) = 0;
            virtual void EndRead() = 0;

            template<typename T>
            Utilities::BufferView<T> BeginWrite()
            {
                return { reinterpret_cast<T*>(BeginWrite(0, GetCapacity())), GetCapacity() / sizeof(T) };
            }

            template<typename T>
            Utilities::BufferView<T> BeginWrite(size_t offset, size_t count)
            {
                auto tsize = sizeof(T);
                auto mapSize = tsize * count + tsize * offset;
                auto bufSize = GetCapacity();

                PK_THROW_ASSERT(mapSize <= bufSize, "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", mapSize, bufSize);

                return { reinterpret_cast<T*>(BeginWrite(offset * tsize, count * tsize)), count };
            }

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

            virtual void MakeRangeResident(const Structs::IndexRange& range) = 0;
            virtual void MakeRangeNonResident(const Structs::IndexRange& range) = 0;

            virtual bool Validate(size_t count) = 0;
            virtual size_t GetCapacity() const = 0;

            constexpr size_t GetCount() const { return m_count; }
            constexpr bool IsSparse() const { return (m_usage & Structs::BufferUsage::Sparse) != 0; }
            constexpr const Structs::BufferUsage GetUsage() const { return m_usage; }
            constexpr const Structs::BufferLayout& GetLayout() const { return m_layout; }
            constexpr Structs::IndexRange GetFullRange() const { return { 0ull, m_count }; }

        protected:
            Buffer(const Structs::BufferLayout& layout, size_t count, Structs::BufferUsage usage) : m_usage(usage), m_layout(layout), m_count(count) {}
            
            Structs::BufferLayout m_layout{};
            Structs::BufferUsage m_usage = Structs::BufferUsage::None;
            Structs::InputRate m_inputRate = Structs::InputRate::PerVertex;
            size_t m_count = 0;
    };
}