#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Base/FileIO.h"
#include "Core/CLI/CVariableRegister.h"
#include "EntityArchive.h"

namespace PK
{    
    EntityArchive::EntityArchive(const char* filepath)
    {
        LoadTree(filepath);
    }

    EntityArchive::~EntityArchive()
    {
        ReleaseTree();
    }

    bool EntityArchive::SaveScene(EntityDatabase* entityDb)
    {
        ReleaseTree();

        auto views = entityDb->Query<EntityViewSerializable>();

        auto tree = ryml::Tree();
        tree.reserve(views.count() * 16ull);
        tree.reserve_arena(8192ull);

        auto root = tree.rootref();
        root.set_map();

        EntityArchiveWrite archive;
        archive.archive = this;
        archive.node = &root;
        archive.name = nullptr;
        archive.sceneId = GetAssetID();
        archive.prefabId = 0u;

        auto isScene = true;
        Serialize::WriteSingle(root[FieldName_IsScene], &isScene);

        auto entities = root[FieldName_Entities];
        entities.set_map();

        for (auto& view : views)
        {
            if (view.serializable->sceneId == archive.sceneId)
            {
                auto entityNode = entities.append_child();
                entityNode.set_map();
                archive.node = &entityNode;
                archive.name = view.serializable->name.c_str();
                archive.prefabId = view.serializable->prefabId;
                entityDb->Visit<EntityArchiveWrite>(*view.entityId, &archive);
            }
        }
        
        auto file = FileIO::OpenWrite(GetFileName(), true);

        if (file)
        {
            ryml::emit_yaml(tree, tree.root_id(), static_cast<FILE*>(file));
            FileIO::CloseFile(file);
            return true;
        }

        return false;
    }

    bool EntityArchive::SavePrefab(EntityDatabase* entityDb, uint32_t entityId)
    {
        ReleaseTree();

        auto view = entityDb->Query<EntityViewSerializable>(entityId);

        if (view.entityId == nullptr)
        {
            return false;
        }

        auto tree = ryml::Tree();
        tree.reserve(16ull);
        tree.reserve_arena(8192ull);

        auto root = tree.rootref();
        root.set_map();
        
        auto isScene = false;
        Serialize::WriteSingle(root[FieldName_IsScene], &isScene);

        auto entities = root[FieldName_Entities];
        entities.set_map();

        auto entityNode = entities.append_child();
        entityNode.set_map();

        EntityArchiveWrite archive;
        archive.archive = this;
        archive.node = &root;
        archive.name = GetFileName();
        archive.sceneId = GetAssetID();
        archive.prefabId = 0u;
        archive.node = &entityNode;

        view.serializable->prefabId = archive.sceneId;

        entityDb->Visit<EntityArchiveWrite>(*view.entityId, &archive);

        FILE* file = fopen(archive.name, "w");

        if (file)
        {
            ryml::emit_yaml(tree, tree.root_id(), file);
            fclose(file);
            return true;
        }

        return false;
    }

    bool EntityArchive::Instantiate(EntityDatabase* entityDb, AssetID sceneId)
    {
        if (!LoadTree(GetFileName()))
        {
            PK_LOG_WARNING("Failed to read EntityArchive file at path '%s'", GetFileName());
            return false;
        }

        Serialize::ReadSingle(m_root[FieldName_IsScene], &m_isScene);

        EntityArchiveRead archive;
        archive.archive = this;
        archive.node = &m_root;
        archive.name = GetFileName();
        archive.sceneId = m_isScene ? GetAssetID() : sceneId;
        archive.prefabId = m_isScene ? AssetID() : sceneId;

        const auto entities = archive.node->find_child(FieldName_Entities);

        if (entities.readable())
        {
            for (auto entity : entities.children())
            {
                const auto nodeUUID = entity.find_child(FieldName_UUID);

                if (nodeUUID.readable())
                {
                    const auto nodePrefab = entity.find_child(FieldName_Prefab);
                    
                    if (nodePrefab.readable())
                    {
                        archive.prefabId = AssetID(Serialize::ReadVal<FixedString128>(nodePrefab).c_str());
                    }
                    
                    auto entityName = Serialize::ReadKey<FixedString64>(entity);
                    auto uuidStr = Serialize::ReadVal<FixedString32>(nodeUUID);
                    auto uuid = String::Base64Decode<uint64_t>(uuidStr.c_str());

                    archive.node = &entity;
                    archive.name = entityName.c_str();

                    entityDb->VisitType<EntityArchiveRead>(uuid, &archive);
                }
            }
        }
        
        return true;
    }

    bool EntityArchive::DeleteScene(EntityDatabase* entityDb)
    {
        if (m_isScene)
        {
            auto views = entityDb->Query<EntityViewSerializable>();

            HeapList<uint32_t> entityIds;
            entityIds.Reserve(views.count(), false);

            for (auto& view : views)
            {
                if (view.serializable->sceneId == GetAssetID())
                {
                    entityIds.Add(*view.entityId);
                }
            }

            for (auto id : entityIds)
            {
                entityDb->Delete(id);
            }

            return entityIds.GetCount() != 0;
        }

        return false;
    }


    void EntityArchive::WriteEntityHeader(EntityArchiveWrite* archive, uint32_t entityId, const uint64_t uuid, const char* typeName)
    {
        FixedString64 name = archive->name;

        // Default to typename + index if no user defined value was set.
        if (!name.Length())
        {
            name = FixedString64("%s_%u", typeName, entityId);
        }

        if (archive->node->parent().has_child(name.c_str()))
        {
            name = FixedString64("%s_%u", name.c_str(), entityId);
        }

        archive->node->save_key(name.c_str());

        archive->name = archive->node->key().data();

        auto uuid64 = String::Base64Encode(uuid);
        auto nodeUUID = (*archive->node)[FieldName_UUID];
        nodeUUID.save(uuid64.c_str(), ryml::VAL_PLAIN);
    }
    
    bool EntityArchive::LoadTree(const char* filepath)
    {
        if (m_data != nullptr)
        {
            return true;
        }

        ReleaseTree();

        if (FileIO::Read(filepath, false, &m_data, &m_size) != 0)
        {
            return false;
        }

        m_tree = ryml::parse_in_place(c4::substr(static_cast<char*>(m_data), m_size));
        m_root = m_tree.rootref();
        return true;
    }
    
    void EntityArchive::ReleaseTree()
    {
        Memory::Free(m_data);
        m_data = nullptr;
        m_size = 0ull;
        m_tree = ryml::Tree();
        m_root = ryml::ConstNodeRef();
    }
}
