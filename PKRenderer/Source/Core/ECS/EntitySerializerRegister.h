#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/TypeMeta.h"
#include "Core/ECS/EntityFactory.h"

namespace PK::App
{
    struct EntitySerializerRegister
    {
        EntitySerializerRegister(EntityDatabase* entityDb, const initializer_list<EntitySerializer>& serializers);

        void SerializeEntities(const char* path);
        void DeserializeEntities(const char* path);

        EntityDatabase* m_entityDb;
        HashMap<UUID128, EntitySerializer, EntitySerializer::SerializerHash> m_serializers;
    };
}