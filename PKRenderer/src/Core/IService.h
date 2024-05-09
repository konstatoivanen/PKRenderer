#pragma once
#include "Utilities/NoCopy.h"

namespace PK::Core
{
    struct IService : public PK::Utilities::NoCopy
    {
        virtual ~IService() = 0;
    };
}