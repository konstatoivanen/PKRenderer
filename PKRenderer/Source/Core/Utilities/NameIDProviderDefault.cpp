#include "PrecompiledHeader.h"
#include "NameIDProviderDefault.h"

namespace PK
{
    // Should not be here but I dont want to make another cpp file.
    INameIDProvider::~INameIDProvider() = default;

    uint32_t NameIDProviderDefault::INameIDProvider_StringToID(const char* str)
    {
        FixedString128 fixed(str);
        return m_names.Add(fixed);
    }

    const char* NameIDProviderDefault::INameIDProvider_IDToString(const uint32_t& id)
    {
        if (id >= m_names.GetCount())
        {
            throw std::invalid_argument("Trying to get a string using an invalid id: " + std::to_string(id));
        }

        return m_names[id].c_str();
    }
}