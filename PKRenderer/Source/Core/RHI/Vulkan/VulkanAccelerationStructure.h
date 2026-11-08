#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"

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
            VkDeviceSize size = 0ull;
            VkDeviceSize buildScratchSize = 0ull;
            VkDeviceSize scratchOffset = 0ull;
            VkDeviceSize bufferOffset = 0ull;
            uint32_t compactionId = 0u;
            NameID name = 0u;
            VulkanRaytracingGeometry geometry{};
        };

        VulkanAccelerationStructure(struct VulkanDriver* driver, const char* name);
        ~VulkanAccelerationStructure();
        
        uint32_t GetInstanceCount() const final { return instanceCount; }
        uint32_t GetSubStructureCount() const final { return substructures.GetCount(); };
        inline const VulkanResourceView GetResourceView() const { return view; };

        uint32_t RegisterGeometry(const RayTracingGeometryInfo& geometry);

        bool PollCompactionResults();
        uint32_t AcquireCompactionQueries(VkAccelerationStructureKHR* outHandles, uint32_t* outQueryStart, const FenceRef& fence);

        VkAccelerationStructureKHR CreateStructure(const Structure* structure, VkAccelerationStructureTypeKHR type, const char* name) const;
        void DisposeStructure(VkAccelerationStructureKHR handle, const FenceRef& fence) const;

        const VulkanDriver* driver = nullptr;
        const FixedString128 name;

        RHIBufferRef buffer = nullptr;
        FixedUnique<VulkanQueryPool> queryPool;
        HashMap<StructureKey, Structure> substructures;
        Structure structure{};
        VulkanResourceView view{};
        
        uint32_t queryCount = 0u;
        uint32_t instanceCount = 0u;
        uint64_t topologyHash = 0u;
    };

    struct VulkanAccelerationStructureBuilder : public RHIAccelerationStructureBuilder
    {
        VulkanAccelerationStructure* structure;
        RHIBuffer* stagingBuffer;
        VkAccelerationStructureInstanceKHR* instances = nullptr;
        uint32_t* instanceIndices;
        uint32_t instanceCount = 0u;
        uint32_t instanceLimit = 0u;
        uint64_t topologyHash = 0u;

        void AddInstance(const RayTracingGeometryInfo& geometry, const float3x4& matrix) final;
    };
}
