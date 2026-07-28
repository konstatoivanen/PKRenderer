#pragma once
#include "Core/Serialization/Serialize.h"
#include "Core/Assets/Asset.h"

namespace PK
{
    template<typename T>
    struct Config : public PK::Asset, public T
    {
        using Type = T;

        Config() {};
        Config(const char* filepath)
        {
            Serialize::Load(filepath, static_cast<T*>(this));
        };
    };

    template<typename T>
    struct AssetTraits<Config<T>>
    {
        constexpr static const char* Extension = "*.cfg";
    };
}
