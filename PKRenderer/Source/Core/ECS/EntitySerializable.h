#pragma once
#include "Core/Utilities/UUID128.h"
#include "Core/Utilities/FixedString.h"
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"

namespace PK
{
    struct ComponentSerial
    {
        FixedString64 name;
        UUID128 typeUUID;
    };

    struct EntityViewSerializable : public IEntityView
    {
        EntityComponentRef<ComponentSerial> serial;
    };

    struct ImplementerSerialized :
        public IEntityImplementer,
        public ComponentSerial
    {
    };
}
