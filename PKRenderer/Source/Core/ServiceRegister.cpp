#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "ServiceRegister.h"

namespace PK
{
    ServiceRegister::Service::~Service() = default;

    void ServiceRegister::AssertTypeExists([[maybe_unused]] bool exists, std::type_index index)
    {
        PK_LOG_INFO("ServiceRegister.Create: %s", index.name());
        PK_DEBUG_THROW_ASSERT(exists, "Service of type (%s) is already registered!", index.name());
    }
}
