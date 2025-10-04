#pragma once
#include <string>
#include "Core/Yaml/RapidyamlFwd.h"
#include "Core/Assets/Asset.h"

namespace PK
{
    struct IYamlStruct
    {
        void YamlLoadFromFile(const std::string& filepath);

        protected: 
            virtual void ParseStruct(const YAML::ConstNode& node) = 0;

            static void ParseMemberStruct(const YAML::ConstNode& node, const char* memberName, IYamlStruct* outValue);
    };
}

#define PK_YAML_STRUCT_BEGIN(TType) \
    struct TType : public PK::IYamlStruct \
    { \
        TType() {}; \
    private: \
        typedef TType meta_ThisType; \
        struct meta_FirstId {}; \
        typedef void*(*meta_ParseFunc)(meta_FirstId, meta_ThisType*, const YAML::ConstNode&); \
        static void* meta_ParsePrev(meta_FirstId, [[maybe_unused]] meta_ThisType* config, [[maybe_unused]] const YAML::ConstNode& node) \
        { \
            return nullptr; \
        } \
        typedef meta_FirstId \

// Needs to be declared out of namespace
#define PK_YAML_ASSET_BEGIN(TType, TExtension) \
    struct TType : public PK::IYamlStruct, public PK::AssetWithImport<> \
    { \
        constexpr static const char* Extension = TExtension; \
        TType() {}; \
        TType(const char* filepath) { AssetImport(filepath); }; \
    private: \
        typedef TType meta_ThisType; \
        struct meta_FirstId {}; \
        typedef void*(*meta_ParseFunc)(meta_FirstId, meta_ThisType*, const YAML::ConstNode&); \
        static void* meta_ParsePrev(meta_FirstId, [[maybe_unused]] meta_ThisType* config, [[maybe_unused]] const YAML::ConstNode& node) \
        { \
            return nullptr; \
        } \
        typedef meta_FirstId \

#define PK_YAML_MEMBER(TType, TName, TInitialValue) \
        meta_Id##TName; \
    public: TType TName = TInitialValue; \
    private: \
        struct meta_NextId##TName {}; \
        static void* meta_ParsePrev(meta_NextId##TName, meta_ThisType* config, const YAML::ConstNode& node) \
        { \
            YAML::Read(node, #TName, &config->TName); \
            void*(*PrevFunc)(meta_Id##TName, meta_ThisType*, const YAML::ConstNode&); \
            PrevFunc = meta_ParsePrev; \
            return (void*)PrevFunc; \
        } \
        typedef meta_NextId##TName \

#define PK_YAML_MEMBER_STRUCT(TType, TName) \
        meta_Id##TName; \
    public: TType TName{}; \
    private: \
        struct meta_NextId##TName {}; \
        static void* meta_ParsePrev(meta_NextId##TName, meta_ThisType* config, const YAML::ConstNode& node) \
        { \
            ParseMemberStruct(node, #TName, &config->TName); \
            void*(*PrevFunc)(meta_Id##TName, meta_ThisType*, const YAML::ConstNode&); \
            PrevFunc = meta_ParsePrev; \
            return (void*)PrevFunc; \
        } \
        typedef meta_NextId##TName \

#define PK_YAML_STRUCT_END() \
        meta_LastId; \
    public: \
        void ParseStruct(const YAML::ConstNode& node) final \
        { \
            void*(*lastParseFunc)(meta_LastId, meta_ThisType*, const YAML::ConstNode&); \
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
        void ParseStruct(const YAML::ConstNode& node) final \
        { \
            void*(*lastParseFunc)(meta_LastId, meta_ThisType*, const YAML::ConstNode&); \
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
template<> inline bool PK::Asset::IsValidExtension<type>(const char* extension) { return strcmp(extension, type::Extension) == 0; }\
template<> inline PK::Ref<type> PK::Asset::Create() { return PK::CreateRef<type>(); }\

