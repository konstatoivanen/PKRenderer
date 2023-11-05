#pragma once
#include "Utilities/NativeInterface.h"
#include "Utilities/PropertyBlock.h"
#include "Core/Services/AssetDatabase.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Layout.h"

namespace PK::Rendering::RHI::Objects
{
    struct ShaderVariantMap : public Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_DIRECTIVES = 16;

        inline bool SupportsKeyword(const uint32_t hashId) const { return keywords.count(hashId) > 0; }
        bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const;
        uint32_t GetIndex(const uint32_t* hashIds, size_t count) const;

        uint32_t variantcount = 0;
        uint32_t directivecount = 0;
        uint32_t directives[MAX_DIRECTIVES];
        std::unordered_map<uint32_t, uint8_t> keywords;

        struct Selector
        {
            const ShaderVariantMap* map;
            uint32_t keywords[MAX_DIRECTIVES]{};
            void SetKeywordsFrom(const Utilities::PropertyBlock& block);
            inline uint32_t GetIndex() const { return map->GetIndex(keywords, map->directivecount); }
        };
    };

    class ShaderVariant : public Utilities::NoCopy, public Utilities::NativeInterface<ShaderVariant>
    {
        friend class Shader;

        public:
            virtual ~ShaderVariant() = default;
            virtual void Dispose() = 0;

            constexpr const BufferLayout& GetVertexLayout() const { return m_vertexLayout; }
            constexpr const ConstantBufferLayout& GetConstantLayout() const { return m_constantLayout; }
            constexpr const ResourceLayout& GetResourceLayout(uint32_t set) const { return m_resourceLayouts[set]; }
            constexpr const ShaderType GetType() const { return m_type; }
            constexpr const uint32_t GetStageFlags() const { return m_stageFlags; }
            constexpr const Math::uint3& GetGroupSize() const { return m_groupSize; }
            virtual ShaderBindingTableInfo GetShaderBindingTableInfo() const = 0;
            bool HasRayTracingShaderGroup(RayTracingShaderGroup group) const;

        protected:
            BufferLayout m_vertexLayout;
            ConstantBufferLayout m_constantLayout;
            ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
            ShaderType m_type = ShaderType::Graphics;
            uint32_t m_stageFlags = 0u;
            Math::uint3 m_groupSize{};
    };

    class Shader : public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        friend Utilities::Ref<Shader> Core::Services::AssetImporters::Create();

        public:
            constexpr ShaderType GetType() const { return m_variants.at(0)->GetType(); }
            constexpr const FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }
            inline uint32_t GetVariantIndex(const uint32_t* keywords, uint32_t count) const { return m_variantMap.GetIndex(keywords, count); }
            inline uint32_t GetVariantIndex(uint32_t keyword) const { return m_variantMap.GetIndex(&keyword, 1); }
            inline uint32_t GetVariantIndex(const std::initializer_list<uint32_t>& keywords) const { return GetVariantIndex(keywords.begin(), (uint32_t)(keywords.end() - keywords.begin())); }
            inline const ShaderVariant* GetVariant(const uint32_t* keywords, uint32_t count) const { return m_variants[m_variantMap.GetIndex(keywords, count)].get(); }
            inline const ShaderVariant* GetVariant(uint32_t index) const { return m_variants[index].get(); }
            inline ShaderVariantMap::Selector GetVariantSelector() const { return { &m_variantMap }; }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_variantMap.SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_variantMap.SupportsKeywords(hashIds, count); }
            inline bool SupportsMaterials() const { return m_materialPropertyLayout.size() > 0; }
            constexpr const Math::uint3 GetGroupSize() const { return m_variants.at(0)->GetGroupSize(); }
            constexpr const BufferLayout& GetMaterialPropertyLayout() const { return m_materialPropertyLayout; }

            void Import(const char* filepath) final;
            std::string GetMetaInfo() const final;

        protected:
            std::vector<Utilities::Ref<ShaderVariant>> m_variants;
            ShaderVariantMap m_variantMap;
            FixedFunctionShaderAttributes m_attributes;
            BufferLayout m_materialPropertyLayout;
    };
}