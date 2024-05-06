#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/NameID.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Structs.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Buffer)

namespace PK::Rendering::RHI::Objects
{
    struct alignas(16) AccelerationStructureGeometryInfo
    {
        Utilities::NameID name;
        Buffer* vertexBuffer;
        Buffer* indexBuffer;
        uint32_t vertexOffset;
        uint32_t vertexStride;
        uint32_t vertexFirst;
        uint32_t vertexCount;
        uint32_t indexStride;
        uint32_t indexFirst;
        uint32_t indexCount;
        uint32_t customIndex;
    };

    typedef Utilities::Ref<class AccelerationStructure> AccelerationStructureRef;

    class AccelerationStructure : public Utilities::NoCopy, public Utilities::NativeInterface<AccelerationStructure>
    {
    public:
        static AccelerationStructureRef Create(const char* name);

        virtual void BeginWrite(QueueType queue, uint32_t instanceLimit) = 0;
        virtual void AddInstance(AccelerationStructureGeometryInfo& geometry, const PK::Math::float3x4& matrix) = 0;
        virtual void EndWrite() = 0;

        virtual uint32_t GetInstanceCount() const = 0;
        virtual uint32_t GetSubStructureCount() const = 0;

        virtual ~AccelerationStructure() = default;
    };
}