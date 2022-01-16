#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Objects/VulkanShader.h"
#include "Utilities/HashHelpers.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace Objects;

    struct PipelineKey 
    {
        IDHandle<VulkanShader> shader;
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
            return HashHelpers::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(PipelineKey), seed);
        }
    };

    class VulkanPipelineCache : public NoCopy
    {
        private:

        public:
            VulkanPipelineCache(VkDevice device, uint64_t pruneDelay) : m_device(device), m_pruneDelay(pruneDelay) {}
            ~VulkanPipelineCache();

            struct PipelineValue
            {
                VulkanPipeline* pipeline = nullptr;
                uint64_t pruneTick = 0;
            };

            const VulkanPipeline* GetPipeline(const PipelineKey& key);
            const VulkanPipeline* GetGraphicsPipeline(const PipelineKey& key);
            const VulkanPipeline* GetComputePipeline(const IDHandle<VulkanShader>& shader);
            void Prune();

        private:
            const VkDevice m_device;
            std::unordered_map<PipelineKey, PipelineValue, PipelineKeyHash> m_graphicsPipelines;
            std::unordered_map<IDHandle<VulkanShader>, PipelineValue, IDHandle<VulkanShader>::Hash> m_computePipelines;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}