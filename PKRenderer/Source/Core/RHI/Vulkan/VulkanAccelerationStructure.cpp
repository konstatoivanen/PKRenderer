#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsMatrix.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanAccelerationStructure.h"

namespace PK
{
    VulkanAccelerationStructure::VulkanAccelerationStructure(const char* name) :
        m_driver(RHIDriver::Get()->GetNative<VulkanDriver>()),
        m_name(name),
        m_substructures(MAX_SUBSTRUCTURES)
    {
        m_queryPool = new VulkanQueryPool(m_driver->device, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, MAX_SUBSTRUCTURES);
    }

    VulkanAccelerationStructure::~VulkanAccelerationStructure()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        if (m_queryPool != nullptr)
        {
            delete m_queryPool;
        }

        if (m_structure.raw != nullptr)
        {
            m_driver->disposer->Dispose(m_structure.raw, fence);
        }

        auto substructures = m_substructures.GetValues();

        for (auto i = 0u; i < substructures.count; ++i)
        {
            m_driver->disposer->Dispose(substructures[i].raw, fence);
        }

        m_driver->DisposePooledBuffer(m_instanceInputBuffer, fence);
        m_driver->DisposePooledBuffer(m_scratchBuffer, fence);
        m_driver->DisposePooledBuffer(m_structureBuffer, fence);
    }

    uint64_t VulkanAccelerationStructure::GetGeometryIndex(const AccelerationStructureGeometryInfo& geometry)
    {
        BLASKey key{ geometry.indexBuffer, ((uint64_t)geometry.indexFirst & 0xFFFFFFFFu) | (((uint64_t)geometry.indexCount) << 32ull) };
        uint32_t index = 0u;

        if (!m_substructures.AddKey(key, &index))
        {
            return (uint64_t)index;
        }

        auto structure = &m_substructures.GetValueAt(index);

        *structure = BLAS();
        structure->name = geometry.name;

        auto adressVertex = geometry.vertexBuffer->GetNative<VulkanBuffer>()->GetRaw()->deviceAddress;
        auto adressIndex = geometry.indexBuffer->GetNative<VulkanBuffer>()->GetRaw()->deviceAddress;

        structure->geometry = VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        structure->geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        structure->geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        structure->geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        structure->geometry.geometry.triangles.vertexStride = geometry.vertexStride;
        structure->geometry.geometry.triangles.vertexData.deviceAddress = adressVertex + geometry.vertexOffset;
        structure->geometry.geometry.triangles.maxVertex = geometry.vertexFirst + geometry.vertexCount - 1;
        structure->geometry.geometry.triangles.indexType = geometry.indexStride > 2 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
        structure->geometry.geometry.triangles.indexData.deviceAddress = adressIndex;
        structure->geometry.geometry.triangles.transformData.deviceAddress = 0ull;
        structure->geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        structure->range = { geometry.indexCount / 3 , geometry.indexStride * geometry.indexFirst, geometry.vertexFirst, 0u };

        structure->buildInfo = VkAccelerationStructureBuildGeometryInfoKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        structure->buildInfo.geometryCount = 1u;
        structure->buildInfo.pGeometries = &structure->geometry;
        structure->buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        structure->buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR |
            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;
        structure->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        structure->size = VulkanGetAccelerationBuildSizesInfo(m_driver->device, structure->buildInfo, structure->range.primitiveCount);
        return index;
    }

    void VulkanAccelerationStructure::ValidateResources()
    {
        VkDeviceSize scratchSize = 0ull;
        VkDeviceSize bufferSize = 0ull;
        uint32_t buildCount = 0u;

        // Compaction queries
        {
            for (auto i = 0u; i < m_substructures.GetCount(); ++i)
            {
                auto structure = &m_substructures.GetValueAt(i);

                if (structure->raw && !structure->isCompacted && structure->queryIndex == -1)
                {
                    structure->queryIndex = (uint32_t)m_cmd->QueryAccelerationStructureCompactSize(structure->raw, m_queryPool);
                }
            }
        }

        auto hasCompactedResults = m_queryPool->TryGetResults(m_queryResults, sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

        {
            if (hasCompactedResults)
            {
                PK_LOG_RHI("Bottom Level Compaction Update: %s", m_name.c_str());
            }

            PK_LOG_SCOPE_INDENT(compaction);

            for (auto i = 0u; i < m_substructures.GetCount(); ++i)
            {
                auto structure = &m_substructures.GetValueAt(i);

                if (hasCompactedResults && structure->raw && !structure->isCompacted && structure->queryIndex != -1)
                {
                    auto size0 = structure->size.accelerationStructureSize;
                    auto size1 = m_queryResults[structure->queryIndex];
                    structure->size.accelerationStructureSize = size1;
                    PK_LOG_RHI("BLAS Compacted from %i to %i bytes", size0, size1);
                }

                structure->bufferOffset = bufferSize;
                bufferSize += Math::GetAlignedSize(structure->size.accelerationStructureSize, 256ull);

                if (!structure->raw)
                {
                    ++buildCount;
                    structure->scratchOffset = scratchSize;
                    scratchSize += Math::GetAlignedSize(structure->size.buildScratchSize, 256ull);
                }
            }

            auto prevSize = m_structure.size.accelerationStructureSize;
            m_structure.geometry = VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            m_structure.geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            m_structure.geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            m_structure.geometry.geometry.instances.arrayOfPointers = VK_FALSE;
            m_structure.geometry.geometry.instances.data.deviceAddress = m_instanceInputBuffer->deviceAddress + m_instanceBufferOffset;
            m_structure.geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            m_structure.buildInfo = VkAccelerationStructureBuildGeometryInfoKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            m_structure.buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            m_structure.buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            m_structure.buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            m_structure.buildInfo.geometryCount = 1u;
            m_structure.buildInfo.pGeometries = &m_structure.geometry;
            m_structure.range = { m_instanceCount, 0u, 0u, 0u };
            m_structure.size = VulkanGetAccelerationBuildSizesInfo(m_driver->device, m_structure.buildInfo, m_instanceCount);
            m_structure.needsRealloc = prevSize < m_structure.size.accelerationStructureSize;
            m_structure.bufferOffset = bufferSize;
            m_structure.scratchOffset = scratchSize;
            bufferSize += Math::GetAlignedSize(m_structure.size.accelerationStructureSize, 256ull);
            scratchSize += Math::GetAlignedSize(m_structure.size.buildScratchSize, 256ull);
        }

        if (m_scratchBuffer == nullptr || m_scratchBuffer->size < scratchSize)
        {
            m_driver->DisposePooledBuffer(m_scratchBuffer, m_cmd->GetFenceRef());
            FixedString128 name({ m_name.c_str(),".ScratchBuffer" });
            auto usage = BufferUsage::DefaultStorage | BufferUsage::AccelerationStructure;
            m_scratchBuffer = m_driver->bufferPool.New(m_driver->device, m_driver->allocator, VulkanBufferCreateInfo(usage, scratchSize), name.c_str());
        }

        if (m_structureBuffer != nullptr &&
            m_structureBuffer->size >= bufferSize &&
            !m_structure.needsRealloc &&
            buildCount == 0u &&
            !hasCompactedResults)
        {
            return;
        }

        PK_LOG_RHI("Acceleration Structure Update: %s", m_name.c_str());
        PK_LOG_SCOPE_INDENT(local);

        {
            m_driver->DisposePooledBuffer(m_structureBuffer, m_cmd->GetFenceRef());
            FixedString128 name({ m_name.c_str(),".StructureBuffer" });
            auto createInfo = VulkanBufferCreateInfo(BufferUsage::DefaultAccelerationStructure, bufferSize);
            m_structureBuffer = m_driver->bufferPool.New(m_driver->device, m_driver->allocator, createInfo, name.c_str());
        }

        std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildStructureRangeInfoPtrs;
        buildGeometryInfos.reserve(buildCount);
        buildStructureRangeInfoPtrs.reserve(buildCount);
        auto deployBuild = false;

        for (auto i = 0u; i < m_substructures.GetCount(); ++i)
        {
            auto structure = &m_substructures.GetValueAt(i);
            VkAccelerationStructureCreateInfoKHR info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
            info.buffer = m_structureBuffer->buffer;
            info.offset = structure->bufferOffset;
            info.size = structure->size.accelerationStructureSize;
            info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            FixedString128 name({ structure->name.c_str(), ".BLAS" });
            auto newstructure = new VulkanRawAccelerationStructure(m_driver->device, info, name.c_str());

            if (structure->raw == nullptr)
            {
                auto buildInfo = structure->buildInfo;
                buildInfo.dstAccelerationStructure = newstructure->structure;
                buildInfo.scratchData.deviceAddress = m_scratchBuffer->deviceAddress + structure->scratchOffset;
                buildGeometryInfos.push_back(buildInfo);
                buildStructureRangeInfoPtrs.push_back(&structure->range);
                deployBuild = true;
            }
            else
            {
                VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
                copyInfo.src = structure->raw->structure;
                copyInfo.dst = newstructure->structure;
                copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;

                if (hasCompactedResults && !structure->isCompacted && structure->queryIndex != -1)
                {
                    structure->isCompacted = true;
                    structure->queryIndex = -1;
                    copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                }

                m_cmd->CopyAccelerationStructure(&copyInfo);
                m_driver->disposer->Dispose(structure->raw, m_cmd->GetFenceRef());
            }

            structure->raw = newstructure;
        }

        if (m_structure.raw != nullptr)
        {
            m_driver->disposer->Dispose(m_structure.raw, m_cmd->GetFenceRef());
        }

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = m_structureBuffer->buffer;
        createInfo.offset = m_structure.bufferOffset;
        createInfo.size = m_structure.size.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        m_structure.raw = new VulkanRawAccelerationStructure(m_driver->device, createInfo, FixedString128({ m_name.c_str(), ".TLAS" }).c_str());
        m_bindHandle.acceleration.structure = m_structure.raw->structure;
        m_bindHandle.IncrementVersion();

        if (deployBuild)
        {
            m_cmd->BuildAccelerationStructures((uint32_t)buildGeometryInfos.size(), buildGeometryInfos.data(), buildStructureRangeInfoPtrs.data());
        }

        VkMemoryBarrier memoryBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        VulkanBarrierInfo barrier;
        barrier.srcStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.dstStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.memoryBarrierCount = 1u;
        barrier.pMemoryBarriers = &memoryBarrier;
        m_cmd->PipelineBarrier(barrier);
        m_structureHashPrev = 0ull;
    }

    void VulkanAccelerationStructure::BeginWrite(QueueType queue, uint32_t instanceLimit)
    {
        PK_THROW_ASSERT(m_writeBuffer == nullptr, "Structure is already being written into!");

        m_cmd = m_driver->queues->GetQueue(queue)->commandPool->GetCurrent();
        m_instanceCount = 0u;
        m_instanceLimit = instanceLimit;
        m_structureHashCurr = 0u;

        auto inputBufferStride = sizeof(VkAccelerationStructureInstanceKHR) * instanceLimit;
        auto inputBufferSize = inputBufferStride * PK_RHI_MAX_FRAMES_IN_FLIGHT;
        m_instanceBufferOffset = (m_instanceBufferOffset + inputBufferStride) % inputBufferSize;

        if (m_instanceInputBuffer == nullptr || m_instanceInputBuffer->size < inputBufferSize)
        {
            m_instanceBufferOffset = 0ull;
            m_driver->DisposePooledBuffer(m_instanceInputBuffer, m_cmd->GetFenceRef());
            m_instanceInputBuffer = m_driver->bufferPool.New(m_driver->device,
                m_driver->allocator,
                VulkanBufferCreateInfo(BufferUsage::InstanceInput | BufferUsage::DefaultStaging | BufferUsage::PersistentStage, inputBufferSize),
                FixedString128({ m_name.c_str(), ".InstanceInputBuffer" }).c_str());
        }

        m_writeBuffer = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(m_instanceInputBuffer->BeginMap(m_instanceBufferOffset));
    }

    void VulkanAccelerationStructure::AddInstance(AccelerationStructureGeometryInfo& geometry, const float3x4& matrix)
    {
        PK_THROW_ASSERT(m_instanceCount < m_instanceLimit, "Instance limit exceeded!");

        VkAccelerationStructureInstanceKHR* instance = m_writeBuffer + m_instanceCount++;

        *reinterpret_cast<float4x4*>(instance->transform.matrix) = matrix;
        instance->instanceCustomIndex = geometry.customIndex;
        instance->mask = 0xFF;
        instance->instanceShaderBindingTableRecordOffset = geometry.recordOffset;
        instance->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance->accelerationStructureReference = GetGeometryIndex(geometry);
        m_structureHashCurr += Math::GetMatrixHash(matrix) * (instance->accelerationStructureReference + 1ull);
    }

    void VulkanAccelerationStructure::EndWrite()
    {
        if (m_instanceCount <= 0)
        {
            PK_LOG_WARNING("VulkanAccelerationStructure.EndWrite: write has 0 instances!");
        }

        ValidateResources();

        auto hasChanged = m_structureHashPrev != m_structureHashCurr;

        for (auto i = 0u; i < m_instanceCount && hasChanged; ++i)
        {
            auto index = m_writeBuffer[i].accelerationStructureReference;
            m_writeBuffer[i].accelerationStructureReference = m_substructures.GetValueAt((uint32_t)index).raw->deviceAddress;
        }

        m_instanceInputBuffer->EndMap(m_instanceBufferOffset, sizeof(VkAccelerationStructureInstanceKHR) * m_instanceLimit);
        m_writeBuffer = nullptr;

        if (hasChanged)
        {
            m_structureHashPrev = m_structureHashCurr;
            auto buildInfo = m_structure.buildInfo;
            buildInfo.dstAccelerationStructure = m_structure.raw->structure;
            buildInfo.scratchData.deviceAddress = m_scratchBuffer->deviceAddress + m_structure.scratchOffset;
            const auto* pBuildStructureRangeInfo = &m_structure.range;
            m_cmd->BuildAccelerationStructures(1, &buildInfo, &pBuildStructureRangeInfo);
        }

        m_cmd = nullptr;
    }
}