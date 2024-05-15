#pragma once
#include "Utilities/NativeInterface.h"
#include "Utilities/PropertyBlock.h"
#include "Core/Assets/Asset.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Layout.h"

namespace PK::Rendering::RHI::Objects
{
    struct ShaderVariantMap : public Utilities::NoCopy
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
            const ShaderVariantMap* map;
            Utilities::NameID keywords[MAX_DIRECTIVES]{};
            void SetKeywordsFrom(const Utilities::PropertyBlock& block);
            inline uint32_t GetIndex() const { return map->GetIndex(keywords, map->directivecount); }
        };
    };

    class ShaderVariant : public Utilities::NoCopy, public Utilities::NativeInterface<ShaderVariant>
    {
        friend struct Shader;

    public:
        virtual ~ShaderVariant() = default;
        virtual void Dispose() = 0;

        constexpr const VertexInputLayout& GetVertexLayout() const { return m_vertexLayout; }
        constexpr const PushConstantLayout& GetPushConstantLayout() const { return m_pushConstantLayout; }
        constexpr const ResourceLayout& GetResourceLayout(uint32_t set) const { return m_resourceLayouts[set]; }
        constexpr const ShaderStageFlags GetStageFlags() const { return m_stageFlags; }
        constexpr const Math::uint3& GetGroupSize() const { return m_groupSize; }
        virtual ShaderBindingTableInfo GetShaderBindingTableInfo() const = 0;
        inline bool HasRayTracingShaderGroup(RayTracingShaderGroup group) const { return (PK_RAYTRACING_GROUP_SHADER_STAGE[(uint32_t)group] & m_stageFlags) != 0; }

    protected:
        VertexInputLayout m_vertexLayout;
        PushConstantLayout m_pushConstantLayout;
        ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
        ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
        Math::uint3 m_groupSize{};
    };

    struct Shader : public Core::Assets::AssetWithImport<>
    {
        constexpr ShaderStageFlags GetStageFlags() const { return m_variants.at(0)->GetStageFlags(); }
        constexpr const FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }
        inline uint32_t GetVariantIndex(const Utilities::NameID* keywords, uint32_t count) const { return m_variantMap.GetIndex(keywords, count); }
        inline uint32_t GetVariantIndex(Utilities::NameID keyword) const { return m_variantMap.GetIndex(&keyword, 1); }
        inline uint32_t GetVariantIndex(const std::initializer_list<Utilities::NameID>& keywords) const { return GetVariantIndex(keywords.begin(), (uint32_t)(keywords.end() - keywords.begin())); }
        inline const ShaderVariant* GetVariant(const Utilities::NameID* keywords, uint32_t count) const { return m_variants[m_variantMap.GetIndex(keywords, count)].get(); }
        inline const ShaderVariant* GetVariant(uint32_t index) const { return m_variants[index].get(); }
        inline ShaderVariantMap::Selector GetVariantSelector() const { return { &m_variantMap }; }
        inline bool SupportsKeyword(const Utilities::NameID keywords) const { return m_variantMap.SupportsKeyword(keywords); }
        inline bool SupportsKeywords(const Utilities::NameID* keywords, const uint32_t count) const { return m_variantMap.SupportsKeywords(keywords, count); }
        inline bool SupportsMaterials() const { return m_materialPropertyLayout.size() > 0; }
        constexpr const Math::uint3 GetGroupSize() const { return m_variants.at(0)->GetGroupSize(); }
        constexpr const BufferLayout& GetMaterialPropertyLayout() const { return m_materialPropertyLayout; }

        void AssetImport(const char* filepath) final;
        std::string GetMetaInfo() const final;

    protected:
        std::vector<Utilities::Ref<ShaderVariant>> m_variants;
        ShaderVariantMap m_variantMap;
        FixedFunctionShaderAttributes m_attributes;
        BufferLayout m_materialPropertyLayout;
    };
}