#include "PrecompiledHeader.h"
#include "Core/Serialization/Serialize.h"
#include "Core/Input/InputKeyBindings.h"
#include "Core/Input/InputKeyCommands.h"

namespace PK
{
    void ISerializer<InputShortcut>::ReadVal(SerialNodeRead node, InputShortcut* rhs)
    {
        const auto child_count = node.num_children();

        *rhs = {};

        if (child_count == 0)
        {
            ISerializer<InputKey>::ReadVal(node, &rhs->First);
        }
        else
        {
            if (child_count >= 1u) ISerializer<InputKey>::ReadVal(node[0], &rhs->First);
            if (child_count >= 2u) ISerializer<InputKey>::ReadVal(node[1], &rhs->Second);
        }
    }

    void ISerializer<InputShortcut>::WriteVal(SerialNodeWrite node, InputShortcut const* rhs)
    {
        if (rhs->Second != InputKey::None)
        {
            node.set_seq(ryml::FLOW_SL);
            node.clear_children();
            ISerializer<InputKey>::WriteVal(node.append_child(), &rhs->First);
            ISerializer<InputKey>::WriteVal(node.append_child(), &rhs->Second);
        }
        else
        {
            node.clear_children();
            node.clear_val();
            ISerializer<InputKey>::WriteVal(node, &rhs->First);
        }
    }

    void ISerializer<InputTriplet>::ReadVal(SerialNodeRead node, InputTriplet* rhs)
    {
        const auto child_count = node.num_children();
        
        *rhs = {};

        if (child_count == 0 || node.is_flow_sl())
        {
            ISerializer<InputShortcut>::ReadVal(node, &rhs->Primary);
        }
        else
        {
            if (child_count >= 1u) ISerializer<InputShortcut>::ReadVal(node[0], &rhs->Primary);
            if (child_count >= 2u) ISerializer<InputShortcut>::ReadVal(node[1], &rhs->Secondary);
            if (child_count >= 3u) ISerializer<InputShortcut>::ReadVal(node[2], &rhs->Tertiary);
        }
    }

    void ISerializer<InputTriplet>::WriteVal(SerialNodeWrite node, InputTriplet const* rhs)
    {
        if (rhs->Secondary.First != InputKey::None)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            if (rhs->Primary.First != InputKey::None) ISerializer<InputShortcut>::WriteVal(node.append_child(), &rhs->Primary);
            if (rhs->Secondary.First != InputKey::None) ISerializer<InputShortcut>::WriteVal(node.append_child(), &rhs->Secondary);
            if (rhs->Tertiary.First != InputKey::None) ISerializer<InputShortcut>::WriteVal(node.append_child(), &rhs->Tertiary);
        }
        else
        {
            node.clear_children();
            node.clear_val();
            ISerializer<InputShortcut>::WriteVal(node, &rhs->Primary);
        }
    }

    void ISerializer<InputKeyBindings>::ReadVal(SerialNodeRead node, InputKeyBindings* rhs)
    {
        auto& map = *rhs;
        map.Reserve((uint32_t)node.num_children());

        for (auto const child : node.children())
        {
            FixedString32 name(child.key().len, child.key().data());
            
            InputTriplet key{};
            ISerializer<InputTriplet>::ReadVal(child, &key);

            map.AddValue(name, key);
        }
    }

    void ISerializer<InputKeyBindings>::WriteVal(SerialNodeWrite node, InputKeyBindings const* rhs)
    {
        node.set_map();
        node.clear_children();

        for (auto i = 0u; i < rhs->GetCount(); ++i)
        {
            auto child = node.append_child();
            child.save_key((*rhs)[i].key.c_str());
            ISerializer<InputTriplet>::WriteVal(child, &(*rhs)[i].value);
        }
    }

    void ISerializer<InputKeyCommands>::ReadVal(SerialNodeRead node, InputKeyCommands* rhs)
    {
        const auto commandCount = node.num_children();

        if (commandCount)
        {
            auto stringLength = 0ull;

            for (auto const child : node.children())
            {
                auto command = child.find_child("Command");

                if (command.readable())
                {
                    stringLength += command.val().len + 1ull;
                }
            }

            rhs->Reserve(commandCount, stringLength);

            for (auto const child : node.children())
            {
                auto key = child.find_child("Key");
                auto command = child.find_child("Command");

                if (key.readable() && command.readable())
                {
                    InputTriplet triplet{};
                    ISerializer<InputTriplet>::ReadVal(key, &triplet);
                    rhs->Add(triplet, command.val().data(), command.val().len);
                }
            }
        }
    }

    void ISerializer<InputKeyCommands>::WriteVal(SerialNodeWrite node, InputKeyCommands const* rhs)
    {
        node.set_map();
        node.clear_children();

        for (const auto& command : *rhs)
        {
            auto child = node.append_child();
            child.set_map();
            ISerializer<InputTriplet>::WriteVal(child["Key"], &command.key);
            child["Command"].save(command.command, ryml::VAL_DQUO);
        }
    }
}
