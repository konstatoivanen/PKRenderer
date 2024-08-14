#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAssetLoader.h>
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/PropertyBlock.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "ShaderAsset.h"

namespace PK
{
    bool ShaderAsset::Map::SupportsKeywords(const NameID* names, const uint32_t count) const
    {
        for (auto i = 0u; i < count; ++i)
        {
            if (!SupportsKeyword(names[i]))
            {
                return false;
            }
        }

        return true;
    }

    uint32_t ShaderAsset::Map::GetIndex(const NameID* names, size_t count) const
    {
        uint8_t flags[MAX_DIRECTIVES]{};

        for (auto i = 0u; i < count; ++i)
        {
            auto kv = keywords.find(names[i]);

            if (kv != keywords.end())
            {
                flags[kv->second >> 4] = kv->second & 0xF;
            }
        }

        return GetIndex(flags);
    }

    uint32_t ShaderAsset::Map::GetIndex(const PropertyBlock* nameblock) const
    {
        uint8_t flags[MAX_DIRECTIVES]{};

        for (auto& kv : keywords)
        {
            bool value;
            if (nameblock->TryGet(kv.first, value) && value)
            {
                flags[kv.second >> 4] = kv.second & 0xF;
            }
        }

        return GetIndex(flags);
    }

    uint32_t ShaderAsset::Map::GetIndex(const uint8_t* flags) const
    {
        auto idx = 0u;

        for (auto i = 0u; i < directivecount; ++i)
        {
            idx += directives[i] * flags[i];
        }

        return idx;
    }

    void ShaderAsset::AssetImport(const char* filepath)
    {
        m_shaders.clear();

        PKAssets::PKAsset asset;

        PK_THROW_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PKAssets::PKAssetType::Shader, "Trying to read a shader from a non shader file!")

        auto shader = PKAssets::ReadAsShader(&asset);
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

        m_map.variantcount = shader->variantcount;
        m_map.directivecount = 0u;

        auto pKeywords = shader->keywords.Get(base);

        for (auto i = 0u; i < shader->keywordCount; ++i)
        {
            auto pKeyword = pKeywords + i;
            auto o = pKeyword->offsets;
            auto d = (o >> 28) & 0xF;
            m_map.directives[d++] = o & 0xFFFFFF;
            m_map.keywords[pKeyword->name] = (o >> 24) & 0xFF;
            m_map.directivecount = d > m_map.directivecount ? d : m_map.directivecount;
        }

        if (shader->materialPropertyCount > 0)
        {
            m_materialPropertyLayout.clear();
            m_materialPropertyLayout.reserve(shader->materialPropertyCount + 1u);

            auto pMaterialProperties = shader->materialProperties.Get(base);

            for (auto i = 0u; i < shader->materialPropertyCount; ++i)
            {
                auto& prop = pMaterialProperties[i];
                m_materialPropertyLayout.push_back({ prop.type, prop.name });
            }

            m_materialPropertyLayout.CalculateOffsetsAndStride(true);
        }

        auto pVariants = shader->variants.Get(base);
        auto fileName = std::filesystem::path(GetFileName()).stem().string();

        for (auto i = 0u; i < shader->variantcount; ++i)
        {
            auto name = fileName + std::to_string(i);
            m_shaders.push_back(RHI::CreateShader(base, pVariants + i, name.c_str()));
        }

        PKAssets::CloseAsset(&asset);
    }

    std::string ShaderAsset::GetMetaInfo() const
    {
        std::string meta = "Asset Metadata: \n";
        meta.append("   Type: Shader\n");
        meta.append("   Name: " + std::string(GetFileName()) + "\n");

        if (m_materialPropertyLayout.size() > 0)
        {
            meta.append("\nMaterial Properties:\n");

            for (const auto& prop : m_materialPropertyLayout)
            {
                meta.append(Parse::FormatToString("   %s,%u,%u,%u,%u,%u\n", 
                    prop.name.c_str(), 
                    (uint32_t)prop.format, 
                    prop.count, 
                    prop.location, 
                    prop.offset, 
                    prop.alignedOffset));
            }
        }

        meta.append("\nVariants:\n");

        std::vector<std::vector<std::string>> keywordlist;

        for (auto& kv : m_map.keywords)
        {
            const auto& keyword = kv.first.c_str();
            auto index0 = (kv.second >> 4u) & 0xFu;
            auto index1 = kv.second & 0xFu;

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

        for (auto j = 0u; j < m_map.variantcount; ++j)
        {
            auto index = j;
            meta.append(Parse::FormatToString("   Variant: %u\n", j));
            meta.append("       Keywords:");

            for (auto i = 0u; i < keywordlist.size(); ++i)
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

            const auto& shader = m_shaders.at(j);

            meta.append("       Vertex Attributes:\n");

            for (auto& element : shader->GetVertexLayout())
            {
                meta.append(Parse::FormatToString("       %s, %u, %u\n", + element->name.c_str(), element->location, (uint32_t)element->format));
            }

            meta.append("       Dynamic Constants:\n");

            for (auto& element : shader->GetPushConstantLayout())
            {
                meta.append(Parse::FormatToString("       %s, %u, %u\n", element->name.c_str(), element->offset, (uint32_t)element->size));
            }

            for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTOR_SETS; ++i)
            {
                auto& set = shader->GetResourceLayout(i);

                meta.append(Parse::FormatToString("       Descriptor Set %u:\n", i));

                for (auto& element : set)
                {
                    meta.append(Parse::FormatToString("       %s, %u\n", element->name.c_str(), (uint32_t)element->type));
                }
            }
        }

        return meta;
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::ShaderAsset>(const char* extension) { return strcmp(extension, ".pkshader") == 0; }

template<>
PK::Ref<PK::ShaderAsset> PK::Asset::Create() { return CreateRef<PK::ShaderAsset>(); }
