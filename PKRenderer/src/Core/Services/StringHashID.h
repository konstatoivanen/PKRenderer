#pragma once
#include "Utilities/ISingleton.h"
#include "Core/Services/IService.h"

namespace PK::Core::Services
{
    class StringHashID : public IService, public Utilities::ISingleton<StringHashID>
    {
        public:
            uint32_t LocalStringToID(const std::string& str);
            uint32_t LocalStringToID(const char* str);
            const std::string& LocalIDToString(uint32_t id);
    
            inline static uint32_t StringToID(const std::string& str) { return Get()->LocalStringToID(str); }
            inline static uint32_t StringToID(const char* str) { return Get()->LocalStringToID(str); }
            inline static const std::string& IDToString(uint32_t id) { return Get()->LocalIDToString(id); }
    
        private:
            std::unordered_map<std::string, uint32_t> m_stringIdMap;
            std::vector<std::string> m_idStringMap = { "NULL_ID" };
            uint32_t m_idCounter = 0;
    };
}