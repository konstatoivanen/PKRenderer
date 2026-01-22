#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "ServiceRegister.h"

namespace PK
{
    ServiceRegister::Service::~Service() = default;

    void ServiceRegister::AssertTypeExists([[maybe_unused]] bool exists, const ConstBufferView<char>& name)
    {
        PK_LOG_INFO("ServiceRegister.Create: %.*s", name.count, name.data);
        PK_DEBUG_THROW_ASSERT(exists, "Service of type (%.*s) is already registered!", name.count, name.data);
    }
}
