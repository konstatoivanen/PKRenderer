#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/Log.h"
#include "Core/NoCopy.h"

namespace PK::Core
{
    class PropertyBlock : public NoCopy, public std::vector<char>
    {
		using uint = unsigned int;
		using ushort = unsigned short;

		protected:
			struct PropertyInfo
			{
				// type index has no empty constructor so lets hack it like this :)
				std::type_index type = std::type_index(typeid(PropertyInfo));
				uint offset = 0;
				ushort size = 0;
				ushort hasChanged = 0;
			
				template<typename T>
				bool Is() { return type == std::type_index(typeid(T)); }
			};
			
		public:
			PropertyBlock(bool deltaChecks = false);
			virtual ~PropertyBlock() = default;
			void CopyFrom(PropertyBlock& from);
			virtual void Clear();
		
			template<typename T>
			const T* GetElementPtr(const PropertyInfo& info) const
			{
				if ((info.offset + (size_t)info.size) > capacity())
				{
					PK_THROW_ERROR("OOB ARRAY INDEX! idx: %i, capacity: %i", info.offset + info.size, capacity());
				}

				return reinterpret_cast<const T*>(data() + info.offset);
			}
	
			template<typename T>
			const T* GetPropertyPtr(const uint hashId) const 
			{
				auto iter = m_properties.find(hashId);
				return iter != m_properties.end() ? GetElementPtr<T>(iter->second) : nullptr;
			}


			template<typename T>
			const bool TryGetPropertyPtr(const uint hashId, const T*& value, size_t* size = nullptr) const
			{
				auto iter = m_properties.find(hashId);

				if (iter != m_properties.end())
				{
					if (size != nullptr)
					{
						*size = (size_t)iter->second.size;
					}

					value = GetElementPtr<T>(iter->second);
					return true;
				}

				return false;
			}

			template<typename T>
			const bool TryGetPropertyValue(const uint hashId, T& value) const 
			{
				auto ptr = GetPropertyPtr<T>(hashId);
				
				if (ptr != nullptr)
				{
					value = *ptr;
				}

				return ptr != nullptr;
			}

			inline bool HasChanged(const uint hashId) { return m_properties.at(hashId).hasChanged != 0; }
			inline void MarkAsRead(const uint hashId) { m_properties.at(hashId).hasChanged = 0; }
			inline void FreezeLayout() { m_explicitLayout = true; }
	
			std::unordered_map<uint, PropertyInfo>::iterator begin() { return m_properties.begin(); }
			std::unordered_map<uint, PropertyInfo>::iterator end() { return m_properties.end(); }
			std::unordered_map<uint, PropertyInfo>::const_iterator begin() const { return m_properties.begin(); }
			std::unordered_map<uint, PropertyInfo>::const_iterator end() const { return m_properties.end(); }
		
			template<typename T>
			void Set(uint hashId, const T* src, uint count = 1)
			{
				PK_THROW_ASSERT(count > 0, "INVALID DATA COUNT!");

				auto wsize = (ushort)(sizeof(T) * count);
				auto type = std::type_index(typeid(T));
				auto& info = m_properties[hashId];

				if (info.size == 0)
				{
					PK_THROW_ASSERT(!m_explicitLayout, "Cannot add elements to explicitly mapped property block!");

					if (m_currentByteOffset >= size())
					{
						resize(size() + wsize);
					}

					info = { type, m_currentByteOffset, wsize, 1 };
					m_currentByteOffset += wsize;
				}

				if (!TryWriteValue(src, info, type, wsize))
				{
					PK_THROW_ERROR("INVALID DATA FORMAT! %i", hashId);
				}
			}

			template<typename T>
			void Reserve(uint hashId, uint count = 1)
			{
				if (m_properties.count(hashId) > 0)
				{
					return;
				}

				PK_THROW_ASSERT(count > 0, "INVALID DATA COUNT!");
				PK_THROW_ASSERT(!m_explicitLayout, "Cannot add elements to explicitly mapped property block!");

				auto wsize = (ushort)(sizeof(T) * count);
				auto type = std::type_index(typeid(T));


				if (m_currentByteOffset >= size())
				{
					resize(size() + wsize);
				}

				m_properties[hashId] = { type, m_currentByteOffset, wsize, 1 };
				m_currentByteOffset += wsize;
			}

			template<typename T>
			void Set(uint hashId, const T& src) { Set(hashId, &src); }
	
		protected:
			bool TryWriteValue(const void* src, PropertyInfo& info, std::type_index writeType, size_t writeSize);

			bool m_explicitLayout = false;
			bool m_deltaChecks = false;
			uint m_currentByteOffset = 0;
			std::unordered_map<uint, PropertyInfo> m_properties;
    };
}