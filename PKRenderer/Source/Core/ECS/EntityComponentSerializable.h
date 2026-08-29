#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Assets/Asset.h"

namespace PK
{
    struct EntityArchiveWrite;
    struct EntityArchiveRead;

    struct ComponentSerializable
    {
        FixedString64 name;
        AssetID sceneId;
        AssetID prefabId;
    };

    struct EntityViewSerializable
    {
        uint32_t* entityId;
        ComponentSerializable* serializable;
    };
}
