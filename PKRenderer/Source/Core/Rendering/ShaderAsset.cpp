#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Core/Utilities/PropertyBlock.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"
#include "ShaderAsset.h"

namespace PK
{
    void ShaderAsset::Map::AddKeyword(const NameID name, uint8_t directive, uint8_t value)
    {
        if (keywordCount == 0u)
        {
            memset(buckets, 255, sizeof(buckets));
        }

        static_assert(MAX_KEYWORDS == 256u, "8 bit buckets dont work with more than 256 elements.");

        // Assumes unique keywords. Doesnt handle duplicates
        auto bucketIndex = name.identifier % MAX_KEYWORDS;
        auto valueIndex = buckets[bucketIndex];

        if (valueIndex < 255u)
        {
            offsets[keywordCount] = valueIndex;
        }

        keywordValues[keywordCount] = (value & 0xFu) | (directive << 4u);
        keywords[keywordCount] = name;
        offsets[keywordCount] = keywordCount;
        buckets[bucketIndex] = keywordCount;
        keywordCount++;
    }

    int32_t ShaderAsset::Map::GetKeywordIndex(NameID name) const
    {
        if (keywordCount == 0)
        {
            return -1;
        }

        auto index = buckets[name.identifier % MAX_KEYWORDS];

        while (keywords[index] != name && offsets[index] != index)
        {
            index = offsets[index];
        }

        return keywords[index] == name ? index : -1;
    }

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
            auto index = GetKeywordIndex(names[i]);

            if (index != -1)
            {
                auto value = keywordValues[index];
                flags[value >> 4] = value & 0xF;
            }
        }

        return GetIndex(flags);
    }

    uint32_t ShaderAsset::Map::GetIndex(const PropertyBlock* nameblock) const
    {
        uint8_t flags[MAX_DIRECTIVES]{};

        for (auto i = 0u; i < keywordCount; ++i)
        {
            bool enabled;
            if (nameblock->TryGet(keywords[i], enabled) && enabled)
            {
                auto value = keywordValues[i];
                flags[value >> 4] = value & 0xF;
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


    ShaderAsset::ShaderAsset(const char* filepath)
    {
        ReleaseVariants();

        PKAssets::PKAsset asset;

        PK_FATAL_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header->type == PKAssets::PKAssetType::Shader, "Trying to read a shader from a non shader file!")

        auto shader = PKAssets::ReadAsShader(&asset);
        auto base = asset.rawData;

        if (shader->variantcount == 0)
        {
            PK_FATAL_ASSERT(shader->variantcount > 0, "Trying to read a shader with 0 variants!");
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
            m_map.directives[pKeyword->directive] = (uint8_t)pKeyword->offset;
            m_map.AddKeyword(pKeyword->name, pKeyword->directive, pKeyword->value);
            m_map.directivecount = pKeyword->directive + 1u;
        }

        if (shader->materialPropertyCount > 0)
        {
            m_materialPropertyLayout.ClearFast();
            m_materialPropertyLayout.Reserve(shader->materialPropertyCount + 1u);

            auto pMaterialProperties = shader->materialProperties.Get(base);

            for (auto i = 0u; i < shader->materialPropertyCount; ++i)
            {
                auto& prop = pMaterialProperties[i];
                m_materialPropertyLayout.Add({ prop.type, prop.name });
            }

            m_materialPropertyLayout.CalculateOffsetsAndStride();
        }

        auto pVariants = shader->variants.Get(base);
        auto fileName = String::ToFilePathStem<64>(filepath);

        m_shaders.Reserve(shader->variantcount);

        for (auto i = 0u; i < shader->variantcount; ++i)
        {
            m_shaders[i] = RHI::CreateShader(base, pVariants + i, FixedString128("%s%u", fileName, i));
        }

        PKAssets::CloseAsset(&asset);
    }

    const char* ShaderAsset::GetMetaInfo() const
    {
        static FixedString1024 meta;
        meta.Clear();
        meta.Append("Asset Metadata: \n");
        meta.Append("   Type: Shader\n");
        meta.Append("   Name: ");
        meta.AppendFormat("   Name: %s \n", GetFileName());

        if (m_materialPropertyLayout.GetCount() > 0)
        {
            meta.Append("\nMaterial Properties:\n");

            for (const auto& prop : m_materialPropertyLayout)
            {
                meta.AppendFormat("   %s,%s,%u,%u\n", prop.name.c_str(), RHIEnumConvert::ElementTypeToString(prop.format), prop.count, prop.offset);
            }
        }

        meta.Append("\nVariants:\n");

        NameID keywordlist[Map::MAX_DIRECTIVES][PKAssets::PK_ASSET_MAX_SHADER_DIRECTIVE_SIZE]{};
        uint32_t directiveSizes[Map::MAX_DIRECTIVES]{};

        for (auto i = 0u; i < m_map.keywordCount; ++i)
        {
            const auto& keyword = m_map.keywords[i].c_str();
            auto value = m_map.keywordValues[i];
            auto index0 = (value >> 4u) & 0xFu;
            auto index1 = value & 0xFu;
            keywordlist[index0][index1] = keyword;
            directiveSizes[index0] = index1 + 1u;
        }

        for (auto j = 0u; j < m_map.variantcount; ++j)
        {
            auto index = j;
            meta.AppendFormat("   Variant: %u\n", j);
            meta.Append("       Keywords:");

            for (auto i = 0u; i < m_map.directivecount; ++i)
            {
                auto& declares = keywordlist[i];
                auto& keyword = declares[index % directiveSizes[i]];

                if (keyword != 0u)
                {
                    meta.Append(keyword.c_str());
                    meta.Append(' ');
                }

                index /= directiveSizes[i];
            }

            meta.Append('\n');

            const auto& shader = m_shaders[j].get();

            meta.Append("       Vertex Attributes:\n");

            for (const auto& element : shader->GetVertexLayout())
            {
                meta.AppendFormat("         %s, %u, %u\n", element.name.c_str(), element.location, (uint32_t)element.format);
            }

            meta.Append("       Dynamic Constants:\n");

            for (const auto& element : shader->GetPushConstantLayout())
            {
                meta.AppendFormat("         %s, %u, %u\n", element.name.c_str(), element.offset, (uint32_t)element.size);
            }

            meta.Append("       Descriptor Set:\n");

            for (const auto& element : shader->GetResourceLayout())
            {
                meta.AppendFormat("         %s, %u\n", element.name.c_str(), (uint32_t)element.type);
            }
        }

        return meta.c_str();
    }

    void ShaderAsset::ReleaseVariants()
    {
        for (auto i = 0u; i < m_shaders.GetCount(); ++i)
        {
            m_shaders[i] = nullptr;
        }

        m_shaders.Clear();
    }
}

template<>
const char* PK::Asset::GetExtension<PK::ShaderAsset>() { return "*.pkshader"; }
