#include "PrecompiledHeader.h"
#include <exception>
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
            FixedString128 fixedMessage("Trying to get a string using an invalid id: %u", id);
            throw std::exception(fixedMessage.c_str());
        }

        return m_names[id].c_str();
    }
}