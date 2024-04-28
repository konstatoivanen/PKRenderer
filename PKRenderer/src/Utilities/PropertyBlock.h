#pragma once
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
			const T* GetInternal(const PropertyInfo& info) const
			{
				if ((info.offset + (uint64_t)info.size) > m_capacity)
				{
					throw std::invalid_argument("Out of bounds array index!");
				}

				return reinterpret_cast<const T*>(reinterpret_cast<char*>(m_buffer) + info.offset);
			}

			template<typename T>
			inline std::unordered_map<uint32_t, PropertyInfo>& GetGroupInternal() { return m_properties[std::type_index(typeid(T))]; }
		
			template<typename T>
			bool SetInternal(uint32_t hashId, const T* src, uint32_t count)
			{
				if (count > 0u)
				{
					auto wsize = (uint16_t)(sizeof(T) * count);
					auto& group = GetGroupInternal<T>();
					auto& info = group[hashId];

					if (info.size == 0)
					{
						if (m_explicitLayout)
						{
							return false;
						}

						ValidateBufferSize(m_head + wsize);
						info = { (uint32_t)m_head, wsize };
						m_head += wsize;
					}

					return TryWriteValue(src, info, wsize);
				}

				return false;
			}

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

				if (group != m_properties.end())
				{
					auto iter = group->second.find(hashId);
					return iter != group->second.end() ? GetInternal<T>(iter->second) : nullptr;
				}

				return nullptr;
			}

			template<typename T>
			const bool TryGet(const uint32_t hashId, const T*& value, uint64_t* size = nullptr) const
			{
				auto group = m_properties.find(std::type_index(typeid(T)));

				if (group != m_properties.end())
				{
					auto iter = group->second.find(hashId);

					if (iter != group->second.end())
					{
						if (size != nullptr)
						{
							*size = (uint64_t)iter->second.size;
						}

						value = GetInternal<T>(iter->second);
						return true;
					}
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
			void Set(uint32_t hashId, const T* src, uint32_t count = 1u) { if (!SetInternal(hashId, src, count)) { throw std::runtime_error("Could not write property to block!"); } }

			template<typename T>
			void Set(uint32_t hashId, const T& src) { Set(hashId, &src); }
			
			template<typename T>
			bool TrySet(uint32_t hashId, const T* src, uint32_t count = 1u) { return SetInternal(hashId, src, count); }

			template<typename T>
			bool TrySet(uint32_t hashId, const T& src) { return TrySet(hashId, &src); }

			template<typename T>
			void Reserve(uint32_t hashId, uint32_t count = 1u)
			{
				if (count > 0u)
				{
					auto& group = GetGroupInternal<T>();

					if (group.count(hashId) == 0)
					{
						if (m_explicitLayout)
						{
							throw std::runtime_error("Cannot add elements to explicitly mapped property block!");
						}
						
						auto wsize = (uint16_t)(sizeof(T) * count);

						ValidateBufferSize(m_head + wsize);
						group[hashId] = { (uint32_t)m_head, wsize };
						m_head += wsize;
					}
				}
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