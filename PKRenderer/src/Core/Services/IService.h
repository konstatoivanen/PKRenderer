#pragma once
#include "Utilities/NoCopy.h"

namespace PK::Core::Services
{
    class IService : public PK::Utilities::NoCopy
    {
        public: virtual ~IService() = 0 {};
    };
}