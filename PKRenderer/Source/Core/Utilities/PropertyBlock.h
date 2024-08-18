#pragma once
#include "NoCopy.h"
#include "FastMap.h"

namespace PK
{
    class PropertyBlock : public NoCopy
    {
        protected:
            struct PropertyKey
            {
                const type_info* typeInfo;
                uint32_t hashId;
                inline bool operator == (const PropertyKey& r) const noexcept { return hashId == r.hashId && *typeInfo == *r.typeInfo; }

                template<typename T>
                inline static PropertyKey Make(uint32_t hashId) { return { &typeid(T), hashId }; }
            };

            struct PropertyInfo
            {
                uint32_t offset = 0;
                uint32_t size = 0;
            };

            struct PropertyKeyHash
            {
                std::size_t operator()(const PropertyKey& k) const noexcept
                {
                    return k.typeInfo->hash_code() ^ k.hashId;
                }
            };

            template<typename T>
            const T* GetInternal(const PropertyInfo& info) const
            {
                if ((info.offset + (uint64_t)info.size) > m_capacity)
                {
                    throw std::invalid_argument("Out of bounds array index!");
                }

                return reinterpret_cast<const T*>(reinterpret_cast<char*>(m_buffer) + info.offset);
            }

            template<typename T>
            uint32_t ReserveInternal(uint32_t hashId, uint32_t count)
            {
                if (count > 0 && !m_fixedLayout)
                {
                    uint32_t index = 0u;
                    if (m_propertyInfos.AddKey(PropertyKey::Make<T>(hashId), &index))
                    {
                        const auto wsize = (uint32_t)(sizeof(T) * count);
                        ValidateBufferSize(m_head + wsize);
                        m_propertyInfos.SetValueAt(index, { (uint32_t)m_head, wsize });
                        m_head += wsize;
                    }

                    return index;
                }

                return ~0u;
            }

            template<typename T>
            bool SetInternal(uint32_t hashId, const T* src, uint32_t count)
            {
                const auto wsize = (uint16_t)(sizeof(T) * count);
                const auto key = PropertyKey::Make<T>(hashId);
                const auto index = m_fixedLayout ? (uint32_t)m_propertyInfos.GetIndex(key) : ReserveInternal<T>(hashId, count);
                return TryWriteValue(src, index, wsize);
            }

        public:
            PropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties);
            ~PropertyBlock();

            void CopyFrom(PropertyBlock& from);
            void Clear();
            void ClearAndReserve(uint64_t capacityBytes, uint64_t capacityProperties);
        
            inline void FreezeLayout() { m_fixedLayout = true; }

            template<typename T>
            const T* Get(const uint32_t hashId) const 
            {
                auto info = m_propertyInfos.GetValueRef(PropertyKey::Make<T>(hashId));
                return info != nullptr ? GetInternal<T>(*info) : nullptr;
            }

            template<typename T>
            bool TryGet(const uint32_t hashId, T& value) const 
            {
                auto ptr = Get<T>(hashId);
                if (ptr != nullptr) value = *ptr;
                return ptr != nullptr;
            }
        
            template<typename T>
            bool TryGet(const uint32_t hashId, const T*& value, uint64_t* size = nullptr) const
            {
                auto info = m_propertyInfos.GetValueRef(PropertyKey::Make<T>(hashId));

                if (info != nullptr)
                {
                    if (size != nullptr) *size = (uint64_t)info->size;
                    value = GetInternal<T>(*info);
                    return true;
                }

                return false;
            }

            template<typename T>
            void Set(uint32_t hashId, const T* src, uint32_t count = 1u) { if (!SetInternal(hashId, src, count)) throw std::runtime_error("Could not write property to block!"); }

            template<typename T>
            void Set(uint32_t hashId, const T& src) { Set(hashId, &src); }

            template<typename T>
            bool TrySet(uint32_t hashId, const T* src, uint32_t count = 1u) { return SetInternal(hashId, src, count); }
            template<typename T>

            bool TrySet(uint32_t hashId, const T& src) { return TrySet(hashId, &src); }

            template<typename T>
            void Reserve(uint32_t hashId, uint32_t count = 1u) { ReserveInternal<T>(hashId, count); }
    
        protected:
            bool TryWriteValue(const void* src, uint32_t index, uint64_t writeSize);
            void ValidateBufferSize(uint64_t size);

            bool m_fixedLayout = false;
            void* m_buffer = nullptr;
            uint64_t m_capacity = 0ull;
            uint64_t m_head = 0ll;
            FastMap<PropertyKey, PropertyInfo, PropertyKeyHash> m_propertyInfos;
    };
}