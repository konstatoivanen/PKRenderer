#pragma once
#include "Core/NoCopy.h"
#include "Rendering/Structs/Enums.h"
#include "Utilities/Ref.h"

namespace PK::Rendering
{
    using namespace Structs;
    using namespace Utilities;

    struct GraphicsDriver : public PK::Core::NoCopy
    {
        virtual ~GraphicsDriver() = default;
        virtual APIType GetAPI() const = 0;

        static Scope<GraphicsDriver> Create(APIType api);
    };

    namespace GraphicsAPI
    {
        GraphicsDriver* GetActiveDriver();
        
        template<typename T>
        inline T* GetActiveDriver() { return static_cast<T*>(GetActiveDriver()); }
        APIType GetActiveAPI();
    }
}