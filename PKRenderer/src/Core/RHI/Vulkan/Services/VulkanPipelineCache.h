#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/Hash.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Vulkan/VulkanShader.h"

namespace PK
{
    struct VulkanPipelineCache : public NoCopy
    {
        struct PipelineKey
        {
            VersionHandle<VulkanShader> shader;
            FixedFunctionState fixedFunctionState{};
            VkBool32 primitiveRestart = VK_FALSE;
            VkRenderPass renderPass = VK_NULL_HANDLE;
            VkVertexInputAttributeDescription vertexAttributes[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VkVertexInputBindingDescription vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};

            inline bool operator == (const PipelineKey& r) const noexcept
            {
                return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineKey)) == 0;
            }
        };

        // Copied from pipeline key in GetPipeline
        struct MeshPipelineKey
        {
            VersionHandle<VulkanShader> shader;
            FixedFunctionState fixedFunctionState{};
            VkRenderPass renderPass = VK_NULL_HANDLE;

            inline bool operator == (const MeshPipelineKey& r) const noexcept
            {
                return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(MeshPipelineKey)) == 0;
            }
        };

        using PipelineKeyHash = Hash::TMurmurHash<PipelineKey>;
        using MeshPipelineKeyHash = Hash::TMurmurHash<MeshPipelineKey>;

        constexpr const static char* PIPELINE_CACHE_FILENAME = "shadercache.cache";

        VulkanPipelineCache(VkDevice device, const std::string& workingDirectory, const VulkanPhysicalDeviceProperties& physicalDeviceProperties, uint64_t pruneDelay);
        ~VulkanPipelineCache();

        struct PipelineValue
        {
            VulkanPipeline* pipeline = nullptr;
            uint64_t pruneTick = 0;
        };

        const VulkanPipeline* GetPipeline(const PipelineKey& key);
        const VulkanPipeline* GetVertexPipeline(const PipelineKey& key);
        const VulkanPipeline* GetMeshPipeline(const MeshPipelineKey& key);
        const VulkanPipeline* GetComputePipeline(const VersionHandle<VulkanShader>& shader);
        const VulkanPipeline* GetRayTracingPipeline(const VersionHandle<VulkanShader>& shader);
        void Prune();

        private:
            const VkDevice m_device;
            const float m_maxOverEstimation;
            const bool m_allowUnderEstimation;

            VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
            std::string m_workingDirectory;
            FixedPool<VulkanPipeline, 2048> m_pipelinePool;
            FastMap<PipelineKey, PipelineValue, PipelineKeyHash> m_vertexPipelines;
            FastMap<MeshPipelineKey, PipelineValue, MeshPipelineKeyHash> m_meshPipelines;
            FastMap<VersionHandle<VulkanShader>, PipelineValue, VersionHandle<VulkanShader>::Hash> m_otherPipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}