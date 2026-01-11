#pragma once
#include <filesystem>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/FixedArena.h"
#include "Core/Utilities/Hash.h"
#include "Core/Utilities/FastTypeIndex.h"
#include "Core/Assets/Asset.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"

namespace PK
{
    enum class AssetCachingMode
    {
        ReferenceCounted,
        GC,
        Persistent
    };

    class AssetDatabase : public ISingleton<AssetDatabase>
    {
        constexpr static uint32_t INVALID_LINK = ~0u;

        struct TypeInfo
        {
            uint32_t typeIndex;
            uint32_t headIndex;
            const char* name;
            const char* shortName;
            IAssetFactory* factory;
            
            bool operator == (const TypeInfo& other) { return typeIndex == other.typeIndex; }
            bool operator != (const TypeInfo& other) { return typeIndex != other.typeIndex; }
        };

        struct AssetObjectBase : public Asset::SharedObject 
        {
            TypeInfo* typeInfo;
            uint32_t indexNext;
            uint16_t cachingMode;
            bool isVirtual;
            bool isLoaded;

            constexpr bool IsReferenceReleasable() const { return isLoaded && !GetStrongRefCount() && cachingMode == (uint16_t)AssetCachingMode::ReferenceCounted; }
            constexpr bool IsGarbageCollectable() const { return isLoaded && !GetStrongRefCount() && cachingMode == (uint16_t)AssetCachingMode::GC; }
            constexpr bool IsPersistent() const { return isLoaded && cachingMode == (uint16_t)AssetCachingMode::Persistent; }
            Ref<Asset> GetBaseReference() { return Ref<Asset>(this, isLoaded ? GetAsset() : nullptr); }
            virtual void ConstructAsset(AssetDatabase* caller, const char* filepath) noexcept = 0;
            virtual void DestructAsset() noexcept = 0;
            virtual Asset* GetAsset() noexcept = 0;
        };

        template<typename T>
        struct AssetObject : public AssetObjectBase
        {
            explicit AssetObject() 
            {
                referenceCount = 0u;
                weakCount = 1u; 
            }

            virtual ~AssetObject() noexcept override {}

            template <typename ... Args>
            void ConstructVirtual(Args&&... args) 
            { 
                new(&value) T(std::forward<Args>(args)...); 
                value.m_sharedObject = this;
                isLoaded = true; 
                isVirtual = true;
            }

            void ConstructAsset(AssetDatabase* caller, const char* filepath) noexcept final
            {
                if constexpr (std::is_constructible<T, const char*>::value)
                {
                    new(&value) T(filepath);
                }
                else if (typeInfo->factory)
                {
                    static_cast<AssetFactory<T>*>(typeInfo->factory)->AssetConstruct(&value, filepath);
                }

                value.m_sharedObject = this;
                isLoaded = true;
                isVirtual = false;
                version++;

                AssetImportEvent<T> importToken = { caller, &value };
                caller->m_sequencer->Next(caller, &importToken);
            }

            void DestructAsset() noexcept final
            {
                if (isLoaded)
                {
                    value.~T();
                    isLoaded = false;
                }
            }

            void Destroy() noexcept final 
            { 
                if (IsReferenceReleasable())
                {
                    DestructAsset();
                }

                IncrementWeakRef();
            }

            void Delete() noexcept final { delete this; }

            Asset* GetAsset() noexcept final { return &value; };

            Ref<T> GetReference() { return Ref<T>(this, &value); }

            struct U { constexpr U() noexcept {} };
            union { U unionDefault; T value; };
        };

        struct TypeInfoHash { size_t operator()(const TypeInfo& k) const noexcept { return (size_t)(k.typeIndex);}};
        struct AssetObjectHash { size_t operator()(const AssetObjectBase* k) const noexcept { return (size_t)(k->assetId);}};

    public:
        AssetDatabase(Sequencer* sequencer);
        ~AssetDatabase();

        template<typename T>
        void RegisterFactory(IAssetFactory* factory) { CreateTypeInfo<T>()->factory = factory; }

        template<typename T, typename ... Args>
        Ref<T> CreateVirtual(AssetID assetId, Args&& ... args)
        {
            auto object = CreateAssetObject<T>(assetId);
            PK_THROW_ASSERT(!object->isLoaded, "AssetDatabase.Register: (%s) already exists!", assetId.c_str());
            PK_LOG_VERBOSE_FUNC("%s, %s", typeid(T).name(), assetId.c_str());
            object->ConstructVirtual(std::forward<Args>(args)...);
            return object->GetReference();
        }

