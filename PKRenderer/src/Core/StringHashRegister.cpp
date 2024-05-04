#include "PrecompiledHeader.h"
#include "StringHashRegister.h"

namespace PK::Core
{
    uint32_t StringHashRegister::LocalStringToID(const std::string& str)
    {
        auto iter = m_stringIdMap.find(str);

        if (iter != m_stringIdMap.end())
        {
            return iter->second;
        }

        m_stringIdMap[str] = ++m_idCounter;
        m_idStringMap.push_back(str);
        return m_idCounter;
    }

    uint32_t StringHashRegister::LocalStringToID(const char* str)
    {
        return StringToID(std::string(str));
    }

    const std::string& StringHashRegister::LocalIDToString(const uint32_t& id)
    {
        if (id > m_idCounter)
        {
            throw std::invalid_argument("Trying to get a string using an invalid id: " + std::to_string(id));
        }

        return m_idStringMap.at(id);
    }
}