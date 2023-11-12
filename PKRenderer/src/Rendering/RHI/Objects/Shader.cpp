#pragma once
#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Rendering/RHI/Vulkan/Objects/VulkanShader.h"
#include "Rendering/RHI/Driver.h"
#include "Shader.h"

using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::RHI::Vulkan::Objects;
using namespace PK::Rendering::RHI::Objects;

namespace PK::Rendering::RHI::Objects
{
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
        m_attributes.rasterization.rasterMode = shader->attributes.rasterMode;
        m_attributes.rasterization.overEstimation = shader->attributes.overEstimation;
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

            auto elements = PK_STACK_ALLOC(BufferElement, shader->materialPropertyCount);

            for (auto i = 0u; i < shader->materialPropertyCount; ++i)
            {
                auto& prop = pMaterialProperties[i];
                elements[i] = { prop.type, prop.name };
            }

            m_materialPropertyLayout = BufferLayout(elements, shader->materialPropertyCount);
        }

        auto api = Driver::Get()->GetAPI();
        auto pVariants = shader->variants.Get(base);
        auto fileName = std::filesystem::path(GetFileName()).stem().string();

        for (auto i = 0u; i < shader->variantcount; ++i)
        {
            auto pVariant = pVariants + i;
            auto name = fileName + std::to_string(i);

            switch (api)
            {
                case APIType::Vulkan: m_variants.push_back(CreateRef<VulkanShader>(base, pVariant, name.c_str()));
            }
        }

        PK::Assets::CloseAsset(&asset);
    }

    std::string Shader::GetMetaInfo() const
    {
        std::string meta = "Asset Metadata: \n";
        meta.append("   Type: Shader\n");
        meta.append("   Name: " + GetFileName() + "\n");

        if (m_materialPropertyLayout.size() > 0)
        {
            meta.append("\nMaterial Properties:\n");

            for (const auto& prop : m_materialPropertyLayout)
            {
                meta.append("   " + StringHashID::IDToString(prop.NameHashId));
                meta.append("," + std::to_string((uint32_t)prop.Type));
                meta.append("," + std::to_string(prop.Count));
                meta.append("," + std::to_string(prop.Location));
                meta.append("," + std::to_string(prop.Offset));
                meta.append("," + std::to_string(prop.AlignedOffset) + "\n");
            }
        }

        meta.append("\nVariants:\n");

        std::vector<std::vector<std::string>> keywordlist;

        for (auto& kv : m_variantMap.keywords)
        {
            const auto& keyword = StringHashID::IDToString(kv.first);
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

        for (auto j = 0u; j < m_variantMap.variantcount; ++j)
        {
            auto index = j;
            meta.append("   Variant: " + std::to_string(j) + "\n");
            meta.append("       Keywords:");

            for (auto i = 0; i < keywordlist.size(); ++i)
            {
                auto& declares = keywordlist.at(i);
                auto& keyword = declares.at(index % declares.size());

                if (!keyword.empty())
                {
                    meta.append(keyword);
                    meta.append(" ");
                }

                index /= (uint32_t)declares.size();
            }

            meta.append("\n");

            const auto& variant = m_variants.at(j);

            meta.append("       Vertex Attributes:\n");

            for (auto& element : variant->GetVertexLayout())
            {
                meta.append("       " + StringHashID::IDToString(element.NameHashId) + ", " + std::to_string(element.Location) + ", " + std::to_string((uint32_t)element.Type) + "\n");
            }

            meta.append("       Dynamic Constants:\n");

            for (auto& kv : variant->GetConstantLayout())
            {
                auto& element = kv.second;
                meta.append("       " + StringHashID::IDToString(element.NameHashId) + ", " + std::to_string(element.Offset) + ", " + std::to_string((uint32_t)element.Size) + "\n");
            }

            for (auto i = 0u; i < PK_MAX_DESCRIPTOR_SETS; ++i)
            {
                auto& set = variant->GetResourceLayout(i);

                meta.append("       Descriptor Set " + std::to_string(i) + ":\n");

                for (auto& element : set)
                {
                    meta.append("       " + StringHashID::IDToString(element->NameHashId) + ", " + std::to_string((uint32_t)element->Type) + "\n");
                }
            }
        }

        return meta;
    }
}

template<>
bool AssetImporters::IsValidExtension<Shader>(const std::filesystem::path& extension) { return extension.compare(".pkshader") == 0; }

template<>
Ref<Shader> AssetImporters::Create() { return CreateRef<Shader>(); }
