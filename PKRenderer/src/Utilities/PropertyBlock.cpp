#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK::Utilities
{
    PropertyBlock::PropertyBlock(uint64_t initialCapacity)
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
		auto& groupsm = m_properties;
		auto& groupst = from.m_properties;

		for (auto& kvm : groupsm)
		{
			auto kvt = groupst.find(kvm.first);

			if (kvt == groupst.end())
			{
				continue;
			}

			auto& gm = kvm.second;
			auto& gt = kvt->second;

			for (auto& mine : gm)
			{
				auto theirs = gt.find(mine.first);

				if (theirs != gt.end())
				{
					TryWriteValue(reinterpret_cast<char*>(from.m_buffer) + theirs->second.offset, mine.second, theirs->second.size);
				}
			}
		}
	}

	void PropertyBlock::Clear()
	{
		m_head = 0;
		m_properties.clear();
		memset(m_buffer, 0, m_capacity);
	}

	bool PropertyBlock::TryWriteValue(const void* src, PropertyInfo& info, uint64_t writeSize)
	{
		if (info.size < writeSize)
		{
			return false;
		}

		auto dst = reinterpret_cast<char*>(m_buffer) + info.offset;
		memcpy(dst, src, writeSize);
		return true;
	}

	void PropertyBlock::ValidateBufferSize(uint64_t size)
	{
		if (size <= m_capacity)
		{
			return;
		}

		auto newBuffer = calloc(1, size);

		if (newBuffer == nullptr)
		{
			throw std::runtime_error("Failed to allocate new buffer!");
		}

		if (m_buffer != nullptr)
		{
			memcpy(newBuffer, m_buffer, m_capacity);
			free(m_buffer);
		}

		m_capacity = size;
		m_buffer = newBuffer;
	}
}