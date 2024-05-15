#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/NameID.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Structs.h"

namespace PK::Rendering::RHI::Objects
{
    typedef Utilities::Ref<struct AccelerationStructure> AccelerationStructureRef;

    struct AccelerationStructure : public Utilities::NoCopy, public Utilities::NativeInterface<AccelerationStructure>
    {
        virtual ~AccelerationStructure() = 0;

        virtual void BeginWrite(QueueType queue, uint32_t instanceLimit) = 0;
        virtual void AddInstance(AccelerationStructureGeometryInfo& geometry, const PK::Math::float3x4& matrix) = 0;
        virtual void EndWrite() = 0;

        virtual uint32_t GetInstanceCount() const = 0;
        virtual uint32_t GetSubStructureCount() const = 0;
    };
}