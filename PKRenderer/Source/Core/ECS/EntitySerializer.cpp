#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Base/FileIO.h"
#include "Core/CLI/CVariableRegister.h"
#include "EntitySerializer.h"

namespace PK
{
    EntityDatabaseSerializer::EntityDatabaseSerializer(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
        CVariableRegister::Create<CVariableFunc>("Engine.Entities.Save", [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                Serialize(args[0]);
            }, "Expected a filepath argument", 1u);

        CVariableRegister::Create<CVariableFunc>("Engine.Entities.Load", [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                Deserialize(args[0]);
            }, "Expected a filepath argument", 1u);
    }
    
    void EntityDatabaseSerializer::Serialize([[maybe_unused]] const char* path)
    {
        auto views = m_entityDb->Query<EntityViewSerializable>();
        auto viewCount = views.count();

        auto tree = ryml::Tree();
        tree.reserve(viewCount * 16u);
        tree.reserve_arena(8192ull);

        auto root = tree.rootref();
        root.set_map();
        
        auto entities = root["Entities"];
        entities.set_map();

        for (auto& view : views)
        {
            if ((view.serializable->flags & EntitySerialFlags::Serialize) != 0u)
            {
                m_entityDb->Visit<SerialNodeWrite>(*view.entityId, &entities);
            }
        }

        ryml::emit_yaml(tree, tree.root_id(), stdout);
    }
    
    void EntityDatabaseSerializer::Deserialize(const char* path)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(path, false, &fileData, &fileSize) != 0)
        {
            PK_LOG_WARNING("Failed to read Scene file at path '%'", path);
            return;
        }

        const auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        const auto root = tree.rootref();
        const auto entities = root.find_child("Entities");

        if (entities.readable())
        {
            for (auto entity : entities.children())
            {
                const auto type = entity.find_child("CompositionUUID");

                if (type.readable())
                {
                    auto uuidStr = Serialize::ReadVal<FixedString32>(type);
                    auto uuid = String::Base64Decode<uint64_t>(uuidStr.c_str());
                    m_entityDb->VisitType<SerialNodeRead>(uuid, &entity);
                }
            }
        }

        Memory::Free(fileData);
    }
}
