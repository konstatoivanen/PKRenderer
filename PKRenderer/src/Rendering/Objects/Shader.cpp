#pragma once
#include "PrecompiledHeader.h"
#include "Shader.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Objects/VulkanShader.h"
#include <PKAssets/PKAssetLoader.h>

using namespace PK::Core;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::VulkanRHI::Objects;

namespace PK::Rendering::Objects
{
    void ShaderVariant::ListProperties()
    {
        PK_LOG_HEADER("Vertex Attributes:");

        for (auto& element : m_vertexLayout)
        {
            PK_LOG_INFO("%s %i %i", StringHashID::IDToString(element.NameHashId).c_str(), element.Location, element.Type);
        }

        PK_LOG_HEADER("Dynamic Constants:");

        for (auto& kv : m_constantLayout)
        {
            auto& element = kv.second;
            PK_LOG_INFO("%s %i %i", StringHashID::IDToString(element.NameHashId).c_str(), element.Offset, element.Size);
        }

        PK_LOG_HEADER("Descriptor Sets:");
        
        for (auto i = 0u; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            PK_LOG_HEADER("Sets %i :", i);

            auto& set = m_resourceLayouts[i];

            for (auto& element : set)
            {
                PK_LOG_INFO("%s %i", StringHashID::IDToString(element.NameHashId).c_str(), element.Type);
            }
        }
    }

    void ShaderVariantMap::ListVariants()
    {
        std::vector<std::vector<std::string>> keywordlist;

        for (auto& kv : keywords)
        {
            auto keyword = StringHashID::IDToString(kv.first);
            auto index0 = (kv.second >> 4) & 0xF;
            auto index1 = kv.second & 0xF;

            if (keywordlist.size() <= index0)
            {
                keywordlist.resize(index0 + 1);
            }

            if (keywordlist.at(index0).size() <= index1)
            {
                keywordlist.at(index0).resize(index1 + 1);
            }

            keywordlist.at(index0)[index1] = keyword;
        }

        PK_LOG_NEWLINE();

        for (auto j = 0u; j < variantcount; ++j)
        {
            std::string defines = "";
            auto index = j;

            for (auto i = 0; i < keywordlist.size(); ++i)
            {
                auto& declares = keywordlist.at(i);
                auto& keyword = declares.at(index % declares.size());

                if (!keyword.empty())
                {
                    defines.append(keyword);
                    defines.append(" ");
                }

                index /= (uint32_t)declares.size();
            }

            PK_LOG_INFO(defines.c_str());
        }

        PK_LOG_NEWLINE();
    }

    bool ShaderVariantMap::SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const
    {
        for (auto i = 0u; i < count; ++i)
        {
            if (!SupportsKeyword(hashIds[i]))
            {
                return false;
            }
        }

        return true;
    }

    uint32_t ShaderVariantMap::GetIndex(const uint32_t* hashIds, size_t count) const
    {
        uint8_t flags[16]{};

        for (auto i = 0u; i < count; ++i)
        {
            auto kv = keywords.find(hashIds[i]);

            if (kv != keywords.end())
            {
                flags[kv->second >> 4] = kv->second & 0xF;
            }
        }

        auto idx = 0u;

        for (auto i = 0u; i < directivecount; ++i)
        {
            idx += directives[i] * flags[i];
        }

        return idx;
    }

    void ShaderVariantMap::Selector::SetKeywordsFrom(const PropertyBlock& block)
    {
        bool value;

        for (auto& kv : map->keywords)
        {
            if (block.TryGet(kv.first, value) && value)
            {
                keywords[kv.second >> 4] = kv.first;
            }
        }
    }

    void Shader::ListVariants()
    {
        PK_LOG_HEADER("Listing variants for shader: %s", GetFileName().c_str());
        m_variantMap.ListVariants();
    }

    void Shader::ListProperties(uint32_t variantIndex)
    {
        if (variantIndex > m_variantMap.variantcount)
        {
            PK_LOG_WARNING("Trying to list properties for variant index (%i) that is out of bounds", variantIndex);
        }

        PK_LOG_NEWLINE();
        PK_LOG_HEADER("Listing properties for shader: %s", GetFileName().c_str());
        m_variants.at(variantIndex)->ListProperties();
        PK_LOG_NEWLINE();
    }

