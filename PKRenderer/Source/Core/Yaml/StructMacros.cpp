#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIO.h"
#include "Core/Yaml/RapidyamlPrivate.h"
#include "StructMacros.h"

namespace PK
{
    void IYamlStruct::YamlLoadFromFile(const char* filepath)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;
        
        if (FileIO::ReadBinary(filepath, false, &fileData, &fileSize) != 0)
        {
            PK_LOG_WARNING("Failed to read IYamlStruct at path '%'", filepath);
            return;
        }

        auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        YAML::ConstNode root = tree.rootref();

        ParseStruct(root);

        Memory::Free(fileData);
    }

    void IYamlStruct::ParseMemberStruct(const YAML::ConstNode& node, const char* memberName, IYamlStruct* outValue)
    {
        auto member = node.find_child(memberName);
        if (member.readable())
        {
            outValue->ParseStruct(member);
        }
    }
}
