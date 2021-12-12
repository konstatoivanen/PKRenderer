#pragma once
#include "Core/AssetDatabase.h"
#include "Core/NativeInterface.h"
#include "Core/PropertyBlock.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace PK::Rendering::Structs;

    class ShaderVariantMap : public NoCopy
    {
        public:
            constexpr static const int MAX_DIRECTIVES = 16;

            void ListVariants();
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
                void SetKeywordsFrom(const PropertyBlock& block);
                inline uint32_t GetIndex() const { return map->GetIndex(keywords, map->directivecount); }
            };
    };

    class ShaderVariant : public NoCopy, public NativeInterface<ShaderVariant>
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

            void ListProperties();

        protected:
            BufferLayout m_vertexLayout;
            ConstantBufferLayout m_constantLayout;
            ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
            ShaderType m_type;
            uint32_t m_stageFlags;
    };

    class Shader : public Asset
    {
        friend Ref<Shader> AssetImporters::Create();

        public:
            constexpr ShaderType GetType() const { return m_variants.at(0)->GetType(); }
            inline const FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }
            inline const ShaderVariant* GetVariant(const uint32_t* keywords, uint32_t count) const { return m_variants[m_variantMap.GetIndex(keywords, count)].get(); }
            inline const ShaderVariant* GetVariant(uint32_t index) const { return m_variants[index].get(); }
            inline ShaderVariantMap::Selector GetVariantSelector() const { return { &m_variantMap }; }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_variantMap.SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_variantMap.SupportsKeywords(hashIds, count); }

            void ListVariants();
            void ListProperties(uint32_t variantIndex);

            void Import(const char* filepath) override final;

        protected:
            std::vector<Ref<ShaderVariant>> m_variants;
            ShaderVariantMap m_variantMap;
            FixedFunctionShaderAttributes m_attributes;
    };
}