#pragma once
#include "NoCopy.h"
#include "FastMap.h"
#include "FastTypeIndex.h"

namespace PK
{
    // Non owning container for simple types.
    // Do not store non trivially destructible types.
    class PropertyBlock : public NoCopy
    {
        protected:
            struct Property
            {
                uint64_t key = 0ull;
                uint32_t offset = 0u;
                uint32_t size = 0u;

                inline bool operator == (const Property& r) const noexcept
                { 
                    return key == r.key; 
                }
            };

            struct PropertyHash
            {
                std::size_t operator()(const Property& k) const noexcept
                {
                    return k.key;
                }
            };

            template<typename T>
            inline static uint64_t MakeKey(const uint32_t hashId)
            {
                return Hash::InterlaceHash32x2(hashId, pk_base_type_index<T>());
            };

        public:
            PropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties);
            ~PropertyBlock();

            void CopyFrom(PropertyBlock& from);
            void Clear();
            void ClearAndReserve(uint64_t capacityBytes, uint64_t capacityProperties);
        
            inline void FreezeLayout() 
            {
                m_fixedLayout = true; 
            }

            template<typename T>
            const T* Get(const uint32_t hashId, size_t* size) const
            {
                return reinterpret_cast<const T*>(GetValueInternal(MakeKey<T>(hashId), size));
            }

            template<typename T>
            const T* Get(const uint32_t hashId) const 
            {
                return Get<T>(hashId, nullptr);
            }
        
            template<typename T>
            bool TryGet(const uint32_t hashId, const T*& value, uint64_t* size) const
            {
                value = Get<T>(hashId, size);
                return value != nullptr;
            }

            template<typename T>
            bool TryGet(const uint32_t hashId, T& value) const 
            {
                const T* ptr = Get<T>(hashId, nullptr);
                if (ptr) value = *ptr;
                return ptr != nullptr;
            }


            template<typename T>
            bool TrySet(uint32_t hashId, const T* src, uint32_t count)
            {
                const auto wsize = sizeof(T) * count;
                const auto index = AddKeyInternal(MakeKey<T>(hashId), wsize);
                return WriteValueInternal(src, index, wsize);
            }

            template<typename T>
            bool TrySet(uint32_t hashId, const T& src) 
            { 
                return TrySet(hashId, &src); 
            }

            template<typename T>
            void Set(uint32_t hashId, const T* src, uint32_t count)
            {
                if (!TrySet(hashId, src, count))
                {
                    throw std::runtime_error("Could not write property to block!");
                }
            }

            template<typename T>
            void Set(uint32_t hashId, const T& src) 
            { 
                Set(hashId, &src, 1u); 
            }


            template<typename T>
            void Reserve(uint32_t hashId, uint32_t count = 1u) 
            { 
                AddKeyInternal(MakeKey<T>(hashId), sizeof(T) * count);
            }
    
        protected:
            void* GetValueInternal(uint64_t key, size_t* size) const;
            uint32_t AddKeyInternal(uint64_t key, size_t size);
            bool WriteValueInternal(const void* src, uint32_t index, size_t writeSize);
            void ValidateBufferSize(uint64_t size);

            FastSet16<Property, PropertyHash> m_properties;
            void* m_buffer = nullptr;
            uint64_t m_capacity = 0ull;
            uint64_t m_head = 0ll;
            bool m_fixedLayout = false;
    };
}
