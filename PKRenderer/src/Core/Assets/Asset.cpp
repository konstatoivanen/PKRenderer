#include "PrecompiledHeader.h"
#include "Core/Services/StringHashID.h"
#include "Asset.h"

namespace PK::Core::Assets
{
    using namespace PK::Core::Services;

    AssetID Asset::NameToId(const std::string& name) 
    {
        return StringHashID::StringToID(name);
    }

    AssetID Asset::NameToId(const char* name) 
    { 
        return StringHashID::StringToID(name); 
    }

    const std::string& Asset::IdToName(AssetID id) 
    { 
        return StringHashID::IDToString(id); 
    }
}