#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Structs.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Buffer)

namespace PK::Rendering::RHI::Objects
{
    struct alignas(16) AccelerationStructureGeometryInfo
    {
        uint32_t vertexOffset;
        uint32_t firstVertex;
        uint32_t vertexCount;
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t customIndex;
        uint32_t nameHashId;
        Buffer* vertexBuffer;
        Buffer* indexBuffer;
    };

    typedef Utilities::Ref<class AccelerationStructure> AccelerationStructureRef;

    class AccelerationStructure : public Utilities::NoCopy, public Utilities::NativeInterface<AccelerationStructure>
    {
    public:
        static AccelerationStructureRef Create(const char* name);

        virtual void BeginWrite(QueueType queue, uint32_t instanceLimit) = 0;
        virtual void AddInstance(AccelerationStructureGeometryInfo& geometry, const PK::Math::float4x4& matrix) = 0;
        virtual void EndWrite() = 0;

        virtual uint32_t GetInstanceCount() const = 0;
        virtual uint32_t GetSubStructureCount() const = 0;

        virtual ~AccelerationStructure() = default;
    };
}