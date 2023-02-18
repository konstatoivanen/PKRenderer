#pragma once
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/Objects/AccelerationStructure.h"
#include "Utilities/IndexedSet.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    struct VulkanAccelerationStructure : public Rendering::Objects::AccelerationStructure
    {
        VulkanAccelerationStructure(const char* name);
        ~VulkanAccelerationStructure();
        void Dispose(const FenceRef& fence);
        
        void BeginWrite(uint32_t instanceLimit) override final;
        void AddInstance(Mesh* mesh, uint32_t submesh, uint32_t customIndex, const PK::Math::float4x4& matrix) override final;
        void EndWrite() override final;

        uint32_t GetInstanceCount() const override final { return m_instanceCount; }
        uint32_t GetSubStructureCount() const override final { return m_subStructures.GetCount(); };

        inline const VulkanBindHandle* GetBindHandle() const { return &m_bindHandle; };

        private:
            struct MeshKey
            {
                Mesh* mesh;
                uint32_t submesh;

                constexpr bool operator < (const MeshKey& other) const
                {
                    return mesh != other.mesh ? mesh < other.mesh : submesh < other.submesh;
                }

                constexpr bool operator == (const MeshKey& other) const
                {
                    return mesh == other.mesh && submesh == other.submesh;
                }
            };

            struct MeshKeyHash
            {
                size_t operator()(const MeshKey& k) const noexcept
                {
                    return PK::Utilities::HashHelpers::FNV1AHash(&k, sizeof(MeshKey));
                }
            };

            VulkanRawBuffer* GetScratchBuffer(size_t size);
            VulkanRawBuffer* GetInstanceBuffer(size_t size);
            VulkanRawAccelerationStructure* GetMeshStructure(Mesh* mesh, uint32_t submeshIndex);

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "AccelerationStructure";

            VulkanRawBuffer* m_instanceInputBuffer = nullptr;
            VulkanRawBuffer* m_scratchBuffer = nullptr;
            VulkanRawAccelerationStructure* m_structure = nullptr;
            PK::Utilities::PointerMap<MeshKey, VulkanRawAccelerationStructure, MeshKeyHash> m_subStructures;
            VulkanBindHandle m_bindHandle{};
            uint32_t m_instanceCount = 0u;
            uint32_t m_instanceLimit = 0u;
            uint32_t m_previousSubStructureCount = 0u;
            VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
    };
}