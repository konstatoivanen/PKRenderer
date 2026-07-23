#pragma once
#include "Core/Utilities/HashMap.h"
#include "Core/Utilities/TypeIndex.h"
#include "Core/Utilities/FixedString.h"
#include "Core/ECS/EntityFactory.h"

namespace PK::App
{
    struct EntitySerializer
    {
        typedef EGID (*FuncDeserialize)(EntityDatabase*, const YAML::ConstNode&, uint32_t);
        typedef void (*FuncSerialize)(EntityDatabase*, YAML::Node&, const EGID&);

        UUID128 uuid;
        FixedString32 name;
        uint32_t typeIndex;
        FuncDeserialize deserialize;
        FuncSerialize serialize;

        template<typename TEntity>
        static EntitySerializer GetSerializer()
        {
            constexpr auto name = pk_base_type_name<TEntity>();
            EntitySerializer serializer;
            serializer.name = FixedString32(name.count, name.data);
            serializer.uuid = Hash::MurmurHash128(name.data, name.count);
            serializer.typeIndex = pk_base_type_index<TEntity>();
            serializer.deserialize = EntityFactory<TEntity>::Deserialize;
            serializer.serialize = EntityFactory<TEntity>::Serialize;
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
        void DerializeEntities(const char* path, uint32_t group);

        EntityDatabase* m_entityDb;
        HashMap<UUID128, EntitySerializer, EntitySerializer::SerializerHash> m_serializers;
    };
}