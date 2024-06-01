#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "ServiceRegister.h"

namespace PK
{
    ServiceRegister::Service::~Service() = default;

    void ServiceRegister::AssertTypeExists(std::type_index index)
    {
        PK_LOG_INFO("ServiceRegister.Create: %s", index.name());
        PK_THROW_ASSERT(m_services.count(index) == 0, "Service of type (%s) is already registered!", index.name());
    }
}
