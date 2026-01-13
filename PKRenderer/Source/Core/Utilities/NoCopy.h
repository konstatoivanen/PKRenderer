#pragma once
namespace PK
{
    struct NoCopy
    {
        NoCopy(NoCopy const&) = delete;
        NoCopy& operator=(NoCopy const&) = delete;
        NoCopy() = default;
    };
}
