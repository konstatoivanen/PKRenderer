#pragma once
#include "Core/Yaml/Serialize.h"
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
            Serialize::LoadStruct(filepath, static_cast<T*>(this));
        };
    };
}