        template<typename T, typename ... Args>
        Ref<T> CreateVirtual(const char* name, Args&& ... args)
        {
            return CreateVirtual<T>(AssetID(name), std::forward<Args>(args)...);
        }

        template<typename T>
        Ref<T> Load(AssetID assetId, bool forceReload = false) 
        {
            FixedString256 filepath(assetId.c_str());
            PK_THROW_ASSERT(std::filesystem::exists(filepath.c_str()), "Asset not found at path: %s", filepath.c_str());
            AssetObject<T>* object = CreateAssetObject<T>(assetId);
            LoadAsset(object, forceReload);
            return object->GetReference();
        }

        template<typename T>
        Ref<T> Load(const char* filepath, bool forceReload = false) { return Load<T>(AssetID(filepath), forceReload); }

        template<typename T>
        void LoadDirectory(const std::string& directory, bool forceReload = false)
        {
            PK_LOG_VERBOSE_FUNC("%s, %s", typeid(T).name(), directory.c_str());

            if (std::filesystem::exists(directory))
            {
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (Asset::IsValidExtension<T>(entry.path().extension().string().c_str()))
                    {
                        Load<T>(AssetID(entry.path().string().c_str()), forceReload);
                    }
                }
            }
        }

        template<typename T>
        void ReloadDirectoryByType(const char* directory) { ReloadDirectoryByType(pk_base_type_index<T>()), directory); }
       
        template<typename T>
        void ReloadByType() { ReloadByType(pk_base_type_index<T>()); }

        template<typename T>
        void UnloadDirectoryByType(const char* directory) { UnloadDirectoryByType(pk_base_type_index<T>()), directory); }
       
        template<typename T>
        void UnloadByType() { UnloadByType(pk_base_type_index<T>()); }

        template<typename T>
        void LogAssetDirectoryByType(const char* directory) { LogAssetDirectoryByType(pk_base_type_index<T>()), directory); }
       
        template<typename T>
        void LogAssetByType() { LogAssetByType(pk_base_type_index<T>()); }

        template<typename T>
        Ref<T> Find(const char* name, bool doAssert = true) const
        {
            auto asset = Find(pk_base_type_index<T>(), name);
            PK_THROW_ASSERT(!doAssert || asset != nullptr, "Could not find asset with name %s", name);
            return asset ? StaticCastRef<T>(asset) : nullptr;
        }
        
        Ref<Asset> Find(uint32_t typeIndex, const char* keyword) const;

        void Reload(AssetID assetId);
        void ReloadDirectory(const char* directory);
        void ReloadDirectoryByType(uint32_t typeIndex, const char* directory);
        void ReloadByType(uint32_t typeIndex);
        void ReloadAll();

        void Unload(AssetID assetId);
        void UnloadDirectory(const char* directory);
        void UnloadDirectoryByType(uint32_t typeIndex, const char* directory);
        void UnloadByType(uint32_t typeIndex);
        void UnloadAll();
        void GC();

        void LogDirectory(const char* directory);
        void LogDirectoryByType(uint32_t typeIndex, const char* directory);
        void LogByType(uint32_t typeIndex);
        void LogAll();

    private:
        template<typename T>
        TypeInfo* CreateTypeInfo() { return CreateTypeInfo(pk_base_type_index<T>(), std::type_index(typeid(T))); }

        template<typename T>
        AssetObject<T>* CreateAssetObject(AssetID assetId)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            auto assetIndex = m_assets.GetHashIndex((size_t)assetId);

            if (assetIndex == -1)
            {
                auto typeInfo = CreateTypeInfo<T>();
                auto newAsset = new AssetObject<T>();
                newAsset->typeInfo = typeInfo;
                newAsset->assetId = assetId;
                newAsset->version = 0u;
                newAsset->indexNext = typeInfo->headIndex;
                newAsset->cachingMode = (uint16_t)AssetCachingMode::Persistent;
                newAsset->isLoaded = false;
                newAsset->isVirtual = false;
                assetIndex = m_assets.Add(newAsset);
                typeInfo->headIndex = assetIndex;
                return newAsset;
            }

            return static_cast<AssetObject<T>*>(m_assets[assetIndex]);
        }

        void LoadAsset(AssetObjectBase* object, bool isReload);
        uint32_t GetTypeHead(uint32_t typeIndex) const;
        TypeInfo* CreateTypeInfo(uint32_t typeIndex, const std::type_index& rttiIndex);

        FastSet<AssetObjectBase*, AssetObjectHash> m_assets;
        FastSet<TypeInfo, TypeInfoHash> m_types;
        Sequencer* m_sequencer;
    };
}
