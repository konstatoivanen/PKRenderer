#pragma once
namespace PK::Utilities
{
    class NoCopy
    {
        public:
            NoCopy(NoCopy const&) = delete;
            NoCopy& operator=(NoCopy const&) = delete;
            NoCopy() = default;
    };
}
