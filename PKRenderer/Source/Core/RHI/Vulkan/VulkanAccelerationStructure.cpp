#include "PrecompiledHeader.h"
#include "Core/Utilities/Memory.h"
#include "Core/Math/Random.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanAccelerationStructure.h"

namespace PK
{
    VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDriver* driver, const char* name) :
        m_driver(driver),
        m_name(name),
        m_substructures(32u, 1ull)
    {
        m_queryPool.New(m_driver->device, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, 256u);
    }

    VulkanAccelerationStructure::~VulkanAccelerationStructure()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        m_queryPool.Delete();
        DisposeVkAccelerationStructureKHR(m_structure.handle, fence);

        for (auto i = 0u; i < m_substructures.GetCount(); ++i)
        {
            DisposeVkAccelerationStructureKHR(m_substructures[i].value.handle, fence);
        }

        m_driver->DisposePooled(m_instanceInputBuffer, fence);
        m_driver->DisposePooled(m_scratchBuffer, fence);
        m_driver->DisposePooled(m_structureBuffer, fence);
    }


    void VulkanAccelerationStructure::DisposeVkAccelerationStructureKHR(VkAccelerationStructureKHR handle, const FenceRef& fence) const
    {
        if (handle != VK_NULL_HANDLE)
        {
            m_driver->disposer->Dispose(m_driver->device, handle, 
                [](void* c, void* v)
                {
                    vkDestroyAccelerationStructureKHR(static_cast<VkDevice>(c), static_cast<VkAccelerationStructureKHR>(v), nullptr);
                },
                fence);
        }
    }

    VkAccelerationStructureKHR VulkanAccelerationStructure::CreateVkAccelerationStructureKHR(const Structure* structure, VkAccelerationStructureTypeKHR type, const char* name) const
    {
        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = m_structureBuffer->buffer;
        createInfo.offset = structure->bufferOffset;
        createInfo.size = structure->size.accelerationStructureSize;
        createInfo.type = type;
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VK_ASSERT_RESULT_CTX(vkCreateAccelerationStructureKHR(m_driver->device, &createInfo, nullptr, &handle), "Failed to create acceleration structure!");
        VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)handle, name);
        return handle;
    }
    

    void VulkanAccelerationStructure::BeginWrite(QueueType queue, uint32_t instanceLimit)
    {
        PK_DEBUG_FATAL_ASSERT(m_writeBuffer == nullptr, "Structure is already being written into!");

        m_cmd = m_driver->queues->GetQueue(queue)->GetCommandBuffer();
        m_instanceCount = 0u;
        m_instanceLimit = instanceLimit;
        m_topologyHashCurr = 0u;

        auto inputBufferStride = sizeof(VkAccelerationStructureInstanceKHR) * instanceLimit;
        auto inputBufferSize = inputBufferStride * PK_RHI_MAX_FRAMES_IN_FLIGHT;
        m_instanceBufferOffset = (m_instanceBufferOffset + inputBufferStride) % inputBufferSize;

        if (m_instanceInputBuffer == nullptr || m_instanceInputBuffer->size < inputBufferSize)
        {
            m_instanceBufferOffset = 0ull;
            m_driver->DisposePooled(m_instanceInputBuffer, m_cmd->GetFenceRef());
            m_instanceInputBuffer = m_driver->CreatePooled<VulkanRawBuffer>(m_driver->device,
                m_driver->allocator,
                VulkanBufferCreateInfo(BufferUsage::InstanceInput | BufferUsage::DefaultStaging | BufferUsage::PersistentStage, inputBufferSize),
                FixedString128({ m_name.c_str(), ".InstanceInputBuffer" }).c_str());
        }

        m_writeBuffer = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(m_instanceInputBuffer->BeginMap(m_instanceBufferOffset, 0ull));
    }

    void VulkanAccelerationStructure::AddInstance(const RayTracingGeometryInfo& geometry, const float3x4& matrix)
    {
        PK_DEBUG_FATAL_ASSERT(m_instanceCount < m_instanceLimit, "Instance limit exceeded!");

        StructureKey key{ geometry.indexBuffer, ((uint64_t)geometry.indexFirst & 0xFFFFFFFFu) | (((uint64_t)geometry.indexCount) << 32ull) };
        uint32_t index = 0u;

        if (m_substructures.AddKey(key, &index))
        {
            auto structure = &m_substructures[index].value;

            *structure = Structure();
            structure->name = geometry.name;

            auto addressVertex = geometry.vertexBuffer->GetDeviceAddress();
            auto addressIndex = geometry.indexBuffer->GetDeviceAddress();

            structure->geometry = VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            structure->geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            structure->geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            structure->geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            structure->geometry.geometry.triangles.vertexStride = geometry.vertexStride;
            structure->geometry.geometry.triangles.vertexData.deviceAddress = addressVertex + geometry.vertexOffset;
            structure->geometry.geometry.triangles.maxVertex = geometry.vertexFirst + geometry.vertexCount - 1u;
            structure->geometry.geometry.triangles.indexType = geometry.indexStride > 2u ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
            structure->geometry.geometry.triangles.indexData.deviceAddress = addressIndex;
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
        }

        VkAccelerationStructureInstanceKHR* instance = m_writeBuffer + m_instanceCount++;
        instance->transform = Memory::BitCast<float3x4, VkTransformMatrixKHR>(matrix);
        instance->instanceCustomIndex = geometry.customIndex;
        instance->mask = 0xFF;
        instance->instanceShaderBindingTableRecordOffset = geometry.recordOffset;
        instance->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance->accelerationStructureReference = (uint64_t)index;

        m_topologyHashCurr += math::hash(matrix, 0.01f) * (instance->accelerationStructureReference + 1ull);
    }

    void VulkanAccelerationStructure::EndWrite()
    {
        PK_DEBUG_WARNING_ASSERT(m_instanceCount > 0, "VulkanAccelerationStructure.EndWrite: write has 0 instances!");

        // Validate everything.
        {
            VkDeviceSize scratchSize = 0ull;
            VkDeviceSize bufferSize = 0ull;
            uint32_t buildCount = 0u;

            // Compaction queries
            for (auto i = 0u; i < m_substructures.GetCount(); ++i)
            {
                auto structure = &m_substructures[i].value;

                if (structure->handle && !structure->compactionId)
                {
                    structure->compactionId = m_cmd->QueryAccelerationStructureCompactSize(structure->handle, m_queryPool) + 1u;
                }
            }

            auto hasCompactedResults = m_queryPool->WaitResults(0ull);
            auto needsRealloc = false;

            // Update tlas header and detect if udpates are required.
            {
                if (hasCompactedResults)
                {
                    PK_LOG_RHI("Bottom Level Compaction Update: %s", m_name.c_str());
                }

                PK_LOG_INDENT(PK_LOG_LVL_RHI);

                for (auto i = 0u; i < m_substructures.GetCount(); ++i)
                {
                    auto structure = &m_substructures[i].value;

                    if (hasCompactedResults && structure->handle && structure->compactionId && structure->compactionId != COMPACTED_ID)
                    {
                        auto size0 = structure->size.accelerationStructureSize;
                        auto size1 = m_queryPool->GetResult<VkDeviceSize>(structure->compactionId - 1u, 0, VK_QUERY_RESULT_WAIT_BIT);
                        structure->size.accelerationStructureSize = size1;
                        PK_LOG_RHI("BLAS Compacted from %i to %i bytes", size0, size1);
                    }

                    structure->bufferOffset = bufferSize;
                    bufferSize += math::align(structure->size.accelerationStructureSize, 256ull);

                    if (!structure->handle)
                    {
                        structure->scratchOffset = scratchSize;
                        scratchSize += math::align(structure->size.buildScratchSize, 256ull);
                        ++buildCount;
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
                m_structure.bufferOffset = bufferSize;
                m_structure.scratchOffset = scratchSize;
                needsRealloc = prevSize < m_structure.size.accelerationStructureSize;
                bufferSize += math::align(m_structure.size.accelerationStructureSize, 256ull);
                scratchSize += math::align(m_structure.size.buildScratchSize, 256ull);
            }

            if (m_scratchBuffer == nullptr || m_scratchBuffer->size < scratchSize)
            {
                m_driver->DisposePooled(m_scratchBuffer, m_cmd->GetFenceRef());
                FixedString128 name({ m_name.c_str(),".ScratchBuffer" });
                auto createInfo = VulkanBufferCreateInfo(BufferUsage::DefaultStorage | BufferUsage::AccelerationStructure, scratchSize);
                m_scratchBuffer = m_driver->CreatePooled<VulkanRawBuffer>(m_driver->device, m_driver->allocator, createInfo, name.c_str());
            }

            if (buildCount || hasCompactedResults || needsRealloc || !m_structureBuffer || m_structureBuffer->size < bufferSize)
            {
                PK_LOG_RHI_SCOPE("Acceleration Structure Update: %s", m_name.c_str());

                // Reset compaction queries.
                m_queryPool->ResetQuery();

                // Needs new buffer in case of compaction copies.
                {
                    m_driver->DisposePooled(m_structureBuffer, m_cmd->GetFenceRef());
                    FixedString128 name({ m_name.c_str(),".StructureBuffer" });
                    auto createInfo = VulkanBufferCreateInfo(BufferUsage::DefaultAccelerationStructure, bufferSize);
                    m_structureBuffer = m_driver->CreatePooled<VulkanRawBuffer>(m_driver->device, m_driver->allocator, createInfo, name.c_str());
                }

                auto buildGeometryInfos = m_driver->arena.Allocate<VkAccelerationStructureBuildGeometryInfoKHR>(buildCount);
                auto buildStructureRangeInfoPtrs = m_driver->arena.Allocate<VkAccelerationStructureBuildRangeInfoKHR*>(buildCount);
                buildCount = 0u;

                // Full rebuild needed even if only one element compacted as buffer offsets have been recalculated.
                for (auto i = 0u; i < m_substructures.GetCount(); ++i)
                {
                    auto structure = &m_substructures[i].value;
                    auto newHandle = CreateVkAccelerationStructureKHR(structure, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, FixedString128({ structure->name.c_str(), ".BLAS" }));

                    if (structure->handle == VK_NULL_HANDLE)
                    {
                        buildGeometryInfos[buildCount] = structure->buildInfo;
                        buildGeometryInfos[buildCount].dstAccelerationStructure = newHandle;
                        buildGeometryInfos[buildCount].scratchData.deviceAddress = m_scratchBuffer->deviceAddress + structure->scratchOffset;
                        buildStructureRangeInfoPtrs[buildCount++] = &structure->range;
                    }
                    else
                    {
                        VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
                        copyInfo.src = structure->handle;
                        copyInfo.dst = newHandle;
                        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;

                        if (hasCompactedResults && structure->compactionId && structure->compactionId != COMPACTED_ID)
                        {
                            structure->compactionId = COMPACTED_ID;
                            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                        }

                        m_cmd->CopyAccelerationStructure(&copyInfo);
                        DisposeVkAccelerationStructureKHR(structure->handle, m_cmd->GetFenceRef());
                    }

                    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, nullptr, newHandle };
                    structure->deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_driver->device, &addressInfo);
                    structure->handle = newHandle;
                }

                // Recreate tlas
                {
                    DisposeVkAccelerationStructureKHR(m_structure.handle, m_cmd->GetFenceRef());
                    m_structure.handle = CreateVkAccelerationStructureKHR(&m_structure, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, FixedString128({ m_name.c_str(), ".TLAS" }));
                    m_bindHandle.acceleration.structure = m_structure.handle;
                    m_bindHandle.IncrementVersion();
                }

                if (buildCount)
                {
                    m_cmd->BuildAccelerationStructures(buildCount, buildGeometryInfos, buildStructureRangeInfoPtrs);
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
                m_topologyHashPrev = 0ull;
            }
        }

        auto hasChanged = m_topologyHashPrev != m_topologyHashCurr;

        for (auto i = 0u; i < m_instanceCount && hasChanged; ++i)
        {
            auto index = (uint32_t)m_writeBuffer[i].accelerationStructureReference;
            m_writeBuffer[i].accelerationStructureReference = m_substructures[index].value.deviceAddress;
        }

        m_instanceInputBuffer->EndMap(m_instanceBufferOffset, sizeof(VkAccelerationStructureInstanceKHR) * m_instanceLimit);
        m_writeBuffer = nullptr;

        if (hasChanged)
        {
            m_topologyHashPrev = m_topologyHashCurr;
            auto buildInfo = m_structure.buildInfo;
            buildInfo.dstAccelerationStructure = m_structure.handle;
            buildInfo.scratchData.deviceAddress = m_scratchBuffer->deviceAddress + m_structure.scratchOffset;
            const auto* pBuildStructureRangeInfo = &m_structure.range;
            m_cmd->BuildAccelerationStructures(1, &buildInfo, &pBuildStructureRangeInfo);
            m_lastBuildFenceRef = m_cmd->GetFenceRef();
        }

        m_cmd = nullptr;
    }
}
