#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;
    struct VulkanCommandBuffer;

    struct VulkanAccelerationStructure : public RHIAccelerationStructure
    {
        VulkanAccelerationStructure(const char* name);
        ~VulkanAccelerationStructure();
        
        void BeginWrite(QueueType queue, uint32_t instanceLimit) final;
        void AddInstance(AccelerationStructureGeometryInfo& geometry, const float3x4& matrix) final;
        void EndWrite() final;

        uint32_t GetInstanceCount() const final { return m_instanceCount; }
        uint32_t GetSubStructureCount() const final { return m_substructures.GetCount(); };

        inline const VulkanBindHandle* GetBindHandle() const { return &m_bindHandle; };

        private:
            struct BLASKey
            {
                void* value0;
                uint64_t value1;

                constexpr bool operator == (const BLASKey& other) const
                {
                    return value0 == other.value0 && value1 == other.value1;
                }
            };

            using GeometryKeyHash = Hash::TFNV1AHash<BLASKey>;
            constexpr static uint32_t MAX_SUBSTRUCTURES = 1024u;

            struct BLAS
            {
                VulkanRawAccelerationStructure* raw = nullptr;
                VkAccelerationStructureGeometryKHR geometry{};
                VkAccelerationStructureBuildRangeInfoKHR range{};
                VkAccelerationStructureBuildSizesInfoKHR size{};
                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
                VkDeviceSize bufferOffset = 0ull;
                VkDeviceSize scratchOffset = 0ull;
                int32_t queryIndex = -1;
                NameID name = 0u;
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

            uint64_t GetGeometryIndex(const AccelerationStructureGeometryInfo& geometry);
            void ValidateResources();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "AccelerationStructure";

            //@TODO This shouldnt be here. replace begin end with cmd injection
            VulkanCommandBuffer* m_cmd = nullptr;
            VulkanRawBuffer* m_instanceInputBuffer = nullptr;
            VulkanRawBuffer* m_scratchBuffer = nullptr;
            VulkanRawBuffer* m_structureBuffer = nullptr;
            VulkanQueryPool* m_queryPool = nullptr;
            TLAS m_structure{};
            FastMap<BLASKey, BLAS, GeometryKeyHash> m_substructures;
            VulkanBindHandle m_bindHandle{};
            uint32_t m_instanceCount = 0u;
            uint32_t m_instanceLimit = 0u;
            VkDeviceSize m_instanceBufferOffset = 0ull;
            VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
            VkDeviceSize m_queryResults[MAX_SUBSTRUCTURES]{};
    };
}