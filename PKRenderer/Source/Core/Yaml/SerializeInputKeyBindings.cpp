#include "PrecompiledHeader.h"
#include "Core/Yaml/Serialize.h"
#include "Core/Input/InputKeyBinding.h"

namespace PK::Serialize
{
    template<>
    void Read<CommandInputKeyBindingMap>(const ConstNode& node, CommandInputKeyBindingMap* rhs)
    {
        auto& map = *rhs;
        map.Reserve((uint32_t)node.num_children());

        for (auto const ch : node.children())
        {
            auto key = ch[1].val();
            auto name = ch[0].val();
            FixedString16 keystr(key.len, key.data());
            FixedString32 namestr(name.len, name.data());
            map.AddValue(namestr, PK::StringToInputKey(keystr));
        }
    }

    template<>
    void Read<InputKeyCommandBindings>(const ConstNode& node, InputKeyCommandBindings* rhs)
    {
        if (node.num_children() == 0)
        {
            return;
        }

        auto& keyCommands = *rhs;
        size_t size = sizeof(InputKeyCommand) * node.num_children();

        for (auto const ch : node.children())
        {
            size += ch[1].val().len + 1u;
        }

        keyCommands.memory.Reserve(size, false);
        keyCommands.count = 0u;
        auto bindings = keyCommands.GetBindings();
        auto head = keyCommands.memory.GetData() + sizeof(InputKeyCommand) * node.num_children();

        for (auto const ch : node.children())
        {
            auto key = ch[0].val();
            auto name = ch[1].val();
            FixedString16 keystr(key.len, key.data());
            bindings[keyCommands.count++] = { head, PK::StringToInputKey(keystr) };
        
            strncpy(head, name.data(), name.len);
            head[name.len] = '\0';
            head += name.len + 1u;
        }
    }
}
