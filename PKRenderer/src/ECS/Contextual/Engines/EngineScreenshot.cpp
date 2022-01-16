#include "PrecompiledHeader.h"
#include "EngineScreenshot.h"

namespace PK::ECS::Engines
{
    EngineScreenshot::EngineScreenshot() : m_frameData(1)
    {
        /*
        MemoryBlock<char> m_frameData;
        Ref<Buffer> m_copyBuffer;
        */
    }
    
    void EngineScreenshot::Step(Window* window)
    {
    }
}