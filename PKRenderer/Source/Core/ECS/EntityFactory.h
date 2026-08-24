#pragma once
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntitySerializable.h"
#include "Core/ECS/NotSerialized.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template <typename TEntity>
    concept TEntityHasDescriptor = requires 
    {
        typename TEntity::Descriptor;
    };

    template <typename TEntity>
    concept TEntityHasOnSerialize = requires(EntityDatabase* db, uint32_t entityId, SerialNodeWrite node)
    {
        TEntity::OnSerialize(db, entityId, node);
    };

    template <typename TEntity>
    concept TEntityHasOnDeserialize = requires(EntityDatabase* db, SerialNodeRead node, TEntity& entity)
    {
        TEntity::OnDeserialize(db, node, entity);
    };

    template <typename TEntity>
    concept TEntityHasOnCreate2 = TEntityHasDescriptor<TEntity> && 
    requires(EntityDatabase* db, TEntity& entity, const typename TEntity::Descriptor& desc)
    {
        TEntity::OnCreate(db, entity, desc);
    };

    template <typename TEntity>
    concept TEntityIsSerializable = TEntityHasDescriptor<TEntity> && requires(typename TEntity::Descriptor instance)
    {
        TEntity::serializable;
        instance.entityName;
        instance.entitySerialize;
    };

    template<typename TEntity>
    struct EntityFactory
    {
        constexpr static const auto TypeName = pk_full_type_name<TEntity>;
        constexpr static const UUID128 UUID = pk_type_uuid128<TEntity>;

        constexpr static EntitySerializer GetSerializer() { return { UUID, TypeName.str, Serialize, Deserialize };}

        static TEntity Instantiate(EntityDatabase* entityDb, const char* serializableName)
        {
            auto entity = entityDb->New<TEntity>();

            if constexpr (TEntityIsSerializable<TEntity>)
            {
                if (serializableName)
                {
                    entity.serializable->name = serializableName;
                    entity.serializable->typeUUID = UUID;
                }
            }

            return entity;
        }

        static void Serialize(EntityDatabase* entityDb, SerialNodeWrite node, const uint32_t entityId)
        {
            if constexpr (!TEntityIsSerializable<TEntity>)
            {
                return;
            }

            auto entity = entityDb->Query<TEntity>(entityId);

            PK::ReflectFields(entity, [&node](auto& component)
            {
                using TComponent = TRemovePtrCVRef_T<decltype(component)>;

                if constexpr (TIsClass<TComponent>)
                {
                    auto isPrivate = false;

                    PK::ReflectFields(*component, [&isPrivate, &node](const char* name, const auto& field)
                    {
                        if ((isPrivate |= TIsSame<TRemoveCVRef_T<decltype(field)>, NotSerialized>, !isPrivate))
                        {
                            Serialize::WriteSingle(node[name], &field);
                        }
                    });
                }
            });

            if constexpr (TEntityHasOnSerialize<TEntity>)
            {
                TEntity::OnSerialize(entityDb, entityId, node);
            }
        }
        
        static uint32_t Deserialize(EntityDatabase* entityDb, SerialNodeRead node, const char* name)
        {
            if constexpr (!TEntityIsSerializable<TEntity>)
            {
                return 0u;
            }

            auto entity = Instantiate(entityDb, name);
            
            PK::ReflectFields(entity, [&node](auto& component)
            {
                using TComponent = TRemovePtrCVRef_T<decltype(component)>;

                if constexpr (TIsClass<TComponent>)
                {
                    auto isPrivate = false;

                    PK::ReflectFields(*component, [&isPrivate, &node](const char* name, auto& field)
                    {
                        if ((isPrivate |= TIsSame<TRemoveCVRef_T<decltype(field)>, NotSerialized>, !isPrivate))
                        {
                            Serialize::ReadSingle(node[name], &field);
                        }
                    });
                }
            });

            if constexpr (TEntityHasOnDeserialize<TEntity>)
            {
                TEntity::OnDeserialize(entityDb, node, entity);
            }
            
            return *entity.entityId;
        }

        static TEntity Create(EntityDatabase* entityDb, const TEntity::Descriptor& descriptor) requires TEntityHasDescriptor<TEntity>
        {
            const char* name = nullptr;

            if constexpr (TEntityIsSerializable<TEntity>)
            {
                name = descriptor.entitySerialize ? descriptor.entityName.c_str() : nullptr;
            }

            auto entity = Instantiate(entityDb, name);

            if constexpr (TEntityHasOnCreate2<TEntity>)
            {
                TEntity::OnCreate(entityDb, entity, descriptor);
            }

            return entity;
        }
    };
}
