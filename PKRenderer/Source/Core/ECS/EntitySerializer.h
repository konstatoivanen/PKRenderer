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

    template<typename TEntity>
    requires TEntityIsSerializable<TEntity>
    struct EntitySerializer
    {
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
            auto node = parent.append_child();
            node.set_map();
            node.save_key(name.c_str());
            node["TypeUUID"].save(uuid64.c_str(), ryml::VAL_PLAIN);
            Serialize::WriteSingle(node["SerializationFlags"], &entity.serializable->flags);

            PK::ReflectFields(entity, [&node](const char* componentName, auto& component)
            {
                using TComponent = TRemovePtrCVRef_T<decltype(component)>;

                if constexpr (TIsClass<TComponent> && !TIsSame<TComponent, ComponentSerializable>)
                {
                    auto isPrivate = false;

                    PK::ReflectFields(*component, [&isPrivate, &componentName, &node](const char* name, const auto& field)
                    {
                        if ((isPrivate |= TIsSame<TRemoveCVRef_T<decltype(field)>, NotSerialized>, !isPrivate))
                        {
                            FixedString256 namepath("%s.%s", componentName, name);
                            auto fieldNode = node.append_child();
                            fieldNode.save_key(namepath.c_str());
                            Serialize::WriteSingle(fieldNode, &field);
                        }
                    });
                }
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
                // WHAT?!?
                return;
            }

            PK::ReflectFields(entity, [&entityNode](const char* componentName, auto& component)
            {
                using TComponent = TRemovePtrCVRef_T<decltype(component)>;

                if constexpr (TIsClass<TComponent> && !TIsSame<TComponent, ComponentSerializable>)
                {
                    auto isPrivate = false;

                    PK::ReflectFields(*component, [&isPrivate, &componentName, &entityNode](const char* name, auto& field)
                    {
                        if ((isPrivate |= TIsSame<TRemoveCVRef_T<decltype(field)>, NotSerialized>, !isPrivate))
                        {
                            FixedString256 namepath("%s.%s", componentName, name);
                            Serialize::ReadSingle(entityNode[namepath.c_str()], &field);
                        }
                    });
                }
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
