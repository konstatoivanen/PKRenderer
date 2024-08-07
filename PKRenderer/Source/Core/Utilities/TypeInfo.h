#pragma once
#include <string>
#include <typeinfo>
#include <typeindex>

namespace PK
{
    std::string GetTypeShortName(const std::type_index& typeIndex);
    std::string GetTypeNameSpace(const std::type_index& typeIndex);
    std::string GetTypeShortName(const std::type_info& typeInfo);
    std::string GetTypeNameSpace(const std::type_info& typeInfo);
    
    template<typename T>
    std::string GetTypeShortName() { return GetTypeShortName(typeid(T)); }

    template<typename T>
    std::string GetTypeNameSpace() { return GetTypeNameSpace(typeid(T)); }

    template<typename T>
    std::type_index GetTypeIndex() { return std::type_index(typeid(T)); }
}