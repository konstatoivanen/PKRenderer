#pragma once
#include <type_traits>
#include "NoCopy.h"
#include "FastTypeIndex.h"

namespace PK
{
    // Non owning container for simple types.
    // Do not store non trivially destructible types.
    // Doesnt support removes. arena style allocation.
    class PropertyBlock : public NoCopy
    {
        protected:
            constexpr static const uint32_t BUCKET_COUNT_FACTOR = 4u;

            struct Property
            {
                uint64_t key = 0ull;
                uint32_t offset = 0u;
                uint16_t size = 0u;
                int16_t previous = -1;
            };

            template<typename T>
            inline static uint64_t MakeKey(const uint32_t hashId)
            {
                static_assert(std::is_trivially_copyable_v<T>, "This container only supports trivially copyable types.");
                return Hash::InterlaceHash32x2(hashId, pk_base_type_index<T>());
            };

        public:
            PropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties);
            ~PropertyBlock();

            void Copy(PropertyBlock& from);
            void Clear();
            void ClearAndReserve(uint64_t capacityBytes, uint64_t capacityProperties);
        
            inline void FreezeLayout() 
            {
                m_fixedLayout = true; 
            }

            template<typename T>
            const T* Get(const uint32_t hashId, size_t* size) const
            {
                return reinterpret_cast<const T*>(GetValuePtr(MakeKey<T>(hashId), size));
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
                const auto index = AddKey(MakeKey<T>(hashId), wsize);
                return WriteValue(src, index, wsize);
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
                AddKey(MakeKey<T>(hashId), sizeof(T) * count);
            }
    
            // The container doesn't handle ownership. use this for types that need destruction.
            template<typename T>
            void Destroy(uint32_t hashId)
            {
                size_t size = 0ull;
                auto ptr = Get<T>(hashId, &size);
                auto count = size / sizeof(T);

                for (auto i = 0u; i < count; ++i)
                {
                    (ptr + i)~T();
                }
            }

            constexpr const void* GetByteBuffer() const { return m_buffer; }
            constexpr void* GetByteBuffer() { return m_buffer; }

        private:
            const void* GetValuePtr(uint64_t key, size_t* size) const;
            uint32_t AddKey(uint64_t key, size_t size);
            bool WriteValue(const void* src, uint32_t index, size_t writeSize);
            bool ReserveMemory(uint64_t byteCapacity, uint32_t propertyCapacity);

            const uint16_t* GetBuckets() const { return m_buffer ? m_buckets : &m_bucketsInline; }
            uint16_t* GetBuckets() { return m_buffer ? m_buckets : &m_bucketsInline; }
            uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % m_bucketCount); }
            void SetValueIndexInBuckets(uint32_t i, int32_t value) { GetBuckets()[i] = (uint16_t)(value + 1); }
            int32_t GetValueIndexFromBuckets(uint32_t i) const { return (int32_t)(GetBuckets()[i]) - 1; }

            void* m_buffer = nullptr;
            Property* m_properties = nullptr;
            
            union
            {
                uint16_t* m_buckets = nullptr;
                uint16_t m_bucketsInline;
            };

            uint64_t m_bufferSize = 0ull;
            uint64_t m_bufferHead = 0ll;
            uint32_t m_propertyCapacity = 0u;
            uint32_t m_propertyCount = 0u;
            uint32_t m_bucketCount = 1u;
            bool m_fixedLayout = false;
    };
}
