#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK::Core
{
    PropertyBlock::PropertyBlock(bool deltaChecks) : m_explicitLayout(false), m_deltaChecks(deltaChecks), m_currentByteOffset(0)
    {
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
				TryWriteValue(from.m_data.data() + theirs->second.offset, mine.second, theirs->second.type, theirs->second.size);
			}
		}
	}

	void PropertyBlock::Clear()
	{
		m_currentByteOffset = 0;
		m_properties.clear();
		memset(m_data.data(), 0, m_data.size());
	}

	bool PropertyBlock::TryWriteValue(const void* src, PropertyInfo& info, std::type_index writeType, size_t writeSize)
	{
		if (info.type != writeType || info.size < writeSize)
		{
			return false;
		}

		auto dst = m_data.data() + info.offset;

		if (!m_deltaChecks || memcmp(dst, src, writeSize) != 0)
		{
			info.hasChanged = 1;
			memcpy(dst, src, writeSize);
		}

		return true;
	}
}