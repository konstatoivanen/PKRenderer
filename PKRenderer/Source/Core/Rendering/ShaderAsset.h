#pragma once
#include "Core/Utilities/NativeInterface.h"
#include "Core/Assets/Asset.h"
#include "Core/RHI/Layout.h"
#include "Core/RHI/RHInterfaces.h"

namespace PK
{
    struct ShaderAsset : public AssetWithImport<>
    {
        struct Map : public NoCopy
        {
            constexpr static const uint32_t MAX_DIRECTIVES = PKAssets::PK_ASSET_MAX_SHADER_DIRECTIVES;
            constexpr static const uint32_t MAX_KEYWORDS = PKAssets::PK_ASSET_MAX_SHADER_KEYWORDS;

            int32_t GetKeywordIndex(NameID name) const;
            void AddKeyword(const NameID name, uint8_t value);
            inline bool SupportsKeyword(NameID name) const { return GetKeywordIndex(name) != -1; }
            bool SupportsKeywords(const NameID* names, const uint32_t count) const;
            uint32_t GetIndex(const NameID* names, size_t count) const;
            uint32_t GetIndex(const PropertyBlock* nameblock) const;
            uint32_t GetIndex(const uint8_t* flags) const;

            uint32_t variantcount = 0u;
            uint32_t directivecount = 0u;
            uint32_t directives[MAX_DIRECTIVES];
            
            NameID keywords[MAX_KEYWORDS]{};
            uint8_t keywordValues[MAX_KEYWORDS]{};
            uint8_t offsets[MAX_KEYWORDS]{};
            uint8_t buckets[MAX_KEYWORDS]{};
            uint32_t keywordCount = 0u;
        };

        ~ShaderAsset() { ReleaseVariants(); }

        constexpr ShaderStageFlags GetStageFlags() const { return m_shaders[0]->GetStageFlags(); }
        constexpr const FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }

        inline uint32_t GetRHIIndex(const NameID* keywords, uint32_t count) const { return m_map.GetIndex(keywords, count); }
        inline uint32_t GetRHIIndex(NameID keyword) const { return m_map.GetIndex(&keyword, 1); }
        inline uint32_t GetRHIIndex(const std::initializer_list<NameID>& keywords) const { return GetRHIIndex(keywords.begin(), (uint32_t)(keywords.end() - keywords.begin())); }
        inline uint32_t GetRHIIndex(const PropertyBlock* keywords) const { return m_map.GetIndex(keywords); }

        inline const RHIShader* GetRHI(const NameID* keywords, uint32_t count) const { return m_shaders[GetRHIIndex(keywords, count)].get(); }
        inline const RHIShader* GetRHI(uint32_t index) const { return m_shaders[index].get(); }
        inline const RHIShader* GetRHI(const PropertyBlock* keywords) const { return m_shaders[GetRHIIndex(keywords)].get(); }
        constexpr uint32_t GetRHICount() const { return m_shaders.GetCount(); }

        inline bool SupportsKeyword(const NameID keywords) const { return m_map.SupportsKeyword(keywords); }
        inline bool SupportsKeywords(const NameID* keywords, const uint32_t count) const { return m_map.SupportsKeywords(keywords, count); }
        inline bool SupportsMaterials() const { return m_materialPropertyLayout.size() > 0; }

        constexpr const uint3 GetGroupSize() const { return m_shaders[0]->GetGroupSize(); }
        constexpr const BufferLayout& GetMaterialPropertyLayout() const { return m_materialPropertyLayout; }

        void AssetImport(const char* filepath) final;
        std::string GetMetaInfo() const final;

    protected:
        void ReleaseVariants();

        MemoryBlock<RHIShaderScope, 4ull> m_shaders;
        Map m_map;
        FixedFunctionShaderAttributes m_attributes;
        BufferLayout m_materialPropertyLayout;
    };
}