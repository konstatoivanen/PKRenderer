#pragma once
#include "Utilities/IndexedSet.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanEnumConversion.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    struct VulkanAccelerationStructure : public Rendering::RHI::Objects::AccelerationStructure
    {
        VulkanAccelerationStructure(const char* name);
        ~VulkanAccelerationStructure();
        
        void BeginWrite(QueueType queue, uint32_t instanceLimit) final;
        void AddInstance(AccelerationStructureGeometryInfo& geometry, const PK::Math::float4x4& matrix) final;
        void EndWrite() final;

        uint32_t GetInstanceCount() const final { return m_instanceCount; }
        uint32_t GetSubStructureCount() const final { return m_substructures.GetCount(); };

        inline const VulkanBindHandle* GetBindHandle() const { return &m_bindHandle; };

        private:
            struct alignas(16) GeometryKey
            {
                // These are very unlikely to be the same for two geometries.
                // Or at least it doesn't make sense that they would be.
                // Good enough key.
                Buffer* vertexBuffer;
                uint64_t offsetCount;

                constexpr bool operator == (const GeometryKey& other) const
                {
                    return vertexBuffer == other.vertexBuffer && offsetCount == other.offsetCount;
                }
            };

            using GeometryKeyHash = PK::Utilities::HashHelpers::TFNV1AHash<GeometryKey>;
            constexpr static uint32_t MAX_SUBSTRUCTURES = 1024u;

            struct BLAS
            {
                uint32_t nameHashId;
                VulkanRawAccelerationStructure* raw = nullptr;
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

            uint64_t GetGeometryIndex(const AccelerationStructureGeometryInfo& geometry);
            void ValidateResources();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "AccelerationStructure";

            Objects::VulkanCommandBuffer* m_cmd = nullptr;
            VulkanRawBuffer* m_instanceInputBuffer = nullptr;
            VulkanRawBuffer* m_scratchBuffer = nullptr;
            VulkanRawBuffer* m_structureBuffer = nullptr;
            VulkanQueryPool* m_queryPool = nullptr;
            TLAS m_structure{};
            PK::Utilities::FastMap<GeometryKey, BLAS, GeometryKeyHash> m_substructures;
            VulkanBindHandle m_bindHandle{};
            uint32_t m_instanceCount = 0u;
            uint32_t m_instanceLimit = 0u;
            VkDeviceSize m_instanceBufferOffset = 0ull;
            VkAccelerationStructureInstanceKHR* m_writeBuffer = nullptr;
            VkDeviceSize m_queryResults[MAX_SUBSTRUCTURES]{};
    };
}