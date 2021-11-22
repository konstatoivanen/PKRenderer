#pragma once
#include "Core/ISingleton.h"
#include "Core/IService.h"

namespace PK::Utilities
{
    using namespace Core;

    class StringHashID : public IService, public ISingleton<StringHashID>
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
            std::unordered_map<uint32_t, std::string> m_idStringMap;
            uint32_t m_idCounter = 0;
    };
}