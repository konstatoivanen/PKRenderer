#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "FastSet.h"
#include "NameID.h"

namespace PK
{
    class NameIDProviderDefault : public INameIDProvider
    {
    public:
        NameIDProviderDefault() : m_names(1024)
        {
            NameID::SetProvider(this);
            m_names.Add("NULL_ID");
        }

        uint32_t INameIDProvider_StringToID(const char* name) final;
        const char* INameIDProvider_IDToString(const uint32_t& name) final;

    private:
        FastSet<FixedString128> m_names;
    };
}