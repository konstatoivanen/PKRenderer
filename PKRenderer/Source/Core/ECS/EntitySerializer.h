#pragma once
#include "Core/Serialization/Serialize.h"
#include "Core/ECS/NotSerialized.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntityComponentSerializable.h"

namespace PK
{
    template <typename TEntity>
    concept TEntityIsSerializable = requires 
    {
        TEntity::serializable; 
        requires TIsSame<decltype(TEntity::serializable), ComponentSerializable*>;
    };

    template <typename TEntity>
    concept TEntityHasOnSerialize = requires(EntityDatabase * db, TEntity & entity, SerialNodeWrite & node)
    {
        TEntity::OnSerialize(db, entity, node);
    };

    template <typename TEntity>
    concept TEntityHasOnDeserialize = requires(EntityDatabase * db, TEntity & entity, SerialNodeRead & node)
    {
        TEntity::OnDeserialize(db, entity, node);
    };

    template<typename TEntity>
    requires TEntityIsSerializable<TEntity>
    struct EntitySerializer
    {
        template<typename TFunc>
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
                            FixedString256 namepath("%s.%s", componentName, name);
                            func(namepath.c_str(), field);
                        }
                    });
                }
            });
        }

        static void Serialize(EntityDatabase* entityDb, uint64_t compositionUUID, uint32_t* entityId, SerialNodeWrite* parentref)
        {
            auto entity = entityDb->Query<TEntity>(*entityId);

            if ((entity.serializable->flags & EntitySerialFlags::Serialize) == 0)
            {
                return;
            }

            auto& parent = *parentref;
            auto name = entity.serializable->name;

            // Default to typename + index if no user defined value was set.
            if (!name.Length())
            {
                constexpr auto typeName = pk_outer_type_name<TEntity>();
                name = FixedString64("%s_%u", typeName, *entityId);
            }

            if (parent.has_child(name.c_str()))
            {
                name = FixedString64("%s_%u", name.c_str(), *entityId);
            }

            auto uuid64 = String::Base64Encode(compositionUUID);
            auto entityNode = parent.append_child();
            entityNode.set_map();
            entityNode.save_key(name.c_str());
            entityNode["CompositionUUID"].save(uuid64.c_str(), ryml::VAL_PLAIN);
            Serialize::WriteSingle(entityNode["SerializationFlags"], &entity.serializable->flags);

            if constexpr (TEntityHasOnSerialize<TEntity>)
            {
                TEntity::OnSerialize(entityDb, entity, entityNode);
                return;
            }

            Reflect(entity, [&entityNode](const char* path, auto& field)
            {
                auto fieldNode = entityNode.append_child();
                fieldNode.save_key(path);
                Serialize::WriteSingle(fieldNode, &field);
            });
        }

        static void Deserialize(EntityDatabase* entityDb, [[maybe_unused]] uint64_t compositionUUID, uint32_t* entityId, SerialNodeRead* entityNodeRef)
        {
            auto& entityNode = *entityNodeRef;
            auto entity = entityDb->New<TEntity>();
            *entityId = *entity.entityId;
            entity.serializable->name = Serialize::ReadKey<FixedString64>(entityNode);
            entity.serializable->flags = Serialize::ReadVal<EntitySerialFlags>(entityNode["SerializationFlags"]);

            if ((entity.serializable->flags & EntitySerialFlags::Serialize) == 0)
            {
                return;
            }

            if constexpr (TEntityHasOnDeserialize<TEntity>)
            {
                TEntity::OnDeserialize(entityDb, entity, entityNode);
                return;
            }

            Reflect(entity, [&entityNode](const char* path, auto& field)
            {
                Serialize::ReadSingle(entityNode[path], &field);
            });
        }
    };

    struct EntityDatabaseSerializer
    {
        EntityDatabaseSerializer(EntityDatabase* entityDb);
        void Serialize(const char* path);
        void Deserialize(const char* path);
        EntityDatabase* m_entityDb;
    };
}
