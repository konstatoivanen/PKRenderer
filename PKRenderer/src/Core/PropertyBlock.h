#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/Log.h"
#include "Core/NoCopy.h"

namespace PK::Core
{
	template<typename T>
	struct Handle 
	{ 
		const T* handle = nullptr; 
		Handle(const T* ptr) { handle = ptr; }
		void operator = (const T* ptr) { handle = ptr; }
		operator const T* () const { return handle; }
	};

	template<typename T>
	struct HandleArray 
	{
		const T* const* handles; 
		HandleArray(const T* const* ptr) { handles = ptr; }
		void operator = (const T* const* ptr) { handles = ptr; }
		operator const T* const* () const { return handles; }
	};

    class PropertyBlock : public NoCopy
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
			PropertyBlock(size_t initialCapacity, bool deltaChecks = false);
			~PropertyBlock();

			void CopyFrom(PropertyBlock& from);
			virtual void Clear();
		
			template<typename T>
			const T* Get(const PropertyInfo& info) const
			{
				if ((info.offset + (size_t)info.size) > m_capacity)
				{
					PK_THROW_ERROR("OOB ARRAY INDEX! idx: %i, capacity: %i", info.offset + info.size, m_capacity);
				}

				return reinterpret_cast<const T*>(reinterpret_cast<char*>(m_buffer) + info.offset);
			}

			template<typename T>
			const T* Get(const uint hashId) const 
			{
				auto iter = m_properties.find(hashId);
				return iter != m_properties.end() ? Get<T>(iter->second) : nullptr;
			}

			template<typename T>
			const bool TryGet(const uint hashId, const T*& value, size_t* size = nullptr) const
			{
				auto iter = m_properties.find(hashId);

				if (iter != m_properties.end())
				{
					if (size != nullptr)
					{
						*size = (size_t)iter->second.size;
					}

					value = Get<T>(iter->second);
					return true;
				}

				return false;
			}

			template<typename T>
			const bool TryGet(const uint hashId, T& value) const 
			{
				auto ptr = Get<T>(hashId);
				
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
					ValidateBufferSize(m_head + wsize);
					info = { type, (uint)m_head, wsize, 1 };
					m_head += wsize;
				}

				if (!TryWriteValue(src, info, type, wsize))
				{
					PK_THROW_ERROR("INVALID DATA FORMAT! %i", hashId);
				}
			}

			template<typename T>
			void Set(uint hashId, const T& src) { Set(hashId, &src); }

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

				ValidateBufferSize(m_head + wsize);
				m_properties[hashId] = { type, (uint)m_head, wsize, 1 };
				m_head += wsize;
			}
	
		protected:
			bool TryWriteValue(const void* src, PropertyInfo& info, std::type_index writeType, size_t writeSize);
			void ValidateBufferSize(size_t size);

			bool m_explicitLayout = false;
			bool m_deltaChecks = false;
			void* m_buffer = nullptr;
			size_t m_capacity = 0ull;
			size_t m_head = 0ll;

			std::unordered_map<uint, PropertyInfo> m_properties;
    };
}