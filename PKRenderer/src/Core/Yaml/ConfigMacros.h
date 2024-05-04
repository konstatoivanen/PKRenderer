#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Assets/Asset.h"

namespace YAML
{
    struct IYamlConfig
    {
        virtual ~IYamlConfig() = 0 {};
        void YamlLoadFromFile(const std::string& filepath);
        virtual void YamlParse(const Node& parent) = 0;
    };
}

#define PK_YAML_STRUCT_BEGIN(TType) \
    struct TType : public YAML::IYamlConfig \
    { \
        TType() {}; \
    private: \
        typedef TType meta_ThisType; \
        struct meta_FirstId {}; \
        typedef void*(*meta_ParseFunc)(meta_FirstId, meta_ThisType*, const YAML::Node&); \
        static void* meta_ParsePrev(meta_FirstId, meta_ThisType* config, const YAML::Node& node) \
        { \
            return nullptr; \
        } \
        typedef meta_FirstId \

// Needs to be declared out of namespace
#define PK_YAML_ASSET_BEGIN(TType, TExtension) \
    struct TType : public YAML::IYamlConfig, public PK::Core::Assets::AssetWithImport<> \
    { \
        constexpr static const char* Extension = TExtension; \
        TType() {}; \
    private: \
        typedef TType meta_ThisType; \
        struct meta_FirstId {}; \
        typedef void*(*meta_ParseFunc)(meta_FirstId, meta_ThisType*, const YAML::Node&); \
        static void* meta_ParsePrev(meta_FirstId, meta_ThisType* config, const YAML::Node& node) \
        { \
            return nullptr; \
        } \
        typedef meta_FirstId \

#define PK_YAML_MEMBER(TType, TName, TInitialValue) \
        meta_Id##TName; \
    public: TType TName = TInitialValue; \
    private: \
        struct meta_NextId##TName {}; \
        static void* meta_ParsePrev(meta_NextId##TName, meta_ThisType* config, const YAML::Node& node) \
        { \
            if (node[#TName]) \
            { \
                config->TName = node[#TName].as<TType>(); \
            } \
            void*(*PrevFunc)(meta_Id##TName, meta_ThisType*, const YAML::Node&); \
            PrevFunc = meta_ParsePrev; \
            return (void*)PrevFunc; \
        } \
        typedef meta_NextId##TName \

#define PK_YAML_STRUCT_END() \
        meta_LastId; \
    public: \
        inline void YamlParse(const YAML::Node& node) final \
        { \
            void*(*lastParseFunc)(meta_LastId, meta_ThisType*, const YAML::Node&); \
            lastParseFunc = meta_ParsePrev; \
            void* parseFunc = (void*)lastParseFunc; \
            do \
            { \
                parseFunc = reinterpret_cast<meta_ParseFunc>(parseFunc)(meta_FirstId(), this, node); \
            } \
            while (parseFunc); \
        } \
    }; \

#define PK_YAML_ASSET_END() \
        meta_LastId; \
    public: \
        inline void YamlParse(const YAML::Node& node) final \
        { \
            void*(*lastParseFunc)(meta_LastId, meta_ThisType*, const YAML::Node&); \
            lastParseFunc = meta_ParsePrev; \
            void* parseFunc = (void*)lastParseFunc; \
            do \
            { \
                parseFunc = reinterpret_cast<meta_ParseFunc>(parseFunc)(meta_FirstId(), this, node); \
            } \
            while (parseFunc); \
        } \
        inline void AssetImport(const char* filepath) final { YamlLoadFromFile(filepath); } \
    }; \

#define PK_YAML_ASSET_ASSETDATABSE_INTERFACE(type)\
template<> inline bool PK::Core::Assets::Asset::IsValidExtension<type>(const std::string& extension) { return extension.compare(type::Extension) == 0; }\
template<> inline PK::Utilities::Ref<type> PK::Core::Assets::Asset::Create() { return PK::Utilities::CreateRef<type>(); }\
