#include "PrecompiledHeader.h"
#include "PlatformInterfaces.h"

namespace PK
{
    IPlatformWindowListener::~IPlatformWindowListener() = default;
    PlatformWindow::~PlatformWindow() = default;
    void IPlatform::PollEvents() { Platform::PollEvents(false); }
    void IPlatform::WaitEvents() { Platform::PollEvents(true); }
}
