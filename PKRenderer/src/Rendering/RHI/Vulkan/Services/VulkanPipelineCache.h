#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/Hash.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanShader.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    using namespace PK::Utilities;

    struct PipelineKey 
    {
        PK::Utilities::VersionHandle<Objects::VulkanShader> shader;
        FixedFunctionState fixedFunctionState{};
        VkBool32 primitiveRestart = VK_FALSE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkVertexInputAttributeDescription vertexAttributes[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkVertexInputBindingDescription vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES]{};

        inline bool operator == (const PipelineKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineKey)) == 0;
        }
    };
        
    // Copied from pipeline key in GetPipeline
    struct MeshPipelineKey
    {
        PK::Utilities::VersionHandle<Objects::VulkanShader> shader;
        FixedFunctionState fixedFunctionState{};
        VkRenderPass renderPass = VK_NULL_HANDLE;

        inline bool operator == (const MeshPipelineKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(MeshPipelineKey)) == 0;
        }
    };

    class VulkanPipelineCache : public PK::Utilities::NoCopy
    {
        using PipelineKeyHash = PK::Utilities::Hash::TMurmurHash<PipelineKey>;
        using MeshPipelineKeyHash = PK::Utilities::Hash::TMurmurHash<MeshPipelineKey>;

        public:
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
            const VulkanPipeline* GetComputePipeline(const PK::Utilities::VersionHandle<Objects::VulkanShader>& shader);
            const VulkanPipeline* GetRayTracingPipeline(const PK::Utilities::VersionHandle<Objects::VulkanShader>& shader);
            void Prune();

        private:
            const VkDevice m_device;
            const bool m_allowUnderEstimation;
            const float m_maxOverEstimation;

            VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
            std::string m_workingDirectory;
            FixedPool<VulkanPipeline, 2048> m_pipelinePool;
            FastMap<PipelineKey, PipelineValue, PipelineKeyHash> m_vertexPipelines;
            FastMap<MeshPipelineKey, PipelineValue, MeshPipelineKeyHash> m_meshPipelines;
            FastMap<VersionHandle<Objects::VulkanShader>, PipelineValue, VersionHandle<Objects::VulkanShader>::Hash> m_otherPipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}