    void Shader::Import(const char* filepath)
    {
        for (auto& variant : m_variants)
        {
            variant->Dispose();
        }

        m_variants.clear();

        PK::Assets::PKAsset asset;

        PK_THROW_ASSERT(PK::Assets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PK::Assets::PKAssetType::Shader, "Trying to read a shader from a non shader file!")

        auto shader = PK::Assets::ReadAsShader(&asset);
        auto base = asset.rawData;

        if (shader->variantcount == 0)
        {
            PK_THROW_ASSERT(shader->variantcount > 0, "Trying to read a shader with 0 variants!");
        }

        m_attributes.blending.srcColorFactor = shader->attributes.blendSrcFactorColor;
        m_attributes.blending.dstColorFactor = shader->attributes.blendDstFactorColor;
        m_attributes.blending.srcAlphaFactor = shader->attributes.blendSrcFactorAlpha;
        m_attributes.blending.dstAlphaFactor = shader->attributes.blendDstFactorAlpha;
        m_attributes.blending.colorOp = shader->attributes.blendOpColor;
        m_attributes.blending.alphaOp = shader->attributes.blendOpAlpha;
        m_attributes.blending.colorMask = (ColorMask)shader->attributes.colorMask;
        m_attributes.depthStencil.depthCompareOp = shader->attributes.ztest;
        m_attributes.depthStencil.depthWriteEnable = shader->attributes.zwrite;
        m_attributes.rasterization.cullMode = shader->attributes.cull;
        m_attributes.rasterization.depthBiasConstantFactor = shader->attributes.zoffsets[0];
        m_attributes.rasterization.depthBiasClamp = shader->attributes.zoffsets[1];
        m_attributes.rasterization.depthBiasSlopeFactor = shader->attributes.zoffsets[2];
        m_attributes.rasterization.depthBiasEnable = shader->attributes.zoffsets[0] != 0.0f ||
                                                     shader->attributes.zoffsets[1] != 0.0f ||
                                                     shader->attributes.zoffsets[2] != 0.0f;

        m_variantMap.variantcount = shader->variantcount;
        m_variantMap.directivecount = 0u;

        auto pKeywords = shader->keywords.Get(base);

        for (auto i = 0u; i < shader->keywordCount; ++i)
        {
            auto pKeyword = pKeywords + i;
            auto hashId = StringHashID::StringToID(pKeyword->name);
            auto o = pKeyword->offsets;
            auto d = (o >> 28) & 0xF;
            m_variantMap.directives[d++] = o & 0xFFFFFF;
            m_variantMap.keywords[hashId] = (o >> 24) & 0xFF;
            m_variantMap.directivecount = d > m_variantMap.directivecount ? d : m_variantMap.directivecount;
        }

        if (shader->materialPropertyCount > 0)
        {
            auto pMaterialProperties = shader->materialProperties.Get(base);

            std::vector<BufferElement> elements;
            elements.reserve(shader->materialPropertyCount);

            for (auto i = 0u; i < shader->materialPropertyCount; ++i)
            {
                auto& prop = pMaterialProperties[i];
                elements.emplace_back(prop.type, prop.name);
            }

            m_materialPropertyLayout = BufferLayout(elements);
        }

        auto api = GraphicsAPI::GetActiveAPI();

        auto pVariants = shader->variants.Get(base);

        for (auto i = 0u; i < shader->variantcount; ++i)
        {
            auto pVariant = pVariants + i;

            switch (api)
            {
                case APIType::Vulkan: m_variants.push_back(CreateRef<VulkanShader>(base, pVariant));
            }
        }

        PK::Assets::CloseAsset(&asset);
    }
}

template<>
bool AssetImporters::IsValidExtension<Shader>(const std::filesystem::path& extension) { return extension.compare(".pkshader") == 0; }

template<>
Ref<Shader> AssetImporters::Create() { return CreateRef<Shader>(); }
