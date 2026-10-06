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
        auto fence = driver->GetQueues()->GetLastSubmitFenceRef();

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
            structure->geometry.vertexAddress = geometry.vertexBuffer->GetDeviceAddress() + geometry.vertexOffset;
            structure->geometry.indexAddress = geometry.indexBuffer->GetDeviceAddress();
            structure->geometry.vertexCount = geometry.vertexCount;
            structure->geometry.vertexFirst = geometry.vertexFirst;
            structure->geometry.vertexStride = geometry.vertexStride;
            structure->geometry.indexCount = geometry.indexCount;
            structure->geometry.indexFirst = geometry.indexFirst;
            structure->geometry.indexStride = geometry.indexStride;

            VkAccelerationStructureGeometryKHR vkGeometry{};
            VkAccelerationStructureBuildRangeInfoKHR range{};
            auto buildInfo = GetBLASBuildInfo(structure->geometry, &vkGeometry, &range);
            const auto sizeInfo = VulkanGetAccelerationBuildSizesInfo(driver->device, buildInfo, range.primitiveCount);
            structure->size = sizeInfo.accelerationStructureSize;
            structure->buildScratchSize = sizeInfo.buildScratchSize;
        }

        return index;
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


    // @TODO MOVE THESE OUT OF HERE.
    VkAccelerationStructureBuildGeometryInfoKHR VulkanAccelerationStructure::GetBLASBuildInfo(const GeometryData& geo, VkAccelerationStructureGeometryKHR* outGeometry, VkAccelerationStructureBuildRangeInfoKHR* outRange)
    {
        *outGeometry = VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        outGeometry->geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        outGeometry->geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        outGeometry->geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        outGeometry->geometry.triangles.vertexStride = geo.vertexStride;
        outGeometry->geometry.triangles.vertexData.deviceAddress = geo.vertexAddress;
        outGeometry->geometry.triangles.maxVertex = geo.vertexFirst + geo.vertexCount - 1u;
        outGeometry->geometry.triangles.indexType = geo.indexStride > 2u ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
        outGeometry->geometry.triangles.indexData.deviceAddress = geo.indexAddress;
        outGeometry->geometry.triangles.transformData.deviceAddress = 0ull;
        outGeometry->flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        *outRange = { geo.indexCount / 3u, geo.indexStride * geo.indexFirst, geo.vertexFirst, 0u };

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.geometryCount = 1u;
        buildInfo.pGeometries = outGeometry;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;
        return buildInfo;
    }

    VkAccelerationStructureBuildGeometryInfoKHR VulkanAccelerationStructure::GetTLASBuildInfo(VkDeviceAddress instanceDataAddress, VkAccelerationStructureGeometryKHR* outGeometry)
    {
        *outGeometry = VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        outGeometry->geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        outGeometry->geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        outGeometry->geometry.instances.arrayOfPointers = VK_FALSE;
        outGeometry->geometry.instances.data.deviceAddress = instanceDataAddress;
        outGeometry->flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.geometryCount = 1u;
        buildInfo.pGeometries = outGeometry;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        return buildInfo;
    }

    VkAccelerationStructureKHR VulkanAccelerationStructure::CreateStructure(const Structure* structure, VkAccelerationStructureTypeKHR type, const char* name) const
    {
        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = buffer->GetNativeView<VulkanBindHandle>()->buffer.buffer;
        createInfo.offset = structure->bufferOffset;
        createInfo.size = structure->size;
        createInfo.type = type;
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VK_ASSERT_RESULT_CTX(vkCreateAccelerationStructureKHR(driver->device, &createInfo, nullptr, &handle), "Failed to create acceleration structure!");
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

}
