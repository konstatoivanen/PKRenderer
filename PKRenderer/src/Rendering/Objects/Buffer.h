#pragma once
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class Buffer : public PK::Core::NoCopy
    {
        public:
            static Ref<Buffer> Create(BufferUsage usage, const BufferLayout& layout, size_t count);

            virtual ~Buffer() = default;
            virtual void SetData(const void* data, size_t offset, size_t size) const = 0;
            virtual bool Validate(size_t count) = 0;
            virtual size_t GetCapacity() const = 0;

            constexpr size_t GetCount() const { return m_count; }
            constexpr const BufferUsage GetUsage() const { return m_usage; }
            constexpr const BufferLayout& GetLayout() const { return m_layout; }

            template<typename T>
            const T* GetNative() const
            {
                static_assert(std::is_base_of<Buffer, T>::value, "Template argument type does not derive from Buffer!");
                return static_cast<const T*>(this);
            }

        protected:
            Buffer(BufferUsage usage, const BufferLayout& layout, size_t count) : m_usage(usage), m_layout(layout), m_count(count) {}
            
            BufferLayout m_layout{};
            BufferUsage m_usage = BufferUsage::None;
            size_t m_count = 0;
    };
}