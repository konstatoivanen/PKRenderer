#include "PrecompiledHeader.h"
#include "VulkanAccelerationStructure.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Core/Services/StringHashID.h"

namespace PK::Rendering::VulkanRHI::Objects
{
	using namespace PK::Core::Services;

	static void CopyVkMatrix(VkTransformMatrixKHR& dst, const float4x4& src)
	{
		for (auto i = 0; i < 3; ++i)
		{
			dst.matrix[i][0] = src[0][i];
			dst.matrix[i][1] = src[1][i];
			dst.matrix[i][2] = src[2][i];
			dst.matrix[i][3] = src[3][i];
		}
	}

    VulkanAccelerationStructure::VulkanAccelerationStructure(const char* name) :
        m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>()), 
        m_name(name)
    {
    }

    VulkanAccelerationStructure::~VulkanAccelerationStructure()
    {
        Dispose(m_driver->GetQueues()->GetFenceRef(QueueType::Graphics));
    }

	VulkanRawBuffer* VulkanAccelerationStructure::GetScratchBuffer(size_t size)
	{
		if (m_scratchBuffer == nullptr || m_scratchBuffer->capacity < size)
		{
			if (m_scratchBuffer != nullptr)
			{
				m_driver->disposer->Dispose(m_scratchBuffer, m_cmd->GetFenceRef());
			}
			
			m_scratchBuffer = new VulkanRawBuffer(m_driver->device, 
				m_driver->allocator, 
				VulkanBufferCreateInfo(BufferUsage::DefaultStorage, size), 
				(m_name + std::string(".ScratchBuffer")).c_str());
		}

		return m_scratchBuffer;
	}

	VulkanRawBuffer* VulkanAccelerationStructure::GetInstanceBuffer(size_t size)
	{
		if (m_instanceInputBuffer == nullptr || m_instanceInputBuffer->capacity < size)
		{
			if (m_instanceInputBuffer != nullptr)
			{
				m_driver->disposer->Dispose(m_instanceInputBuffer, m_cmd->GetFenceRef());
			}

			m_instanceInputBuffer = new VulkanRawBuffer(m_driver->device,
				m_driver->allocator,
				VulkanBufferCreateInfo(BufferUsage::InstanceInput | BufferUsage::DefaultStaging, size),
				(m_name + std::string(".InstanceInputBuffer")).c_str());
		}

		return m_instanceInputBuffer;
	}

	VulkanRawAccelerationStructure* VulkanAccelerationStructure::GetMeshStructure(Mesh* mesh, uint32_t submeshIndex)
	{
		MeshKey key{ mesh, submeshIndex };
		VulkanRawAccelerationStructure* structure = nullptr;

		if (m_subStructures.TryGetValue(key, &structure))
		{
			return structure;
		}

		auto vertexPositionHash = StringHashID::StringToID(PK_VS_POSITION);
		auto& vertexBuffers = mesh->GetVertexBuffers();

		const BufferElement* vertexElement = nullptr;
		const Buffer* vertexBuffer = nullptr;
		uint32_t positionIndex = 0u;

		for (auto j = 0u; j < vertexBuffers.size(); ++j)
		{
			vertexBuffer = vertexBuffers.at(j).get();
			vertexElement = vertexBuffer->GetLayout().TryGetElement(vertexPositionHash, &positionIndex);

			if (vertexElement != nullptr)
			{
				break;
			}
		}

		PK_THROW_ASSERT(vertexElement, "Could not find position stream in mesh!");
		PK_THROW_ASSERT(vertexElement->Type == ElementType::Float3, "Non float3 vertex positions are not supported!");

		auto submesh = mesh->GetSubmesh(submeshIndex);
		auto nativeVertexBuffer = vertexBuffer->GetNative<VulkanBuffer>();
		auto nativeIndexBuffer = mesh->GetIndexBuffer()->GetNative<VulkanBuffer>();
		auto submeshCount = mesh->GetSubmeshCount();

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		// Hardcoded format for float3
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = nativeVertexBuffer->GetRaw()->deviceAddress + vertexElement->Offset;
		accelerationStructureGeometry.geometry.triangles.maxVertex = (uint32_t)nativeVertexBuffer->GetCount() - 1u;
		accelerationStructureGeometry.geometry.triangles.vertexStride = nativeVertexBuffer->GetLayout().GetStride();
		accelerationStructureGeometry.geometry.triangles.indexType = nativeIndexBuffer->GetLayout().GetStride() > 2 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = nativeIndexBuffer->GetRaw()->deviceAddress;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0ull;

		VkAccelerationStructureBuildRangeInfoKHR rangeInfo {};
		rangeInfo.primitiveCount = submesh.indexCount / 3;
		rangeInfo.primitiveOffset = submesh.firstIndex * nativeIndexBuffer->GetLayout().GetStride();
		rangeInfo.firstVertex = submesh.firstVertex;
		rangeInfo.transformOffset = 0;

		structure = new VulkanRawAccelerationStructure(m_driver->device,
			m_driver->allocator,
			accelerationStructureGeometry,
			rangeInfo,
			VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
			(mesh->GetFileName() + std::string(".Submesh") + std::to_string(submeshIndex) + std::string(".BLAS")).c_str());

		m_subStructures.AddValue(key, structure);
		return structure;
	}

	void VulkanAccelerationStructure::BeginWrite(Structs::QueueType queue, uint32_t instanceLimit)
	{
		PK_THROW_ASSERT(m_writeBuffer == nullptr, "Structure is already being written into!");

		m_cmd = m_driver->queues->GetQueue(queue)->commandPool->GetCurrent();
		m_previousSubStructureCount = m_subStructures.GetCount();
		m_instanceLimit = instanceLimit;
		m_instanceCount = 0u;

		auto instanceInputBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_instanceLimit;
		auto instanceSize = sizeof(VkAccelerationStructureInstanceKHR);
		auto instanceInputBuffer = GetInstanceBuffer(instanceInputBufferSize);
		m_writeBuffer = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(instanceInputBuffer->BeginMap(0ull));
	}

	void VulkanAccelerationStructure::AddInstance(Mesh* mesh, uint32_t submesh, uint32_t customIndex, const PK::Math::float4x4& matrix)
	{
		// Wait for mesh uploads to finnish before building BLAS
		if (mesh->HasPendingUpload())
		{
			return;
		}

		PK_THROW_ASSERT(m_instanceCount < m_instanceLimit, "Instance limit exceeded!");

		auto substructure = GetMeshStructure(mesh, submesh);

		VkAccelerationStructureInstanceKHR* instance = m_writeBuffer + m_instanceCount++;

		CopyVkMatrix(instance->transform, matrix);
		instance->instanceCustomIndex = 0;
		instance->mask = 0xFF;
		instance->instanceShaderBindingTableRecordOffset = 0;
		instance->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance->accelerationStructureReference = substructure->deviceAddress;
	}

	void VulkanAccelerationStructure::EndWrite()
	{
		if (m_instanceCount <= 0)
		{
			PK_LOG_WARNING("Ray tracing structure build has no instances!");
		}

		auto substructureCount = m_subStructures.GetCount() - m_previousSubStructureCount;
		auto instanceInputBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_instanceLimit;
		auto instanceSize = sizeof(VkAccelerationStructureInstanceKHR);
		VkDeviceSize scratchBufferSize = 0ull;

		for (auto i = m_previousSubStructureCount; i < m_subStructures.GetCount(); ++i)
		{
			scratchBufferSize += m_subStructures.GetValues()[i]->scratchBufferSize;
		}

		m_instanceInputBuffer->EndMap(0, instanceInputBufferSize);
		m_writeBuffer = nullptr;

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data.deviceAddress = m_instanceInputBuffer->deviceAddress;

		auto accelerationStructureBuildSizesInfo = VulkanRHI::Utilities::VulkanGetAccelerationBuildSizesInfo(m_driver->device,
			accelerationStructureGeometry,
			VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
			m_instanceCount);

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = m_instanceCount;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;

		if (m_structure == nullptr || m_structure->rawBuffer->capacity < accelerationStructureBuildSizesInfo.accelerationStructureSize)
		{
			if (m_structure != nullptr)
			{
				m_driver->disposer->Dispose(m_structure, m_cmd->GetFenceRef());
			}

			m_structure = new VulkanRawAccelerationStructure(m_driver->device,
				m_driver->allocator,
				accelerationStructureGeometry,
				accelerationStructureBuildRangeInfo,
				VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
				(m_name + std::string(".TLAS")).c_str());
		}
		
		auto scratchBuffer = GetScratchBuffer(scratchBufferSize + accelerationStructureBuildSizesInfo.buildScratchSize);
		auto scratchOffset = 0ull;

		if (substructureCount > 0)
		{
			std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildStructureRangeInfoPtrs;
			buildGeometryInfos.reserve(substructureCount + 1);
			buildStructureRangeInfoPtrs.reserve(substructureCount + 1);

			for (auto i = m_previousSubStructureCount; i < m_subStructures.GetCount(); ++i)
			{
				const auto& blas = m_subStructures.GetValueAt(i);
				VkAccelerationStructureBuildGeometryInfoKHR blasGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
				blasGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
				blasGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
				blasGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
				blasGeometryInfo.dstAccelerationStructure = blas->structure;
				blasGeometryInfo.geometryCount = 1;
				blasGeometryInfo.pGeometries = &blas->geometryInfo;
				blasGeometryInfo.scratchData.deviceAddress = scratchBuffer->deviceAddress + scratchOffset;
				scratchOffset += blas->scratchBufferSize;
				buildGeometryInfos.push_back(blasGeometryInfo);
				buildStructureRangeInfoPtrs.push_back(&blas->rangeInfo);
			}

			m_cmd->BuildAccelerationStructures((uint32_t)buildGeometryInfos.size(), buildGeometryInfos.data(), buildStructureRangeInfoPtrs.data());

			VkMemoryBarrier memoryBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			VulkanBarrierInfo barrier;
			barrier.srcStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
			barrier.dstStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
			barrier.memoryBarrierCount = 1u;
			barrier.pMemoryBarriers = &memoryBarrier;
			m_cmd->PipelineBarrier(barrier);
		}

		VkAccelerationStructureBuildGeometryInfoKHR tlasGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		tlasGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		tlasGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		tlasGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		tlasGeometryInfo.dstAccelerationStructure = m_structure->structure;
		tlasGeometryInfo.geometryCount = 1;
		tlasGeometryInfo.pGeometries = &accelerationStructureGeometry;
		tlasGeometryInfo.scratchData.deviceAddress = scratchBuffer->deviceAddress + scratchOffset;
		const auto* pBuildStructureRangeInfo = &accelerationStructureBuildRangeInfo;

		m_cmd->BuildAccelerationStructures(1, &tlasGeometryInfo, &pBuildStructureRangeInfo);

		m_bindHandle.acceleration.structure = m_structure->structure;
		m_bindHandle.IncrementVersion();
		m_cmd = nullptr;
	}

    void VulkanAccelerationStructure::Dispose(const FenceRef& fence)
    {
		if (m_instanceInputBuffer != nullptr)
		{
            m_driver->disposer->Dispose(m_instanceInputBuffer, fence);
		}

		if (m_scratchBuffer != nullptr)
		{
            m_driver->disposer->Dispose(m_scratchBuffer, fence);
		}

        if (m_structure != nullptr)
        {
            m_driver->disposer->Dispose(m_structure, fence);
        }

		auto substructures = m_subStructures.GetValues();

		for (auto i = 0u; i < substructures.count; ++i)
        {
            m_driver->disposer->Dispose(substructures[i], fence);
        }
    }
}