#include "PrecompiledHeader.h"
#include "VulkanRenderState.h"
#include "Core/Services/Log.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::VulkanRHI::Utilities;
    using namespace Services;
    using namespace Core::Services;

    void VulkanRenderState::Reset()
    {
        memset(m_descriptorSetKeys, 0, sizeof(m_descriptorSetKeys));
        memset(&m_pipelineKey, 0, sizeof(PipelineKey));
        memset(m_frameBufferKey, 0, sizeof(m_frameBufferKey));
        memset(m_renderPassKey, 0, sizeof(m_renderPassKey));
        memset(m_frameBufferImages, 0, sizeof(m_frameBufferImages));
        memset(m_viewports, 0, sizeof(m_viewports));
        memset(m_scissors, 0, sizeof(m_scissors));

        m_pipelineKey.fixedFunctionState = FixedFunctionState();
        m_renderPass = nullptr;
        m_pipeline = nullptr;
        m_frameBuffer = nullptr;
        m_indexBuffer = nullptr;
        m_dirtyFlags = PK_RENDER_STATE_DIRTY_RENDERTARGET | 
                       PK_RENDER_STATE_DIRTY_PIPELINE | 
                       PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;

        m_frameBufferKey[0].layers = 1;
        m_frameBufferKey[1].layers = 1;
        m_renderPassKey[0].samples = 1;
        m_renderPassKey[1].samples = 1;
        m_pipelineKey.primitiveRestart = VK_FALSE;
        m_pipelineKey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    void VulkanRenderState::SetRenderTarget(const VulkanBindHandle* const* renderTargets, const VulkanBindHandle* const* resolves, uint32_t count)
    {
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;

        auto passKey = m_renderPassKey;
        auto fboKey = m_frameBufferKey;

        memset(m_frameBufferImages, 0, sizeof(m_frameBufferImages));
        memset(passKey, 0, sizeof(RenderPassKey));
        memset(fboKey, 0, sizeof(FrameBufferKey));

        auto imagesColor = m_frameBufferImages;
        auto imagesResolve = imagesColor + Structs::PK_MAX_RENDER_TARGETS;
        auto imageDepth = imagesResolve + Structs::PK_MAX_RENDER_TARGETS;

        // These should be the same for all targets. Validation will assert if not.
        passKey->samples = renderTargets[0]->image.samples;
        fboKey->layers = renderTargets[0]->image.range.layerCount;
        fboKey->extent = { renderTargets[0]->image.extent.width, renderTargets[0]->image.extent.height };

        for (auto i = 0u, j = 0u; i < count; ++i)
        {
            auto target = renderTargets[i];
            auto resolve = resolves != nullptr ? resolves[i] : nullptr;

            auto isDepth = EnumConvert::IsDepthFormat(target->image.format);
            auto attachment = isDepth ? &passKey->depth : (passKey->colors + j);
            attachment->format = target->image.format;
            attachment->loadop = LoadOp::Keep;
            attachment->storeop = StoreOp::Store;
            attachment->resolve = false;

            if (isDepth)
            {
                // @TODO Handle depth resolves
                fboKey->depth = target->image.view;
                *imageDepth = target;
            }
            else
            {
                attachment->resolve = resolve != nullptr && resolve->image.view != VK_NULL_HANDLE;

                if (attachment->resolve)
                {
                    imagesResolve[j] = resolve;
                    fboKey->resolve[j] = resolve->image.view;
                }

                imagesColor[j] = target;
                fboKey->color[j++] = target->image.view;
            }
        }
    }

    void VulkanRenderState::ClearColor(const color& color, uint32_t index)
    {
        m_renderPassKey[0].colors[index].loadop = LoadOp::Clear;
        m_clearValues[index].color.float32[0] = color.r;
        m_clearValues[index].color.float32[1] = color.g;
        m_clearValues[index].color.float32[2] = color.b;
        m_clearValues[index].color.float32[3] = color.a;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }
    
    void VulkanRenderState::ClearDepth(float depth, uint32_t stencil)
    {
        m_renderPassKey[0].depth.loadop = LoadOp::Clear;
        m_clearValues[PK_MAX_RENDER_TARGETS].depthStencil.depth = depth;
        m_clearValues[PK_MAX_RENDER_TARGETS].depthStencil.stencil = stencil;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }
    
    void VulkanRenderState::DiscardColor(uint32_t index)
    {
        m_renderPassKey[0].colors[index].loadop = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::DiscardDepth()
    {
        m_renderPassKey[0].depth.loadop = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    bool VulkanRenderState::SetViewports(const uint4* rects, uint32_t& count, VkViewport** outViewports)
    {
        if (count > PK_MAX_VIEWPORTS)
        {
            count = PK_MAX_VIEWPORTS;
        }

        *outViewports = m_viewports;
        bool hasChanged = false;

        for (auto i = 0u; i < count; ++i)
        {
            auto& rect = rects[i];
            VkViewport v = { (float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w, 0.0f, 1.0f };

            if (memcmp(&m_viewports[i], &v, sizeof(VkViewport)) != 0)
            {
                m_viewports[i] = v;
                hasChanged = true;
            }
        }

        return hasChanged;
    }

    bool VulkanRenderState::SetScissors(const uint4* rects, uint32_t& count, VkRect2D** outScissors)
    {
        if (count > PK_MAX_VIEWPORTS)
        {
            count = PK_MAX_VIEWPORTS;
        }

        *outScissors = m_scissors;
        bool hasChanged = false;

        for (auto i = 0u; i < count; ++i)
        {
            auto& rect = rects[i];
            VkRect2D v = {{(int)rect.x, (int)rect.y}, { rect.z, rect.w }};

            if (memcmp(&m_scissors[i], &v, sizeof(VkRect2D)) != 0)
            {
                m_scissors[i] = v;
                hasChanged = true;
            }
        }

        return hasChanged;
    }

    void VulkanRenderState::SetShader(const VulkanShader* shader)
    {
        if (m_pipelineKey.shader != shader)
        {
            m_pipelineKey.shader = shader;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE | PK_RENDER_STATE_DIRTY_SHADER;
        }
    }

    void VulkanRenderState::SetBlending(const BlendParameters& blend)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.blending, &blend, sizeof(BlendParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.blending = blend;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetRasterization(const RasterizationParameters& rasterization)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.rasterization, &rasterization, sizeof(RasterizationParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.rasterization = rasterization;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.depthStencil = depthStencil;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.multisampling, &multisampling, sizeof(MultisamplingParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.multisampling = multisampling;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count)
    {
        auto i = 0u;

        for (; i < PK_MAX_VERTEX_ATTRIBUTES && i < count; ++i)
        {
            auto handle = handles[i];

            PK_THROW_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != m_vertexBuffers[i])
            {
                m_vertexBuffers[i] = handle;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
            }
        }

        if (i < PK_MAX_VERTEX_ATTRIBUTES)
        {
            memset(m_vertexBuffers + i, 0, sizeof(m_vertexBuffers[0]) * (PK_MAX_VERTEX_ATTRIBUTES - i));
        }
    }

    void VulkanRenderState::SetIndexBuffer(const VulkanBindHandle* handle, VkIndexType indexType)
    {
        if (m_indexBuffer != handle || (handle != nullptr && indexType != m_indexType))
        {
            m_indexBuffer = handle;
            m_indexType = indexType;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_INDEXBUFFER;
        }
    }

    void VulkanRenderState::SetShaderBindingTableAddress(Structs::RayTracingShaderGroup group, VkDeviceAddress address, size_t stride, size_t size)
    {
        m_shaderBindingTableBundle.addresses[(uint32_t)group] = { address, stride, size };
    }


    VkRenderPassBeginInfo VulkanRenderState::GetRenderPassInfo() const
    {
        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = m_renderPass->renderPass;
        renderPassInfo.framebuffer = m_frameBuffer->frameBuffer;
        renderPassInfo.renderArea = { {}, m_frameBufferKey->extent };
        renderPassInfo.clearValueCount = m_clearValueCount;
        renderPassInfo.pClearValues = m_clearValues;
        return renderPassInfo;
    }

    VulkanVertexBufferBundle VulkanRenderState::GetVertexBufferBundle() const
    {
        VulkanVertexBufferBundle bundle{};

        auto i = 0u;

        for (; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto bufferAttrib = &m_pipelineKey.vertexBuffers[i];
            auto handle = m_vertexBuffers[i];

            if (bufferAttrib->stride == 0 || handle == nullptr)
            {
                break;
            }

            bundle.buffers[i] = handle->buffer.buffer;
        }

        bundle.count = i;

        return bundle;
    }

    VulkanDescriptorSetBundle VulkanRenderState::GetDescriptorSetBundle(const ExecutionGate& gate, uint32_t dirtyFlags)
    {
        VulkanDescriptorSetBundle bundle{};

        if (m_pipelineKey.shader != nullptr)
        {
            bundle.bindPoint = EnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetType());
            bundle.layout = m_pipelineKey.shader->GetPipelineLayout()->layout;
            bundle.firstSet = 0xFFFFu;
            bundle.count = 0u;

            for (auto i = 0u; i < PK_MAX_DESCRIPTOR_SETS; ++i)
            {
                if ((dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
                {
                    if (i < bundle.firstSet)
                    {
                        bundle.firstSet = i;
                    }

                    auto set = m_descriptorSets[i];
                    set->executionGate = gate;
                    bundle.sets[bundle.count++] = set->set;
                }
            }
        }

        return bundle;
    }

    VulkanShaderBindingTableBundle VulkanRenderState::GetShaderBindingTableBundle()
    {
        VulkanShaderBindingTableBundle bundle{};

        if (m_pipelineKey.shader != nullptr)
        {
            for (auto i = 0u; i < (uint32_t)Structs::RayTracingShaderGroup::MaxCount; ++i)
            {
                if (m_pipelineKey.shader->HasRayTracingShaderGroup((Structs::RayTracingShaderGroup)i))
                {
                    bundle.addresses[i] = m_shaderBindingTableBundle.addresses[i];
                }
            }
        }

        return bundle;
    }

    const VulkanBindHandle* VulkanRenderState::GetIndexBuffer(VkIndexType* outIndexType) const
    {
        *outIndexType = m_indexType;
        return m_indexBuffer;
    }

    void VulkanRenderState::ValidateRenderTarget()
    {
        auto shader = m_pipelineKey.shader;
        auto shaderType = shader->GetType();

        if (shaderType != ShaderType::Graphics)
        {
            return;
        }

        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET) == 0)
        {
            return;
        }

        if (memcmp(m_frameBufferKey, m_frameBufferKey + 1, sizeof(FrameBufferKey)) == 0 &&
            memcmp(m_renderPassKey, m_renderPassKey + 1, sizeof(RenderPassKey)) == 0)
        {
            m_dirtyFlags &= ~PK_RENDER_STATE_DIRTY_RENDERTARGET;
            return;
        }

        // Record resource access
        {
            VulkanBarrierHandler::AccessRecord record{};
            VulkanBarrierHandler::AccessRecord recordPrevious{};
            m_renderPassKey->accessMask = 0u;
            m_renderPassKey->stageMask = 0u;

            for (auto i = 0u; i < Structs::PK_MAX_RENDER_TARGETS; ++i)
            {
                auto color = m_frameBufferImages[i];
                auto resolve = m_frameBufferImages[i + PK_MAX_RENDER_TARGETS];

                if (!color || !color->image.image)
                {
                    continue;
                }

                record.imageRange = Utilities::VulkanConvertRange(color->image.range);
                record.aspect = color->image.range.aspectMask;
                record.layout = color->image.layout;
                record.access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                record.stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                RecordAccess(color->image.image, record, &recordPrevious, false);

                m_renderPassKey->accessMask |= recordPrevious.access;
                m_renderPassKey->stageMask |= recordPrevious.stage;
                m_renderPassKey->colors[i].initialLayout = recordPrevious.layout;
                m_renderPassKey->colors[i].finalLayout = record.layout;

                if (!resolve || !resolve->image.image)
                {
                    continue;
                }

                record.imageRange = Utilities::VulkanConvertRange(resolve->image.range);
                record.aspect = resolve->image.range.aspectMask;
                record.layout = resolve->image.layout;
                record.access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                record.stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                RecordAccess(resolve->image.image, record, &recordPrevious, false);
                m_renderPassKey->accessMask |= recordPrevious.access;
                m_renderPassKey->stageMask |= recordPrevious.stage;
            }

            auto depth = m_frameBufferImages[PK_MAX_RENDER_TARGETS * 2];

            if (depth && depth->image.image)
            {
                record.imageRange = Utilities::VulkanConvertRange(depth->image.range);
                record.aspect = depth->image.range.aspectMask;
                record.layout = depth->image.layout;
                record.access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                record.stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

                RecordAccess(depth->image.image, record, &recordPrevious, false);

                m_renderPassKey->accessMask |= recordPrevious.access;
                m_renderPassKey->stageMask |= recordPrevious.stage;
                m_renderPassKey->depth.initialLayout = recordPrevious.layout;
                m_renderPassKey->depth.finalLayout = record.layout;
            }

            if (m_renderPassKey->stageMask == 0u)
            {
                m_renderPassKey->stageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
        }

        memcpy(m_frameBufferKey + 1, m_frameBufferKey, sizeof(FrameBufferKey));
        memcpy(m_renderPassKey + 1, m_renderPassKey, sizeof(RenderPassKey));

        m_renderPass = m_services.frameBufferCache->GetRenderPass(m_renderPassKey[0]);
        m_frameBufferKey[0].renderPass = m_renderPass->renderPass;
        m_frameBuffer = m_services.frameBufferCache->GetFrameBuffer(m_frameBufferKey[0]);

        m_pipelineKey.renderPass = m_renderPass->renderPass;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;

        for (auto i = 0u; i < PK_MAX_RENDER_TARGETS; ++i)
        {
            if (m_renderPassKey[0].colors[i].format == VK_FORMAT_UNDEFINED)
            {
                m_pipelineKey.fixedFunctionState.colorTargetCount = i;
                m_clearValueCount = i;
                m_clearValues[i] = m_clearValues[PK_MAX_RENDER_TARGETS];
                break;
            }
        }

        if (m_renderPassKey[0].depth.format != VK_FORMAT_UNDEFINED)
        {
            m_clearValueCount++;
        }
    }

    void VulkanRenderState::ValidateVertexBuffers()
    {
        auto shader = m_pipelineKey.shader;
        auto shaderType = shader->GetType();

        if (shaderType != ShaderType::Graphics)
        {
            return;
        }

        auto index = 0u;
        auto validateAttributes = false;

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_SHADER)
        {
            for (const auto& element : shader->GetVertexLayout())
            {
                auto* attribute = m_pipelineKey.vertexAttributes + index++;
                auto format = EnumConvert::GetFormat(element.Type);

                if (attribute->location != element.Location ||
                    attribute->format != format)
                {
                    validateAttributes = true;
                    attribute->location = element.Location;
                    attribute->format = format;
                }
            }

            // Attribute count changed
            if (index < PK_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                memset(m_pipelineKey.vertexAttributes + index, 0, sizeof(VkVertexInputAttributeDescription) * (PK_MAX_VERTEX_ATTRIBUTES - index));
                validateAttributes = true;
            }
        }

        if (!validateAttributes && (m_dirtyFlags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) == 0)
        {
            return;
        }

        const auto& shaderLayout = shader->GetVertexLayout();

        for (index = 0u; index < PK_MAX_VERTEX_ATTRIBUTES; ++index)
        {
            auto vertexBuffer = m_vertexBuffers[index];
            
            if (vertexBuffer == nullptr)
            {
                break;
            }

            const auto& bufferLayout = *vertexBuffer->buffer.layout;
            auto stride = bufferLayout.GetStride();
            
            auto bufferAttribute = &m_pipelineKey.vertexBuffers[index];
            bufferAttribute->binding = index;
        
            if (bufferAttribute->inputRate != vertexBuffer->buffer.inputRate || bufferAttribute->stride != stride)
            {
                bufferAttribute->inputRate = vertexBuffer->buffer.inputRate;
                bufferAttribute->stride = stride;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            }

            for (const auto& element : bufferLayout)
            {
                uint32_t elementIdx = 0u;
                if (shaderLayout.TryGetElement(element.NameHashId, &elementIdx) == nullptr)
                {
                    continue;
                }

                auto* attribute = &m_pipelineKey.vertexAttributes[elementIdx];

                // Format equality isn't checked against so that meshes without certain attributes can still be bound.
                if (attribute->binding != index || attribute->offset != element.Offset)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                    attribute->binding = index;
                    attribute->offset = element.Offset;
                }
            }
        }

        // Buffer count changed!
        if (index < PK_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexBuffers[index].stride != 0)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            memset(m_pipelineKey.vertexBuffers + index, 0, sizeof(VkVertexInputBindingDescription) * (PK_MAX_VERTEX_ATTRIBUTES - index));
        }
    }

    void VulkanRenderState::ValidateDescriptorSets(const ExecutionGate& gate)
    {
        auto shader = m_pipelineKey.shader;
        auto setCount = shader->GetDescriptorSetCount();
        auto index = 0u;

        for (auto i = 0u; i < setCount; ++i)
        {
            auto layout = shader->GetDescriptorSetLayout(i);

            if (layout == nullptr)
            {
                continue;
            }

            if (m_descriptorSetKeys[i].stageFlags != layout->stageFlags)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                m_descriptorSetKeys[i].stageFlags = layout->stageFlags;
            }

            auto* bindings = m_descriptorSetKeys[i].bindings;
            Handle<VulkanBindHandle> wrappedHandle = nullptr;
            Handle<VulkanBindArray> wrappedHandleArray = nullptr;
            index = 0u;

            for (const auto& element : shader->GetResourceLayout(i))
            {
                auto* binding = bindings + index++;
                
                if (element.Count > 1)
                {
                    PK_THROW_ASSERT(m_resourceState.TryGet(element.NameHashId, wrappedHandleArray), "Descriptors (%s) not bound!", StringHashID::IDToString(element.NameHashId).c_str());

                    uint32_t version = 0u;
                    uint32_t count = 0u;
                    auto handles = wrappedHandleArray.handle->GetHandles(&version, &count);
                    count = count < element.Count ? (uint16_t)count : element.Count;

                    if (binding->count != count || binding->type != element.Type || binding->handles != handles || binding->version != version || !binding->isArray)
                    {
                        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                        binding->count = count;
                        binding->type = element.Type;
                        binding->handles = handles;
                        binding->version = version;
                        binding->isArray = true;
                    }

                    continue;
                }

                PK_THROW_ASSERT(m_resourceState.TryGet(element.NameHashId, wrappedHandle), "Descriptor (%s) not bound!", StringHashID::IDToString(element.NameHashId).c_str());
                auto handle = wrappedHandle.handle;

                if (binding->count != element.Count || binding->type != element.Type || binding->handle != handle || binding->version != handle->Version() || binding->isArray)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                    binding->count = element.Count;
                    binding->type = element.Type;
                    binding->handle = handle;
                    binding->version = handle->Version();
                    binding->isArray = false;
                }
            }

            // Binding count changed
            if (index < PK_MAX_DESCRIPTORS_PER_SET && bindings[index].count != 0)
            {
                memset(bindings + index, 0, sizeof(bindings[0]) * (PK_MAX_DESCRIPTORS_PER_SET - index));
                m_dirtyFlags |= (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i);
            }
            
            if (m_dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i))
            {
                m_descriptorSets[i] = m_services.descriptorCache->GetDescriptorSet(shader->GetDescriptorSetLayout(i), m_descriptorSetKeys[i], gate);
            }
        }

        // If a lower number set has changed all sets above it need to be rebound.
        //for (auto i = 0; i < (int32_t)(setCount - 1); ++i)
        //{
        //    if ((m_dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
        //    {
        //        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << (i + 1);
        //    }
        //}

        // @TODO investigate why this happens.
        // Unfortunately for some reason a set is being unbound despite using the same layout & resources (in the same pipeline bind point).
        // I'll just have to dirty all of them :/
        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS) != 0)
        {
            for (auto i = 0u; i < setCount; ++i)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
            }
        }

        // Clear remaining keys as they will go unbound when this pipe is used
        if (setCount < PK_MAX_DESCRIPTOR_SETS)
        {
            memset(m_descriptorSetKeys + setCount, 0, sizeof(m_descriptorSetKeys[0]) * (PK_MAX_DESCRIPTOR_SETS - setCount));
        }
    }

    void VulkanRenderState::ValidateResourceStates()
    {
        VulkanBarrierHandler::AccessRecord record{};

        auto shader = m_pipelineKey.shader;
        auto shaderType = shader->GetType();
        auto setCount = m_pipelineKey.shader->GetDescriptorSetCount();

        for (auto i = 0u; i < setCount; ++i)
        {
            const auto& setkey = m_descriptorSetKeys[i];

            for (auto j = 0u; j < Structs::PK_MAX_DESCRIPTORS_PER_SET && setkey.bindings[j].count > 0; ++j)
            {
                auto binding = setkey.bindings[j];

                // @TODO add array support
                if (binding.isArray)
                {
                    continue;
                }
                
                auto handle = binding.handle;
                record.stage = EnumConvert::GetPipelineStageFlags(setkey.stageFlags);
                record.access = shader->GetResourceLayout(i)[j].WriteStageMask != 0u ? VK_ACCESS_SHADER_WRITE_BIT : 0u;

                switch (binding.type)
                {
                    case ResourceType::SamplerTexture:
                    case ResourceType::Texture:
                    case ResourceType::Image:
                    {
                        record.imageRange = VulkanConvertRange(handle->image.range);
                        record.layout = handle->image.layout;
                        record.aspect = handle->image.range.aspectMask;
                        record.access |= VK_ACCESS_SHADER_READ_BIT;
                        RecordAccess(handle->image.image, record);
                    }
                    break;
                    case ResourceType::StorageBuffer:
                    case ResourceType::DynamicStorageBuffer:
                    {
                        record.bufferRange.offset = (uint32_t)handle->buffer.offset;
                        record.bufferRange.size = (uint32_t)handle->buffer.range;
                        record.access |= VK_ACCESS_SHADER_READ_BIT;
                        RecordAccess(handle->buffer.buffer, record);
                    }
                    break;
                    case ResourceType::ConstantBuffer:
                    case ResourceType::DynamicConstantBuffer:
                    {
                        record.bufferRange.offset = (uint32_t)handle->buffer.offset;
                        record.bufferRange.size = (uint32_t)handle->buffer.range;
                        record.access |= VK_ACCESS_UNIFORM_READ_BIT;
                        RecordAccess(handle->buffer.buffer, record);
                    }
                    break;
                }
            }
        }

        if (shaderType != ShaderType::Graphics)
        {
            return;
        }

        for (auto i = 0u; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            if (m_vertexBuffers[i] == nullptr)
            {
                break;
            }

            record.bufferRange.offset = (uint32_t)m_vertexBuffers[i]->buffer.offset;
            record.bufferRange.size = (uint32_t)m_vertexBuffers[i]->buffer.range;
            record.stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            record.access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            RecordAccess(m_vertexBuffers[i]->buffer.buffer, record);
        }

        if (m_indexBuffer != nullptr)
        {
            record.bufferRange.offset = (uint32_t)m_indexBuffer->buffer.offset;
            record.bufferRange.size = (uint32_t)m_indexBuffer->buffer.range;
            record.stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            record.access = VK_ACCESS_INDEX_READ_BIT;
            RecordAccess(m_indexBuffer->buffer.buffer, record);
        }
    }

    PKRenderStateDirtyFlags VulkanRenderState::ValidatePipeline(const ExecutionGate& gate)
    {
        PK_THROW_ASSERT(m_pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        auto shaderType = m_pipelineKey.shader->GetType();
        auto graphicsFlags = m_dirtyFlags & (PK_RENDER_STATE_DIRTY_VERTEXBUFFERS | PK_RENDER_STATE_DIRTY_INDEXBUFFER | PK_RENDER_STATE_DIRTY_RENDERTARGET);

        ValidateRenderTarget();
        ValidateVertexBuffers();
        ValidateDescriptorSets(gate);
        ValidateResourceStates();

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_PIPELINE)
        {
            m_pipeline = m_services.pipelineCache->GetPipeline(m_pipelineKey);
        }

        auto flags = (PKRenderStateDirtyFlags)m_dirtyFlags;
        m_dirtyFlags = 0u;

        // Dont dirty render pass or vertex buffers when not using a graphics pipeline
        if (shaderType != ShaderType::Graphics)
        {
            flags = (PKRenderStateDirtyFlags)(flags & ~graphicsFlags);
            
            // Restore graphics specific dirty flags so that a later pipeline validation can pick these up pipeline assignment can pickup this change.
            m_dirtyFlags |= graphicsFlags;
        }
        
        return flags;
    }
}