#pragma once
#include "Core/ECS/EGID.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    // Check for static OnDeserialize
    template <typename TEntity, typename TImplementers>
    concept TEntityHasOnDeserialize = requires(EntityDatabase* db, const EGID& egid, SerialNodeRead node, TImplementers& impls) 
    {
        TEntity::OnDeserialize(db, egid, node, impls);
    };

    // Check for static OnSerialize
    template <typename TEntity>
    concept TEntityHasOnSerialize = requires(EntityDatabase* db, const EGID& egid, SerialNodeWrite node) 
    {
        TEntity::OnSerialize(db, egid, node);
    };

    // Check for static OnCreate
    template <typename TEntity, typename TImplementers>
    concept TEntityHasOnCreate = requires(EntityDatabase* db, const EGID& egid, const TEntity& desc, TImplementers& impls) 
    {
        TEntity::OnCreate(db, egid, desc, impls);
    };

    template<typename TEntity>
    struct EntityFactory
    {
        using TImplementers = typename TEntity::TImplementers;
        using TViews = typename TEntity::TViews;
        constexpr static const bool IsSerializable = TEntity::IsSerializable;

        static void OnDeserialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeRead node, TImplementers& implementers) = delete;
        static void OnSerialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeWrite node) = delete;
        static void OnCreate(EntityDatabase* entityDb, const EGID& egid, const TEntity& descriptor, TImplementers& implementers) = delete;

        static TImplementers Instantiate(EntityDatabase* entityDb, const EGID& egid)
        {
            auto implementers = TImplementers::Dispatch([&]<typename... Args>()
            {
                return Sequence::Copy(entityDb->NewImplementer<TRemovePtr_T<Args>>()...);
            });

            TViews::Dispatch([&]<typename... Args>()
            {
                Sequence::Dispatch([&](auto&... implementer)
                {
                    (entityDb->NewView<Args>(egid, implementer...), ...);
                },
                implementers);
            });

            return implementers;
        }
        
        static EGID Deserialize(EntityDatabase* entityDb, SerialNodeRead node, uint32_t group)
        {
            /*
            if constexpr (IsSerializable)
            {
                auto egid = entityDb->ReserveEntityId(group);
                auto implementers = Instantiate(entityDb, egid);
                
                Sequence::For([node](auto implementer)
                {
                    using TImplementer = TRemoveCVRef_T<TRemovePtr_T<decltype(implementer)>>;
                    using TBases = decltype(pk_base_list<TImplementer>);

                    TBases::For([node]<typename TComponent>()
                    {
                        if constexpr (!PK::TIsSame<TComponent, IEntityImplementer>)
                        {
                            auto& component = *static_cast<TComponent*>(implementer);

                            ReflectFields(component, [node](const char* name, auto& value)
                            {
                                // Deserialize your fields using 'node' here
                            });
                        }
                    });
                }, 
                implementers);

                if constexpr (TEntityHasOnDeserialize<TEntity, TImplementers>)
                {
                    TEntity::OnDeserialize(entityDb, egid, node, implementers);
                }
                
                return egid;
            }
            */

            return EGIDInvalid;
        }

        static void Serialize(EntityDatabase* entityDb, SerialNodeWrite node, const EGID& egid)
        {
            if constexpr (IsSerializable)
            {
                if constexpr (TEntityHasOnSerialize<TEntity>)
                {
                    TEntity::OnSerialize(entityDb, egid, node);
                }
            }
        }

        static EGID Create(EntityDatabase* entityDb, EGID egid, const TEntity& descriptor)
        {
            if (egid.entityID() == 0u)
            {
                egid = entityDb->ReserveEntityId(egid.groupID());
            }

            auto implementers = Instantiate(entityDb, egid);

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
