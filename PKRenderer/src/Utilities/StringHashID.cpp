#include "PrecompiledHeader.h"
#include "Utilities/StringHashID.h"

namespace PK::Utilities
{
    uint32_t StringHashID::LocalStringToID(const std::string& str)
    {
        if (m_stringIdMap.count(str) > 0)
        {
            return m_stringIdMap.at(str);
        }
    
        m_stringIdMap[str] = ++m_idCounter;
        m_idStringMap[m_idCounter] = str;
        return m_idCounter;
    }
    
    uint32_t StringHashID::LocalStringToID(const char* str)
    {
        return StringToID(std::string(str));
    }
    
    const std::string& StringHashID::LocalIDToString(uint32_t id)
    {
        if (id > m_idCounter)
        {
            throw std::invalid_argument("Trying to get a string using an invalid id: " + std::to_string(id));
        }
    
        return m_idStringMap.at(id);
    }
}