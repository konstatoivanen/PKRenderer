#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIO.h"
#include "Core/Yaml/RapidyamlPrivate.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntitySerializable.h"
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
    }
    
    void EntityFactoryRegister::SerializeEntities([[maybe_unused]] const char* path, uint32_t group)
    {
        ryml::Tree tree;
        ryml::NodeRef root = tree.rootref();
        root |= ryml::MAP; 
        
        auto entities = root["Entities"];
        entities |= ryml::MAP;

        auto views = m_entityDb->Query<EntityViewSerializable>(group);

        for (auto i = 0u; i < views.count; ++i)
        {
            const auto& view = views[i];
            auto serializer = m_serializers.GetValuePtr(view.serial->typeUUID);

            if (serializer)
            {
                auto name = view.serial->name;

                // Default to typename + index if no user defined value was set.
                if (!name.Length())
                {
                    name = FixedString64("%s%u", serializer->name.c_str(), i);
                }

                // Copy byte data to string as yaml << operator doesn't support substr?
                FixedString32 uuidstr(16u, serializer->uuid.bytes);

                auto entity = entities[name.c_str()];
                entity |= ryml::MAP;
                entity["Type"] << uuidstr.c_str() |= ryml::VAL_DQUO;

                serializer->serialize(m_entityDb, entity, view.GID);
            }
        }

        ryml::emit_yaml(tree, tree.root_id(), stdout);
    }
    
    void EntityFactoryRegister::DerializeEntities(const char* path, uint32_t group)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(path, false, &fileData, &fileSize) != 0)
        {
            PK_LOG_WARNING("Failed to read Scene file at path '%'", path);
            return;
        }

        auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        YAML::ConstNode root = tree.rootref();

        auto entities = root.find_child("Entities");

        if (entities.readable())
        {
            for (auto entity : entities.children())
            {
                auto type = entity.find_child("Type");

                if (type.readable())
                {
                    auto uuidStr = YAML::Read<FixedString32>(type);
                    auto name = YAML::ReadKey<FixedString32>(entity);
                    auto uuid = Memory::BitCast<FixedString32, UUID128>(&uuidStr);

                    auto serializer = m_serializers.GetValuePtr(uuid);

                    if (serializer)
                    {
                        serializer->deserialize(m_entityDb, entity, group);
                    }

                }
            }
        }

        Memory::Free(fileData);
    }
}
