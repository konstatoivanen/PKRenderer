#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Utilities/Ref.h"
#include "Rendering/Objects/Mesh.h"

namespace PK::Rendering::Objects
{
    class AccelerationStructure : public Utilities::NoCopy, public Utilities::NativeInterface<AccelerationStructure>
    {
        public:
            static Utilities::Ref<AccelerationStructure> Create(const char* name);

            virtual void BeginWrite(uint32_t instanceLimit) = 0;
            virtual void AddInstance(Mesh* mesh, uint32_t submesh, uint32_t customIndex, const PK::Math::float4x4& matrix) = 0;
            virtual void EndWrite() = 0;

            virtual uint32_t GetInstanceCount() const = 0;
            virtual uint32_t GetSubStructureCount() const = 0;

            virtual ~AccelerationStructure() = default;
    };
}