#pragma once
#include "Core/ECS/EGID.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ECS/EntitySerializable.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template <typename TEntity>
    concept TEntityHasOnSerialize = requires(EntityDatabase* db, const EGID& egid, SerialNodeWrite node) 
    {
        TEntity::OnSerialize(db, egid, node);
    };

    template <typename TEntity, typename TImplementers>
    concept TEntityHasOnDeserialize = requires(EntityDatabase* db, const EGID& egid, SerialNodeRead node, TImplementers& impls) 
    {
        TEntity::OnDeserialize(db, egid, node, impls);
    };

    template <typename TEntity, typename TImplementers>
    concept TEntityHasOnCreate = requires(EntityDatabase* db, const EGID& egid, const TEntity& desc, TImplementers& impls) 
    {
        TEntity::OnCreate(db, egid, desc, impls);
    };

    template <typename TEntity>
    concept TEntityIsSerializable = requires(TEntity instance)  
    {
        instance.entityName;
        instance.entitySerialize;
    };

    template <typename... T> struct DumpType;

    template<typename TEntity>
    struct EntityFactory
    {
        using TImplementers = typename TEntity::TImplementers;
        using TViews = typename TEntity::TViews;
        
        constexpr static const auto TypeName = pk_outer_type_name<TEntity>;
        constexpr static const UUID128 UUID = Hash::MurmurHash128(TypeName.str, TypeName.length);
        
        static TImplementers Instantiate(EntityDatabase* entityDb, const EGID& egid, const char* serializableName)
        {
            auto implementers = TImplementers::Dispatch([&]<typename... Args>()
            {
                return Sequence::Make(entityDb->NewImplementer<TRemovePtr_T<Args>>()...);
            });

            TViews::Dispatch([&]<typename... Args>()
            {
                Sequence::Dispatch([&](auto&... implementer)
                {
                    (entityDb->NewView<Args>(egid, implementer...), ...);
                },
                implementers);
            });

            if constexpr (TEntityIsSerializable<TEntity>)
            {
                if (serializableName)
                {
                    auto view = entityDb->NewView<EntityViewSerializable>(egid);
                    view->name = serializableName;
                    view->typeUUID = UUID;
                }
            }

            return implementers;
        }

        static void Serialize(EntityDatabase* entityDb, SerialNodeWrite node, const EGID& egid)
        {
            if constexpr (!TEntityIsSerializable<TEntity>)
            {
                return;
            }

            TViews::For([entityDb, &node, egid]<typename TView>()
            {
                auto view = entityDb->Query<TView>(egid);

                PK::ReflectFields(*static_cast<TView*>(view), [&node](auto& value)
                {
                    using TComponentRef = PK::TRemoveCVRef_T<decltype(value)>;

                    if constexpr (TIsSpecialization<TComponentRef, EntityComponentRef>)
                    {
                        PK::ReflectFields(*value.pointer, [&node](const char* name, const auto& field)
                        {
                            Serialize::WriteSingle(node[name], &field);
                        });
                    }
                });
            });

            if constexpr (TEntityHasOnSerialize<TEntity>)
            {
                TEntity::OnSerialize(entityDb, egid, node);
            }
        }
        
        static EGID Deserialize(EntityDatabase* entityDb, SerialNodeRead node, uint32_t group, const char* name)
        {
            if constexpr (!TEntityIsSerializable<TEntity>)
            {
                return EGIDInvalid;
            }

            auto egid = entityDb->ReserveEntityId(group);
            auto implementers = Instantiate(entityDb, egid, name);
            
            Sequence::For([node](auto&& implementer)
            {
                using TImplementer = TRemovePtr_T<TRemoveCVRef_T<decltype(implementer)>>;
                TImplementer::TComponents::For([node, &implementer]<typename TComponent>()
                {
                    Serialize::ReadVal(node, static_cast<TComponent*>(implementer));
                });
            }, 
            implementers);

            if constexpr (TEntityHasOnDeserialize<TEntity, TImplementers>)
            {
                TEntity::OnDeserialize(entityDb, egid, node, implementers);
            }
            
            return egid;
        }

        static EGID Create(EntityDatabase* entityDb, EGID egid, const TEntity& descriptor)
        {
            if (egid.entityID() == 0u)
            {
                egid = entityDb->ReserveEntityId(egid.groupID());
            }

            const char* name = nullptr;

            if constexpr (TEntityIsSerializable<TEntity>)
            {
                name = descriptor.entitySerialize ? descriptor.entityName : nullptr;
            }

            auto implementers = Instantiate(entityDb, egid, name);

            if constexpr (TEntityHasOnCreate<TEntity, TImplementers>)
            {
                TEntity::OnCreate(entityDb, egid, descriptor, implementers);
            }

            return egid;
        }

        inline static EGID Create(EntityDatabase* entityDb, uint32_t groupId, const TEntity& descriptor)
        {
            return Create(entityDb, EGID(0u, groupId), descriptor);
        }
    };
}
