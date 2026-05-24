#pragma once
#include "Core/Utilities/HashMap.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanAccelerationStructure : public RHIAccelerationStructure
    {
        constexpr const static uint32_t COMPACTED_ID = ~0u;
        
        struct StructureKey
        {
            void* value0;
            uint64_t value1;

            constexpr bool operator == (const StructureKey& other) const
            {
                return value0 == other.value0 && value1 == other.value1;
            }
        };

        struct Structure
        {
            VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
            VkDeviceAddress deviceAddress = 0ull;
            VkAccelerationStructureGeometryKHR geometry{};
            VkAccelerationStructureBuildRangeInfoKHR range{};
            VkAccelerationStructureBuildSizesInfoKHR size{};
            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
            VkDeviceSize bufferOffset = 0ull;
            VkDeviceSize scratchOffset = 0ull;
            uint32_t compactionId = 0u;
            NameID name = 0u;
        };

        VulkanAccelerationStructure(struct VulkanDriver* driver, const char* name);
        ~VulkanAccelerationStructure();
        
        void BeginWrite(QueueType queue, uint32_t instanceLimit) final;
        void AddInstance(const RayTracingGeometryInfo& geometry, const float3x4& matrix) final;
        void EndWrite() final;
        
        uint32_t GetInstanceCount() const final { return m_instanceCount; }
        uint32_t GetSubStructureCount() const final { return m_substructures.GetCount(); };
        FenceRef GetLastBuildFenceRef() const final { return m_lastBuildFenceRef; }

        inline const VulkanBindHandle* GetBindHandle() const { return &m_bindHandle; };
    
    private:
        void DisposeVkAccelerationStructureKHR(VkAccelerationStructureKHR handle, const FenceRef& fence) const;
        VkAccelerationStructureKHR CreateVkAccelerationStructureKHR(const Structure* structure, VkAccelerationStructureTypeKHR type, const char* name) const;

        const VulkanDriver* m_driver = nullptr;
        const FixedString128 m_name;

        VulkanRawBuffer* m_instanceInputBuffer = nullptr;
        VulkanRawBuffer* m_scratchBuffer = nullptr;
        VulkanRawBuffer* m_structureBuffer = nullptr;
        FixedUnique<VulkanQueryPool> m_queryPool;
        HashMap<StructureKey, Structure> m_substructures;
        Structure m_structure{};
        VulkanBindHandle m_bindHandle{};
        
        // Temporaries used during build process
        uint32_t m_instanceCount = 0u;
        uint32_t m_instanceLimit = 0u;
        uint64_t m_topologyHashPrev = 0u;
        uint64_t m_topologyHashCurr = 0u;
        VkDeviceSize m_instanceBufferOffset = 0ull;

        //@TODO This shouldnt be here. replace begin end with cmd injection
        struct VulkanCommandBuffer* m_cmd = nullptr;
        FenceRef m_lastBuildFenceRef = {};
        VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
    };
}
