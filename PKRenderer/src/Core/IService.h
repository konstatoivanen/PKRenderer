#pragma once
#include "Core/NoCopy.h"

namespace PK::Core
{
    class IService : public NoCopy
    {
        public: virtual ~IService() = 0 {};
    };
}