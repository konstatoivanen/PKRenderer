#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/NoCopy.h"

namespace PK::Utilities
{
    class PropertyBlock : public Utilities::NoCopy
    {
		protected:
			struct PropertyInfo
			{
				uint32_t offset = 0;
				uint32_t size = 0;
			};

			template<typename T>
			const T* Get(const PropertyInfo& info) const
			{
				if ((info.offset + (uint64_t)info.size) > m_capacity)
				{
					throw std::invalid_argument("Out of bounds array index!");
				}

				return reinterpret_cast<const T*>(reinterpret_cast<char*>(m_buffer) + info.offset);
			}

			template<typename T>
			inline std::unordered_map<uint32_t, PropertyInfo>& GetGroup() { return m_properties[std::type_index(typeid(T))]; }
		
		public:
			PropertyBlock(uint64_t initialCapacity);
			PropertyBlock(void* buffer, uint64_t initialCapacity);
			~PropertyBlock();

			void CopyFrom(PropertyBlock& from);
			virtual void Clear();

			template<typename T>
			const T* Get(const uint32_t hashId) const 
			{
				auto group = m_properties.find(std::type_index(typeid(T)));

				if (group == m_properties.end())
				{
					return nullptr;
				}

				auto iter = group->second.find(hashId);
				return iter != group->second.end() ? Get<T>(iter->second) : nullptr;
			}

			template<typename T>
			const bool TryGet(const uint32_t hashId, const T*& value, uint64_t* size = nullptr) const
			{
				auto group = m_properties.find(std::type_index(typeid(T)));

				if (group == m_properties.end())
				{
					return false;
				}

				auto iter = group->second.find(hashId);

				if (iter != group->second.end())
				{
					if (size != nullptr)
					{
						*size = (uint64_t)iter->second.size;
					}

					value = Get<T>(iter->second);
					return true;
				}

				return false;
			}

			template<typename T>
			const bool TryGet(const uint32_t hashId, T& value) const 
			{
				auto ptr = Get<T>(hashId);
				
				if (ptr != nullptr)
				{
					value = *ptr;
				}

				return ptr != nullptr;
			}

			inline void FreezeLayout() { m_explicitLayout = true; }
		
			template<typename T>
			void Set(uint32_t hashId, const T* src, uint32_t count = 1u)
			{
				if (count < 1u)
				{
					return;
				}

				auto wsize = (uint16_t)(sizeof(T) * count);
				auto& group = GetGroup<T>();
				auto& info = group[hashId];

				if (info.size == 0)
				{
					if (m_explicitLayout)
					{
						throw std::runtime_error("Cannot add elements to explicitly mapped property block!");
					}

					ValidateBufferSize(m_head + wsize);
					info = { (uint32_t)m_head, wsize };
					m_head += wsize;
				}

				if (!TryWriteValue(src, info, wsize))
				{
					throw std::invalid_argument("Trying to write values to a block that has insufficient capacity!");
				}
			}

			template<typename T>
			void Set(uint32_t hashId, const T& src) { Set(hashId, &src); }

			template<typename T>
			void Reserve(uint32_t hashId, uint32_t count = 1u)
			{
				if (count < 1u)
				{
					return;
				}

				auto& group = GetGroup<T>();

				if (group.count(hashId) > 0)
				{
					return;
				}

				if (m_explicitLayout)
				{
					throw std::runtime_error("Cannot add elements to explicitly mapped property block!");
				}
				
				auto wsize = (uint16_t)(sizeof(T) * count);

				ValidateBufferSize(m_head + wsize);
				group[hashId] = { (uint32_t)m_head, wsize };
				m_head += wsize;
			}
	
		protected:
			bool TryWriteValue(const void* src, PropertyInfo& info, uint64_t writeSize);
			void ValidateBufferSize(uint64_t size);
			void SetForeign(void* buffer, uint64_t capacity);

			bool m_foreignBuffer = false;
			bool m_explicitLayout = false;
			void* m_buffer = nullptr;
			uint64_t m_capacity = 0ull;
			uint64_t m_head = 0ll;
			std::unordered_map<std::type_index, std::unordered_map<uint32_t, PropertyInfo>> m_properties;
    };
}