#pragma once
#include "Core/AssetDatabase.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace PK::Rendering::Structs;

    class ShaderVariantMap : public PK::Core::NoCopy
    {
        public:
            void ListVariants();
            inline bool SupportsKeyword(const uint32_t hashId) const { return keywords.count(hashId) > 0; }
            bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const;
            uint32_t GetIndex(const uint32_t* hashIds, size_t count) const;
    
            uint32_t variantcount = 0;
            uint32_t directivecount = 0;
            uint32_t directives[16];
            std::unordered_map<uint32_t, uint8_t> keywords;
    };

    class ShaderVariant : public PK::Core::NoCopy
    {
        friend class Shader;

        public:
            virtual ~ShaderVariant() = default;
            virtual void Release() = 0;

            constexpr const VertexLayout& GetVertexLayout() const { return m_vertexLayout; }
            constexpr const ResourceLayout& GetResourceLayout(uint32_t set) const { return m_resourceLayouts[set]; }
            constexpr const ShaderType GetType() const { return m_type; }

            template<typename T>
            const T* GetNative() const
            { 
                static_assert(std::is_base_of<ShaderVariant, T>::value, "Template argument type does not derive from ShaderVariant!");
                return static_cast<const T*>(this);
            }

        protected:
            VertexLayout m_vertexLayout;
            ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
            ShaderType m_type;
    };

    class Shader : public PK::Core::Asset
    {
        friend Ref<Shader> AssetImporters::Create();

        public:
            constexpr ShaderType GetType() const { return m_variants.at(0)->GetType(); }
            inline const FixedFunctionShaderAttributes& GetFixedFunctionAttributes() const { return m_attributes; }
            inline const ShaderVariant* GetVariant(const uint32_t* keywords, uint32_t count) const { return m_variants[m_variantMap.GetIndex(keywords, count)].get(); }
            inline const ShaderVariant* GetVariant(uint32_t index) const { return m_variants[index].get(); }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_variantMap.SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_variantMap.SupportsKeywords(hashIds, count); }

            void Import(const char* filepath) override final;

        protected:
            std::vector<Ref<ShaderVariant>> m_variants;
            ShaderVariantMap m_variantMap;
            FixedFunctionShaderAttributes m_attributes;
    };
}