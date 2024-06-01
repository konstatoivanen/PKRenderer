#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "NameID.h"

namespace PK
{
    class NameIDProviderDefault : public INameIDProvider
    {
    public:
        NameIDProviderDefault() { NameID::SetProvider(this); }

        uint32_t INameIDProvider_StringToID(const std::string& name) final;
        const std::string& INameIDProvider_IDToString(const uint32_t& name) final;

    private:
        std::unordered_map<std::string, uint32_t> m_stringIdMap;
        std::vector<std::string> m_idStringMap = { "NULL_ID" };
        uint32_t m_idCounter = 0;
    };
}