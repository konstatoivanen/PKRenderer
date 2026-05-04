#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/HashMap.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanShader.h"

namespace PK
{
    struct VulkanPipelineCache : public NoCopy
    {
        struct FixedFunctionState
        {
            RasterizationParameters rasterization{};
            BlendParameters blending{};
            DepthStencilParameters depthStencil{};
            MultisamplingParameters multisampling{};
            VkFormat colorFormats[PK_RHI_MAX_RENDER_TARGETS]{};
            VkFormat depthFormat = VK_FORMAT_UNDEFINED;
            uint16_t excludeStageMask = 0u;
        };

        struct PipelineKey
        {
            VersionHandle<VulkanShader> shader;
            FixedFunctionState fixedFunctionState{};
            VkBool32 primitiveRestart = VK_FALSE;
            VkVertexInputAttributeDescription vertexAttributes[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VkVertexInputBindingDescription vertexStreams[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};

            inline bool operator == (const PipelineKey& r) const noexcept { return memcmp(this, &r, sizeof(PipelineKey)) == 0; }
        };

        // Copied from pipeline key in GetPipeline
        struct MeshPipelineKey
        {
            VersionHandle<VulkanShader> shader;
            FixedFunctionState fixedFunctionState{};

            inline bool operator == (const MeshPipelineKey& r) const noexcept { return memcmp(this, &r, sizeof(MeshPipelineKey)) == 0; }
        };

        using PipelineKeyHash = Hash::TMurmurHash<PipelineKey>;
        using MeshPipelineKeyHash = Hash::TMurmurHash<MeshPipelineKey>;

        constexpr const static char* PIPELINE_CACHE_FILENAME = "shadercache.cache";

        VulkanPipelineCache(VkDevice device, 
            const VulkanPhysicalDeviceProperties& physicalDeviceProperties, 
            const char* workingDirectory, 
            bool enablePipelineCache,
            uint64_t pruneDelay);

        ~VulkanPipelineCache();

        struct PipelineValue
        {
            VulkanPipeline* pipeline = nullptr;
            uint64_t pruneTick = 0;
        };

        const VulkanPipeline* GetPipeline(const PipelineKey& key);
        const VulkanPipeline* GetGraphicsPipeline(const PipelineKey& key);
        const VulkanPipeline* GetComputePipeline(const VersionHandle<VulkanShader>& shader);
        const VulkanPipeline* GetRayTracingPipeline(const VersionHandle<VulkanShader>& shader);
        void Prune();

        private:
            const VkDevice m_device;
            const float m_maxOverEstimation;
            const bool m_allowUnderEstimation;

            VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
            FixedString256 m_workingDirectory;
            FixedPool<VulkanPipeline, PK_VK_MAX_PIPELINES> m_pipelinePool;
            FixedMap16<PipelineKey, PipelineValue, PK_VK_MAX_PIPELINES_VERTEX, PipelineKeyHash> m_vertexPipelines;
            FixedMap16<MeshPipelineKey, PipelineValue, PK_VK_MAX_PIPELINES_MESH, MeshPipelineKeyHash> m_meshPipelines;
            FixedMap16<VersionHandle<VulkanShader>, PipelineValue, PK_VK_MAX_PIPELINES_GENERIC> m_otherPipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}
