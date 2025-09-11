#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "FastMap.h"
#include "NameID.h"

namespace PK
{
    class NameIDProviderDefault : public INameIDProvider
    {
    public:
        NameIDProviderDefault() : m_names(1024u, 3u)
        {
            NameID::SetProvider(this);
            m_names.Add("NULL_ID");
        }

        uint32_t INameIDProvider_StringToID(const char* name) final;
        const char* INameIDProvider_IDToString(const uint32_t& name) final;

    private:
        FastSet<FixedString128, std::hash<FixedString128>> m_names;
    };
}
