#include "PrecompiledHeader.h"
#include "TypeInfo.h"

namespace PK
{
    std::string GetTypeShortName(const std::type_index& typeIndex)
    {
        auto name = std::string(typeIndex.name());
        auto namespaceEnd = name.find_last_of(":");
        return name.substr(namespaceEnd + 1);
    }

    std::string GetTypeNameSpace(const std::type_index& typeIndex)
    {
        auto name = std::string(typeIndex.name());
        auto namespaceEnd = name.find_last_of(':');
        auto keywordEnd = name.find_last_of(' ') + 1;
        return name.substr(keywordEnd, namespaceEnd - keywordEnd);
    }

    std::string GetTypeShortName(const std::type_info& typeInfo)
    {
        auto name = std::string(typeInfo.name());
        auto namespaceEnd = name.find_last_of(":");
        return name.substr(namespaceEnd + 1);
    }

    std::string GetTypeNameSpace(const std::type_info& typeInfo)
    {
        auto name = std::string(typeInfo.name());
        auto namespaceEnd = name.find_last_of(':');
        auto keywordEnd = name.find_last_of(' ') + 1;
        return name.substr(keywordEnd, namespaceEnd - keywordEnd);
    }
}