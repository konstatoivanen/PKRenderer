#pragma once
#include "Rendering/VulkanRHI/Objects/VulkanShader.h"
#include "Core/NoCopy.h"
#include "Math/PKMath.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace Objects;

    struct MultisamplingParameters
    {
        VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkBool32 sampleShadingEnable = VK_FALSE;
        float minSampleShading = 1.0f;
        VkBool32 alphaToCoverageEnable = VK_FALSE;
        VkBool32 alphaToOneEnable = VK_FALSE;
    };

    struct DepthStencilParameters
    {
        VkBool32 depthTestEnable = VK_FALSE;
        VkBool32 depthWriteEnable = VK_FALSE;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;
        VkBool32 depthBoundsTestEnable = VK_FALSE;
        VkBool32 stencilTestEnable = VK_FALSE;
        float minDepthBounds = 0;
        float maxDepthBounds = 1;
    };

    struct RasterizationParameters
    {
        VkBool32 depthClampEnable = VK_FALSE;
        VkBool32 rasterizerDiscardEnable = VK_FALSE;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
        VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
        VkBool32 depthBiasEnable = VK_FALSE;
        float depthBiasConstantFactor = 0;
        float depthBiasClamp = 0;
        float depthBiasSlopeFactor = 0;
        float lineWidth = 0;
    };

    struct FixedFunctionState 
    {
        RasterizationParameters rasterization{};
        VkPipelineColorBlendAttachmentState blending{};
        DepthStencilParameters depthStencil{};
        MultisamplingParameters multisampling{};
        uint32_t colorTargetCount = 0;
        uint32_t viewportCount = 1;
    };
        
    struct PipelineKey 
    {
        const VulkanShader* shader = nullptr;
        FixedFunctionState fixedFunctionState{};
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkBool32 primitiveRestart = VK_FALSE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkVertexInputAttributeDescription vertexAttributes[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkVertexInputBindingDescription vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES]{};

        inline bool operator == (const PipelineKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineKey)) == 0;
        }
    };

    struct PipelineKeyHash
    {
        std::size_t operator()(const PipelineKey& k) const noexcept
        {
            constexpr ulong seed = 18446744073709551557;
            return PK::Math::Functions::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(PipelineKey), seed);
        }
    };

    class VulkanPipelineCache : public PK::Core::NoCopy
    {
        public:
            VulkanPipelineCache(VkDevice device, uint64_t pruneDelay) : m_device(device), m_pruneDelay(pruneDelay) {}

            struct PipelineValue
            {
                Ref<VulkanPipeline> pipeline = nullptr;
                uint64_t pruneTick = 0;
            };

            const VulkanPipeline* GetPipeline(const PipelineKey& key);
            void Prune();

        private:
            const VkDevice m_device;
            std::unordered_map<PipelineKey, PipelineValue, PipelineKeyHash> m_pipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}