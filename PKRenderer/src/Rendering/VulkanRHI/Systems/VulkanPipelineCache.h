#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Objects/VulkanShader.h"
#include "Utilities/HashHelpers.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    struct PipelineKey 
    {
        PK::Utilities::VersionHandle<Objects::VulkanShader> shader;
        Structs::FixedFunctionState fixedFunctionState{};
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkBool32 primitiveRestart = VK_FALSE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkVertexInputAttributeDescription vertexAttributes[Structs::PK_MAX_VERTEX_ATTRIBUTES]{};
        VkVertexInputBindingDescription vertexBuffers[Structs::PK_MAX_VERTEX_ATTRIBUTES]{};

        inline bool operator == (const PipelineKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineKey)) == 0;
        }
    };

    struct PipelineKeyHash
    {
        std::size_t operator()(const PipelineKey& k) const noexcept
        {
            constexpr uint64_t seed = 18446744073709551557;
            return PK::Utilities::HashHelpers::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(PipelineKey), seed);
        }
    };

    class VulkanPipelineCache : public PK::Utilities::NoCopy
    {
        private:

        public:
            constexpr const static char* PIPELINE_CACHE_FILENAME = "shadercache.cache";

            VulkanPipelineCache(VkDevice device, const std::string& workingDirectory, uint64_t pruneDelay);
            ~VulkanPipelineCache();

            struct PipelineValue
            {
                VulkanPipeline* pipeline = nullptr;
                uint64_t pruneTick = 0;
            };

            const VulkanPipeline* GetPipeline(const PipelineKey& key);
            const VulkanPipeline* GetGraphicsPipeline(const PipelineKey& key);
            const VulkanPipeline* GetComputePipeline(const PK::Utilities::VersionHandle<Objects::VulkanShader>& shader);
            void Prune();

        private:
            const VkDevice m_device;
            VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
            std::string m_workingDirectory;
            std::unordered_map<PipelineKey, PipelineValue, PipelineKeyHash> m_graphicsPipelines;
            std::unordered_map<PK::Utilities::VersionHandle<Objects::VulkanShader>, PipelineValue, PK::Utilities::VersionHandle<Objects::VulkanShader>::Hash> m_computePipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}