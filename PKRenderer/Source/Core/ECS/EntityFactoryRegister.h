#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/TypeMeta.h"
#include "Core/ECS/EntityFactory.h"

namespace PK::App
{
    struct EntitySerializer
    {
        typedef void (*FuncSerialize)(EntityDatabase*, SerialNodeWrite, const EGID&);
        typedef EGID (*FuncDeserialize)(EntityDatabase*, SerialNodeRead, uint32_t, const char*);

        UUID128 uuid;
        const char* name;
        uint32_t typeIndex;
        FuncSerialize serialize;
        FuncDeserialize deserialize;

        template<typename TEntity>
        static EntitySerializer Get()
        {
            EntitySerializer serializer;
            serializer.name = EntityFactory<TEntity>::TypeName.str;
            serializer.uuid = EntityFactory<TEntity>::UUID;
            serializer.typeIndex = pk_base_type_index<TEntity>();
            serializer.serialize = EntityFactory<TEntity>::Serialize;
            serializer.deserialize = EntityFactory<TEntity>::Deserialize;
            return serializer;
        }

        constexpr bool operator == (const EntitySerializer& r) const noexcept
        {
            return uuid == r.uuid;
        }

        struct SerializerHash 
        { 
            constexpr size_t operator()(const UUID128& k) const noexcept
            { 
                return k.low;
            } 
        };
    };

    struct EntityFactoryRegister
    {
        EntityFactoryRegister(EntityDatabase* entityDb, const initializer_list<EntitySerializer>& serializers);

        void SerializeEntities(const char* path, uint32_t group);
        void DeserializeEntities(const char* path, uint32_t group);

        EntityDatabase* m_entityDb;
        HashMap<UUID128, EntitySerializer, EntitySerializer::SerializerHash> m_serializers;
    };
}