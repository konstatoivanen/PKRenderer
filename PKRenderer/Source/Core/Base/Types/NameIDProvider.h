#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Types/NameID.h"

namespace PK
{
    // @TODO refactor backing memory to use spans from a common large pool
    // instead of 128byte chunks for each name.
    struct NameIDProvider
    {
        NameIDProvider() : m_names(1024u, 3u)
        {
            NameID::SetProvider(this);
            StringToID("NULL_ID");
        }

        uint32_t StringToID(const char* name)
        {
            FixedString128 fixed(name);
            return m_names.Add(fixed);
        }

        const char* IDToString(const uint32_t& name)
        {
            if (name >= m_names.GetCount())
            {
                FixedString128 fixedMessage("Trying to get a string using an invalid id: %u", name);
                Memory::Assert(false, fixedMessage.c_str());
            }

            return m_names[name].c_str();
        }

        HashSet<FixedString128> m_names;
    };

    uint32_t NameID::NameIDProvider_StringToID(const char* name) 
    { 
        return s_Provider->StringToID(name); 
    }

    const char* NameID::NameIDProvider_IDToString(const uint32_t& name) 
    { 
        return s_Provider->IDToString(name); 
    }
}