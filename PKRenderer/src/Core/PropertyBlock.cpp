#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK::Core
{
    PropertyBlock::PropertyBlock(size_t initialCapacity, bool deltaChecks) : m_deltaChecks(deltaChecks)
    {
		ValidateBufferSize(initialCapacity);
    }

	PropertyBlock::~PropertyBlock()
	{
		if (m_buffer != nullptr)
		{
			free(m_buffer);
		}
	}

	void PropertyBlock::CopyFrom(PropertyBlock& from)
	{
		auto& propsm = m_properties;
		auto& propst = from.m_properties;

		for (auto& mine : propsm)
		{
			auto theirs = propst.find(mine.first);

			if (theirs != propst.end())
			{
				TryWriteValue(reinterpret_cast<char*>(from.m_buffer) + theirs->second.offset, mine.second, theirs->second.type, theirs->second.size);
			}
		}
	}

	void PropertyBlock::Clear()
	{
		m_head = 0;
		m_properties.clear();
		memset(m_buffer, 0, m_capacity);
	}

	bool PropertyBlock::TryWriteValue(const void* src, PropertyInfo& info, std::type_index writeType, size_t writeSize)
	{
		if (info.type != writeType || info.size < writeSize)
		{
			return false;
		}

		auto dst = reinterpret_cast<char*>(m_buffer) + info.offset;

		if (!m_deltaChecks || memcmp(dst, src, writeSize) != 0)
		{
			info.hasChanged = 1;
			memcpy(dst, src, writeSize);
		}

		return true;
	}

	void PropertyBlock::ValidateBufferSize(size_t size)
	{
		if (size <= m_capacity)
		{
			return;
		}

		auto newBuffer = calloc(1, size);

		PK_THROW_ASSERT(newBuffer != nullptr, "Failed to allocate new buffer!");

		if (m_buffer != nullptr)
		{
			memcpy(newBuffer, m_buffer, m_capacity);
			free(m_buffer);
		}

		m_capacity = size;
		m_buffer = newBuffer;
	}
}