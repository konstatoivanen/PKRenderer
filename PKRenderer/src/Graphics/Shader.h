#pragma once
#include <unordered_map>
#include "Utilities/NativeInterface.h"
#include "Utilities/PropertyBlock.h"
#include "Core/Assets/Asset.h"
#include "Graphics/RHI/Structs.h"
#include "Graphics/RHI/Layout.h"
#include "Graphics/RHI/RHIShader.h"

namespace PK::Graphics
{
    struct Shader : public Core::Assets::AssetWithImport<>
    {
        struct Map : public Utilities::NoCopy
        {
            constexpr static const uint32_t MAX_DIRECTIVES = 16;

            inline bool SupportsKeyword(const Utilities::NameID name) const { return keywords.count(name) > 0; }
            bool SupportsKeywords(const Utilities::NameID* names, const uint32_t count) const;
            uint32_t GetIndex(const Utilities::NameID* names, size_t count) const;

            uint32_t variantcount = 0;
            uint32_t directivecount = 0;
            uint32_t directives[MAX_DIRECTIVES];
            std::unordered_map<Utilities::NameID, uint8_t> keywords;

            struct Selector
            {
                const Map* map;
                Utilities::NameID keywords[MAX_DIRECTIVES]{};
                void SetKeywordsFrom(const Utilities::PropertyBlock& block);
                inline uint32_t GetIndex() const { return map->GetIndex(keywords, map->directivecount); }
            };
        };

        constexpr RHI::ShaderStageFlags GetStageFlags() const { return m_shaders.at(0)->GetStageFlags(); }
        constexpr const RHI::FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }
        inline uint32_t GetRHIIndex(const Utilities::NameID* keywords, uint32_t count) const { return m_map.GetIndex(keywords, count); }
        inline uint32_t GetRHIIndex(Utilities::NameID keyword) const { return m_map.GetIndex(&keyword, 1); }
        inline uint32_t GetRHIIndex(const std::initializer_list<Utilities::NameID>& keywords) const { return GetRHIIndex(keywords.begin(), (uint32_t)(keywords.end() - keywords.begin())); }
        inline const RHI::RHIShader* GetRHI(const Utilities::NameID* keywords, uint32_t count) const { return m_shaders[m_map.GetIndex(keywords, count)].get(); }
        inline const RHI::RHIShader* GetRHI(uint32_t index) const { return m_shaders[index].get(); }
        inline Map::Selector GetRHISelector() const { return { &m_map }; }
        inline bool SupportsKeyword(const Utilities::NameID keywords) const { return m_map.SupportsKeyword(keywords); }
        inline bool SupportsKeywords(const Utilities::NameID* keywords, const uint32_t count) const { return m_map.SupportsKeywords(keywords, count); }
        inline bool SupportsMaterials() const { return m_materialPropertyLayout.size() > 0; }
        constexpr const Math::uint3 GetGroupSize() const { return m_shaders.at(0)->GetGroupSize(); }
        constexpr const RHI::BufferLayout& GetMaterialPropertyLayout() const { return m_materialPropertyLayout; }

        void AssetImport(const char* filepath) final;
        std::string GetMetaInfo() const final;

    protected:
        std::vector<RHI::RHIShaderScope> m_shaders;
        Map m_map;
        RHI::FixedFunctionShaderAttributes m_attributes;
        RHI::BufferLayout m_materialPropertyLayout;
    };
}