#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "Utilities/NameID.h"
#include "Utilities/ISingleton.h"
#include "Core/IService.h"

namespace PK::Core
{
    class StringHashRegister : public IService, 
        public Utilities::ISingleton<StringHashRegister>, 
        public Utilities::INameIDProvider
    {
    public:
        StringHashRegister() { Utilities::NameID::SetProvider(this); }

        uint32_t LocalStringToID(const std::string& str);
        uint32_t LocalStringToID(const char* str);
        const std::string& LocalIDToString(const uint32_t& id);

        inline static uint32_t StringToID(const std::string& str) { return StringHashRegister::Get()->LocalStringToID(str); }
        inline static uint32_t StringToID(const char* str) { return Get()->LocalStringToID(str); }
        inline static const std::string& IDToString(const uint32_t& id) { return Get()->LocalIDToString(id); }

        uint32_t INameIDProvider_StringToID(const std::string& name) final { return StringToID(name); }
        const std::string& INameIDProvider_IDToString(const uint32_t& name) final { return IDToString(name); }

    private:
        std::unordered_map<std::string, uint32_t> m_stringIdMap;
        std::vector<std::string> m_idStringMap = { "NULL_ID" };
        uint32_t m_idCounter = 0;
    };
}