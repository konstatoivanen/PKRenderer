#include "PrecompiledHeader.h"
#include "Core/Input/InputKeyBinding.h"
#include <rapidyaml/ryaml.h>
#include "Core/Yaml/RapidyamlFwd.h"

namespace PK::YAML
{
    template<>
    bool Read<CommandInputKeyBindingMap>(const ConstNode& node, CommandInputKeyBindingMap* rhs)
    {
        for (auto const ch : node.children())
        {
            std::string name;
            std::string key;
            ryml::from_chars(ch[0].val(), &name);
            ryml::from_chars(ch[1].val(), &key);
            (*rhs)[name] = PK::StringToInputKey(key.c_str());
        }

        return true;
    }

    template<>
    bool Read<InputKeyCommandBindings>(const ConstNode& node, InputKeyCommandBindings* rhs)
    {
        if (node.num_children() == 0)
        {
            return true;
        }

        auto& bindings = *rhs;
        bindings.count = 0u;
        bindings.bindings.Validate(node.num_children());

        for (auto const ch : node.children())
        {
            auto key = ch[0].val();
            auto name = ch[1].val();
            FixedString16 keystr(key.len, key.data());
            bindings.bindings[bindings.count++] = { PK::StringToInputKey(keystr),  FixedString128(name.len, name.data()) };
        }

        return true;
    }

    PK_YAML_DECLARE_READ_MEMBER(CommandInputKeyBindingMap)
    PK_YAML_DECLARE_READ_MEMBER(InputKeyCommandBindings)
}