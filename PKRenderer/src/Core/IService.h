#pragma once
#include "Utilities/NoCopy.h"

namespace PK::Core
{
    class IService : public PK::Utilities::NoCopy
    {
    public: virtual ~IService() = 0 {};
    };
}