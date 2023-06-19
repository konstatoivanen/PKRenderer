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
        
        void BeginWrite(Structs::QueueType queue, uint32_t instanceLimit) final;
        void AddInstance(Mesh* mesh, uint32_t submesh, uint32_t customIndex, const PK::Math::float4x4& matrix) final;
        void EndWrite() final;

        uint32_t GetInstanceCount() const final { return m_instanceCount; }
        uint32_t GetSubStructureCount() const final { return m_substructures.GetCount(); };

        inline const VulkanBindHandle* GetBindHandle() const { return &m_bindHandle; };

        private:
            struct MeshKey
            {
                Mesh* mesh;
                uint64_t submesh;

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

            constexpr static uint32_t MAX_SUBSTRUCTURES = 1024u;

            struct BLAS
            {
                VulkanRawAccelerationStructure* raw = nullptr;
                MeshKey key;
                VkAccelerationStructureGeometryKHR geometry{};
                VkAccelerationStructureBuildRangeInfoKHR range{};
                VkAccelerationStructureBuildSizesInfoKHR size{};
                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
                VkDeviceSize bufferOffset = 0ull;
                VkDeviceSize scratchOffset = 0ull;
                int32_t queryIndex = -1;
                bool isCompacted = false;
            };

            struct TLAS
            {
                VulkanRawAccelerationStructure* raw = nullptr;
                VkAccelerationStructureGeometryKHR geometry{};
                VkAccelerationStructureBuildRangeInfoKHR range{};
                VkAccelerationStructureBuildSizesInfoKHR size{};
                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
                VkDeviceSize bufferOffset = 0ull;
                VkDeviceSize scratchOffset = 0ull;
                bool needsRealloc = false;
            };

            uint64_t GetMeshStructureIndex(Mesh* mesh, uint32_t submeshIndex);
            void ValidateResources();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "AccelerationStructure";

            Objects::VulkanCommandBuffer* m_cmd = nullptr;
            VulkanRawBuffer* m_instanceInputBuffer = nullptr;
            VulkanRawBuffer* m_scratchBuffer = nullptr;
            VulkanRawBuffer* m_structureBuffer = nullptr;
            VulkanQueryPool* m_queryPool = nullptr;
            TLAS m_structure{};
            PK::Utilities::FastMap<MeshKey, BLAS, MeshKeyHash> m_substructures;
            VulkanBindHandle m_bindHandle{};
            uint32_t m_instanceCount = 0u;
            uint32_t m_instanceLimit = 0u;
            VkDeviceSize m_instanceBufferOffset = 0ull;
            VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
            VkDeviceSize m_queryResults[MAX_SUBSTRUCTURES]{};
    };
}