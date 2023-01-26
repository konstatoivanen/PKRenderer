#include "PrecompiledHeader.h"
#include "VulkanBarrierHandler.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::Services
{
    VulkanBarrierHandler::~VulkanBarrierHandler()
    {
    }
    
    void VulkanBarrierHandler::RecordAccess(VkBuffer buffer, uint32_t offset, uint32_t size, VkPipelineStageFlags stage, VkAccessFlags access)
    {
        auto& current = m_buffers[buffer];
        auto cRead = EnumConvert::IsReadAccess(access);
        auto cWrite = EnumConvert::IsWriteAccess(access);
        auto pRead = EnumConvert::IsReadAccess(current.access);
        auto pWrite = EnumConvert::IsReadAccess(current.access);
    

    }
    
    void VulkanBarrierHandler::RecordAccess(VkImage image, const Structs::TextureViewRange& range, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access)
    {
    }
    
    bool VulkanBarrierHandler::Resolve(VulkanBarrierInfo* outBarrierInfo)
    {
        return false;
    }
}