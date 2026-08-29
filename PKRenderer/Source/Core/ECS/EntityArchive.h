#pragma once
#include "Core/Serialization/Serialize.h"
#include "Core/ECS/NotSerialized.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntityComponentSerializable.h"

namespace PK
{
    struct EntityArchive;

    struct EntityArchiveWrite
    {
        EntityArchive* archive;
        SerialNodeWrite* node;
        const char* name;
        AssetID sceneId;
        AssetID prefabId;
    };

    struct EntityArchiveRead
    {
        EntityArchive* archive;
        const SerialNodeRead* node;
        const char* name;
        AssetID sceneId;
        AssetID prefabId;
    };

    template <typename TEntity>
    concept TEntityIsSerializable = requires
    {
        TEntity::serializable;
        requires TIsSame<decltype(TEntity::serializable), ComponentSerializable*>;
    };

    template <typename TEntity>
    concept TEntityHasOnSerialize = requires(EntityDatabase* db, TEntity& entity, EntityArchiveWrite* archive)
    {
        TEntity::OnSerialize(db, entity, archive);
    };

    template <typename TEntity>
    concept TEntityHasOnDeserialize = requires(EntityDatabase* db, TEntity& entity, EntityArchiveRead* archive)
    {
        TEntity::OnDeserialize(db, entity, archive);
    };

    // Loading or creating these doesn't actually deserialize the entities
    // It only initializes the archive header which can be used to instantiate or save data through an EntityDatabase instance.
    struct EntityArchive : public Asset
    {
        constexpr static const char* FieldName_IsScene = "IsScene";
        constexpr static const char* FieldName_Entities = "Entities";
        constexpr static const char* FieldName_UUID = "SerialUUID";
        constexpr static const char* FieldName_Prefab = "SerialPrefab";

        EntityArchive(const char* filepath);
        ~EntityArchive();
        
        bool SaveScene(EntityDatabase* entityDb);
        bool SavePrefab(EntityDatabase* entityDb, uint32_t entityId);
        bool Instantiate(EntityDatabase* entityDb, AssetID sceneId = 0u);
        bool DeleteScene(EntityDatabase* entityDb);

        template<typename TEntity>
        requires TEntityIsSerializable<TEntity>
        static void Serialize(EntityDatabase* entityDb, uint32_t* entityId, EntityArchiveWrite* archive)
        {
            auto entity = entityDb->Query<TEntity>(*entityId);
            
            WriteEntityHeader(archive, *entityId, pk_entity_composition_uuid<TEntity>(), pk_outer_type_name<TEntity>());

            if constexpr (TEntityHasOnSerialize<TEntity>)
            {
                TEntity::OnSerialize(entityDb, entity, archive);
                return;
            }

            Reflect(entity, [archive](const char* componentName, const char* fieldName, auto& field)
            {
                FixedString256 namepath("%s.%s", componentName, fieldName);
                auto nodeField = archive->node->append_child();
                nodeField.save_key(namepath.c_str());
                Serialize::WriteSingle(nodeField, &field);
            });
        }
    
        template<typename TEntity>
        requires TEntityIsSerializable<TEntity>
        static void Deserialize(EntityDatabase* entityDb, uint32_t* entityId, EntityArchiveRead* archive)
        {
            auto entity = entityDb->New<TEntity>();
            entity.serializable->name = archive->name;
            entity.serializable->sceneId = archive->sceneId;
            entity.serializable->prefabId = archive->prefabId;
            *entityId = *entity.entityId;

            // @TODO load prefab if valid and override divergent variables from base archive.

            if constexpr (TEntityHasOnDeserialize<TEntity>)
            {
                TEntity::OnDeserialize(entityDb, entity, archive);
                return;
            }

            Reflect(entity, [archive](const char* componentName, const char* fieldName, auto& field)
            {
                FixedString256 namepath("%s.%s", componentName, fieldName);
                Serialize::ReadSingle((*archive->node)[namepath.c_str()], &field);
            });
        }

    private:
        template<typename TEntity, typename TFunc>
        static void Reflect(TEntity& entity, TFunc&& func)
        {
            PK::ReflectFields(entity, [&func](const char* componentName, auto& component)
            {
                using TComponent = TRemovePtrCVRef_T<decltype(component)>;

                if constexpr (TIsClass<TComponent> && !TIsSame<TComponent, ComponentSerializable>)
                {
                    auto isPrivate = false;

                    PK::ReflectFields(*component, [&isPrivate, &componentName, &func](const char* name, auto& field)
                    {
                        if ((isPrivate |= TIsSame<TRemoveCVRef_T<decltype(field)>, NotSerialized>, !isPrivate))
                        {
                            func(componentName, name, field);
                        }
                    });
                }
            });
        }
        
        static void WriteEntityHeader(EntityArchiveWrite* archive, uint32_t entityId, const uint64_t uuid, const char* typeName);

        bool LoadTree(const char* filepath);
        void ReleaseTree();

        ryml::Tree m_tree;
        SerialNodeRead m_root;
        bool m_isScene = false;
        void* m_data = nullptr;
        size_t m_size = 0ull;
    };
}
