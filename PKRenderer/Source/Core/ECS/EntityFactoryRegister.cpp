#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Base/FileIO.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntitySerializable.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Serialization/Serialize.h"
#include "EntityFactoryRegister.h"

namespace PK::App
{
    EntityFactoryRegister::EntityFactoryRegister(EntityDatabase* entityDb, const initializer_list<EntitySerializer>& serializers) :
        m_entityDb(entityDb),
        m_serializers((uint32_t)serializers.size(), 3u)
    {
        for (auto& serializer : serializers)
        {
            m_serializers.AddValue(serializer.uuid, serializer);
        }

        CVariableRegister::Create<CVariableFunc>("Entities.Save", [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                SerializeEntities(args[0], (uint32_t)ENTITY_GROUPS::ACTIVE);
            }, "Expected a filepath argument", 1u);

        CVariableRegister::Create<CVariableFunc>("Entities.Load", [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                DeserializeEntities(args[0], (uint32_t)ENTITY_GROUPS::ACTIVE);
            }, "Expected a filepath argument", 1u);
    }
    
    void EntityFactoryRegister::SerializeEntities([[maybe_unused]] const char* path, uint32_t group)
    {
        auto views = m_entityDb->Query<EntityViewSerializable>(group);

        ryml::Tree tree = ryml::Tree();
        tree.reserve(views.count * 16u);
        tree.reserve_arena(8192ull);

        ryml::NodeRef root = tree.rootref();
        root |= ryml::MAP; 
        
        auto entities = root["Entities"];
        entities |= ryml::MAP;

        for (auto i = 0u; i < views.count; ++i)
        {
            const auto& view = views[i];
            auto serializer = m_serializers.GetValuePtr(view.typeUUID);

            if (serializer)
            {
                auto name = view.name;

                // Default to typename + index if no user defined value was set.
                if (!name.Length())
                {
                    name = FixedString64("%s%u", serializer->name, i);
                }

                // Copy byte data to string as yaml << operator doesn't support substr?
                FixedString32 uuidstr(16u, serializer->uuid.bytes);

                auto entity = entities.append_child();
                entity |= ryml::MAP;
                entity.set_key(name.c_str());
                entity["Type"] << uuidstr.c_str() |= ryml::VAL_PLAIN;

                serializer->serialize(m_entityDb, entity, view.GID);
            }
        }

        ryml::emit_yaml(tree, tree.root_id(), stdout);
    }
    
    void EntityFactoryRegister::DeserializeEntities(const char* path, uint32_t group)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(path, false, &fileData, &fileSize) != 0)
        {
            PK_LOG_WARNING("Failed to read Scene file at path '%'", path);
            return;
        }

        auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        auto root = tree.rootref();
        auto entities = root.find_child("Entities");

        if (entities.readable())
        {
            for (auto entity : entities.children())
            {
                auto type = entity.find_child("Type");

                if (type.readable())
                {
                    auto uuidStr = Serialize::ReadVal<FixedString32>(type);
                    auto name = Serialize::ReadKey<FixedString32>(entity);
                    auto uuid = Memory::BitCast<FixedString32, UUID128>(&uuidStr);
                    auto serializer = m_serializers.GetValuePtr(uuid);

                    if (serializer)
                    {
                        serializer->deserialize(m_entityDb, entity, group, name);
                    }

                }
            }
        }

        Memory::Free(fileData);
    }
}
