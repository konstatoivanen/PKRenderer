#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Utilities/BufferView.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/Structs/Layout.h"
#include "Rendering/Structs/StructsCommon.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class Buffer : public NoCopy, public NativeInterface<Buffer>
    {
        public:
            static Ref<Buffer> Create(const BufferLayout& layout, const void* data, size_t count, BufferUsage usage);

            inline static Ref<Buffer> CreateVertex(const BufferLayout& layout, 
                                                   const void* data, 
                                                   size_t count, 
                                                   BufferUsage extraFlags = BufferUsage::None)
            { 
                return Create(layout, data, count, BufferUsage::DefaultVertex | extraFlags);
            }

            inline static Ref<Buffer> CreateIndex(ElementType type, 
                                                  const void* data, 
                                                  size_t count,
                                                  BufferUsage extraFlags = BufferUsage::None)
            {
                return Create(BufferLayout({{ type, "INDEX" }}), data, count, BufferUsage::DefaultIndex | extraFlags);
            }

            inline static Ref<Buffer> CreateConstant(const BufferLayout& layout, BufferUsage extraFlags = BufferUsage::None)
            {
                return Create(layout, nullptr, 1, BufferUsage::DefaultConstant | extraFlags);
            }

            inline static Ref<Buffer> CreateStorage(const BufferLayout& layout, size_t count, BufferUsage extraFlags = BufferUsage::None)
            {
                return Create(layout, nullptr, count, BufferUsage::DefaultStorage | extraFlags);
            }

            virtual ~Buffer() = default;
            virtual void SetData(const void* data, size_t offset, size_t size) = 0;
            virtual void SetSubData(const void* data, size_t offset, size_t size) = 0;
            virtual void* BeginMap(size_t offset, size_t size) = 0;
            virtual void EndMap() = 0;

            template<typename T>
            BufferView<T> BeginMap()
            {
                return { reinterpret_cast<T*>(BeginMap(0, GetCapacity())), GetCapacity() / sizeof(T) };
            }

            template<typename T>
            BufferView<T> BeginMap(size_t offset, size_t count)
            {
                auto tsize = sizeof(T);
                auto mapSize = tsize * count + tsize * offset;
                auto bufSize = GetCapacity();

                PK_THROW_ASSERT(mapSize <= bufSize, "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", mapSize, bufSize);

                return { reinterpret_cast<T*>(BeginMap(offset * tsize, count * tsize)), count };
            }

            virtual void MakeRangeResident(const IndexRange& range) = 0;
            virtual void MakeRangeNonResident(const IndexRange& range) = 0;

            virtual bool Validate(size_t count) = 0;
            virtual size_t GetCapacity() const = 0;

            constexpr size_t GetCount() const { return m_count; }
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