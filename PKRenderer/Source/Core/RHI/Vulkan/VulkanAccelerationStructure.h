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
        FenceRef GetLastBuildFenceRef() const final { return m_lastBuildFenceRef; }

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
            FixedString128 m_name;

            VulkanRawBuffer* m_instanceInputBuffer = nullptr;
            VulkanRawBuffer* m_scratchBuffer = nullptr;
            VulkanRawBuffer* m_structureBuffer = nullptr;
            VulkanQueryPool* m_queryPool = nullptr;
            TLAS m_structure{};
            FastMap<BLASKey, BLAS, GeometryKeyHash> m_substructures;
            VulkanBindHandle m_bindHandle{};
            
            // Temporaries used during build process
            uint32_t m_instanceCount = 0u;
            uint32_t m_instanceLimit = 0u;
            uint64_t m_structureHashPrev = 0u;
            uint64_t m_structureHashCurr = 0u;
            VkDeviceSize m_instanceBufferOffset = 0ull;

            //@TODO This shouldnt be here. replace begin end with cmd injection
            VulkanCommandBuffer* m_cmd = nullptr;
            FenceRef m_lastBuildFenceRef = {};
            VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
    };
}