#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Memory.h"
#include "Core/Math/Random.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanAccelerationStructure.h"

namespace PK
{
    VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDriver* driver, const char* name) :
        driver(driver),
        name(name),
        substructures(32u, 1ull)
    {
        queryPool.New(driver->device, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, PK_VK_MAX_AS_COMPACTIONS);
    }

    VulkanAccelerationStructure::~VulkanAccelerationStructure()
    {
        auto fence = driver->graph->GetFenceRef();

        queryPool.Delete();
        DisposeStructure(structure.handle, fence);

        for (auto i = 0u; i < substructures.GetCount(); ++i)
        {
            DisposeStructure(substructures[i].value.handle, fence);
        }

        buffer = nullptr;
    }


    uint32_t VulkanAccelerationStructure::RegisterGeometry(const RayTracingGeometryInfo& geometry)
    {
        StructureKey key{ geometry.indexBuffer, ((uint64_t)geometry.indexFirst & 0xFFFFFFFFu) | (((uint64_t)geometry.indexCount) << 32ull) };
        uint32_t index = 0u;

        if (substructures.AddKey(key, &index))
        {
            auto structure = &substructures[index].value;

            *structure = Structure();
            structure->name = geometry.name;
            structure->geometry.vertexRange.address = geometry.vertexBuffer->GetDeviceAddress() + geometry.vertexOffset + (VkDeviceSize)geometry.vertexFirst * geometry.vertexStride;
            structure->geometry.vertexRange.size = (VkDeviceSize)geometry.vertexCount * geometry.vertexStride;
            structure->geometry.indexRange.address = geometry.indexBuffer->GetDeviceAddress() + (VkDeviceSize)geometry.indexFirst * geometry.indexStride;
            structure->geometry.indexRange.size = (VkDeviceSize)geometry.indexCount * geometry.indexStride;
            structure->geometry.vertexStride = geometry.vertexStride;
            structure->geometry.indexStride = geometry.indexStride;

            VkAccelerationStructureGeometryKHR vkGeometry{};
            VkAccelerationStructureBuildRangeInfoKHR range{};
            auto buildInfo = VulkanGetBLASBuildInfo(structure->geometry, &vkGeometry, &range);
            const auto sizeInfo = VulkanGetAccelerationBuildSizesInfo(driver->device, buildInfo, range.primitiveCount);
            structure->size = sizeInfo.accelerationStructureSize;
            structure->buildScratchSize = sizeInfo.buildScratchSize;
        }

        return index;
    }

    bool VulkanAccelerationStructure::PollCompactionResults()
    {
        if (queryCount && queryPool->WaitResults(0ull))
        {
            PK_LOG_RHI("Bottom Level Compaction Update: %s", name.c_str());
            PK_LOG_INDENT(PK_LOG_LVL_RHI);

            for (auto i = 0u; i < substructures.GetCount(); ++i)
            {
                auto* structure = &substructures[i].value;

                if (structure->handle && structure->compactionId && structure->compactionId != COMPACTED_ID)
                {
                    const auto size0 = structure->size;
                    const auto size1 = queryPool->GetResult<VkDeviceSize>(structure->compactionId - 1u, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
                    structure->size = size1;
                    PK_LOG_RHI("BLAS Compacted from %i to %i bytes", size0, size1);
                }
            }

            queryPool->ResetQuery(0u, queryCount);
            queryCount = 0u;
            return true;
        }

        return false;
    }

    uint32_t VulkanAccelerationStructure::AcquireCompactionQueries(VkAccelerationStructureKHR* outHandles, uint32_t* outQueryStart, const FenceRef& fence)
    {
        *outQueryStart = queryCount;
        auto count = 0u;

        for (auto i = 0u; i < substructures.GetCount() && queryCount < PK_VK_MAX_AS_COMPACTIONS; ++i)
        {
            auto* structure = &substructures[i].value;

            if (structure->handle && !structure->compactionId)
            {
                queryPool->SetFence(fence);
                outHandles[count++] = structure->handle;
                structure->compactionId = ++queryCount;
            }
        }

        return count;
    }

    VkAccelerationStructureKHR VulkanAccelerationStructure::CreateStructure(const Structure* structure, VkAccelerationStructureTypeKHR type, const char* name) const
    {
        VkAccelerationStructureCreateInfo2KHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_2_KHR };
        createInfo.addressRange.address = buffer->GetDeviceAddress() + structure->bufferOffset;
        createInfo.addressRange.size = structure->size;
        createInfo.type = type;
        createInfo.createFlags = 0;
        createInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VK_ASSERT_RESULT_CTX(vkCreateAccelerationStructure2KHR(driver->device, &createInfo, nullptr, &handle), "Failed to create acceleration structure!");
        VulkanSetObjectDebugName(driver->device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)handle, name);
        return handle;
    }

    void VulkanAccelerationStructure::DisposeStructure(VkAccelerationStructureKHR handle, const FenceRef& fence) const
    {
        if (handle != VK_NULL_HANDLE)
        {
            driver->disposer->Dispose(driver->device, handle, 
            [](void* c, void* v)
            {
                vkDestroyAccelerationStructureKHR(static_cast<VkDevice>(c), static_cast<VkAccelerationStructureKHR>(v), nullptr);
            },
            fence);
        }
    }

    void VulkanAccelerationStructureBuilder::AddInstance(const RayTracingGeometryInfo& geometry, const float3x4& matrix)
    {
        PK_DEBUG_FATAL_ASSERT(instanceCount < instanceLimit, "Instance limit exceeded!");

        auto index = structure->RegisterGeometry(geometry);
        
        VkAccelerationStructureInstanceKHR instance;
        instance.transform = Memory::BitCast<float3x4, VkTransformMatrixKHR>(matrix);
        instance.instanceCustomIndex = geometry.customIndex;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = geometry.recordOffset;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = 0ull;

        // Important do not not "optimize" this to assign the members individually.
        // This goes directly to PCIE BAR.
        instances[instanceCount] = instance;
        instanceIndices[instanceCount++] = index;
        topologyHash += math::hash(matrix, 0.01f) * ((uint64_t)index + 1ull);
    }
}
