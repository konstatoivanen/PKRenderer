#include "PrecompiledHeader.h"
#include "Core/Serialization/Serialize.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/CVariablesYaml.h"

namespace PK
{
    static void AddArg(char* buff, const char** args, ryml::csubstr substr, size_t& head, uint32_t& count)
    {
        if (head + substr.len + 1u >= 512)
        {
            return;
        }

        args[count++] = buff + head;
        memcpy(buff + head, substr.data(), substr.len);
        buff[head + substr.len] = '\0';
        head += substr.len + 1ull;
    }

    void ISerializer<CVariablesYaml>::ReadVal(SerialNodeRead node, [[maybe_unused]] CVariablesYaml* rhs)
    {
        char buffer[512];
        const char* arguments[32];

        for (const auto child : node.children())
        {
            auto head = 0ull;
            auto count = 0u;
            AddArg(buffer, arguments, child.key(), head, count);

            if (child.is_seq())
            {
                for (const auto elem : child.children())
                {
                    AddArg(buffer, arguments, elem.val(), head, count);
                }
            }
            else if (child.readable())
            {
                AddArg(buffer, arguments, child.val(), head, count);
            }

            PK::CVariableRegister::Execute(arguments, count);
        }
    }
}
