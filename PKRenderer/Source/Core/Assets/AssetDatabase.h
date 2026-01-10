#pragma once
#include <filesystem>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/FixedArena.h"
#include "Core/Utilities/Hash.h"
#include "Core/Assets/Asset.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"

namespace PK
{
    class AssetDatabase : public ISingleton<AssetDatabase>
    {
        constexpr static uint32_t INVALID_LINK = ~0u;

        struct AssetObjectBase : public Asset::SharedObject 
        {
            uint32_t indexNext;
            bool isVirtual;
            bool isLoaded;

            virtual void Construct(AssetDatabase* caller, const char* filepath) noexcept = 0;
            virtual Asset* GetAsset() noexcept = 0;
            virtual const char* GetTypeName() const noexcept = 0;
        };

        template<typename T>
        struct AssetObject : public AssetObjectBase
        {
            explicit AssetObject() {}
            virtual ~AssetObject() noexcept override {}

            template <typename ... Args>
            void ConstructVirtual(Args&&... args) 
            { 
                new(&value) T(std::forward<Args>(args)...); 
                value.m_sharedObject = this;
                isLoaded = true; 
                isVirtual = true;
            }

            void Construct(AssetDatabase* caller, const char* filepath) noexcept final
            {
                if constexpr (std::is_constructible<T, const char*>::value)
                {
                    new(&value) T(filepath);
                }
                else if (factory)
                {
                    factory->AssetConstruct(&value, filepath);
                }

                value.m_sharedObject = this;
                isLoaded = true;
                isVirtual = false;
                version++;

                AssetImportEvent<T> importToken = { caller, &value };
                caller->m_sequencer->Next(caller, &importToken);
            }

            void Destroy() noexcept final 
            { 
                if (isLoaded)
                {
                    value.~T(); 
                    isLoaded = false; 
                }
            }

            void Delete() noexcept final { delete this; }
            Asset* GetAsset() noexcept final { return &value; };
            const char* GetTypeName() const noexcept final { return typeid(T).name(); }
            struct U { constexpr U() noexcept {} };
            union { U unionDefault; T value; };
            AssetFactory<T>* factory;
        };

        struct TypeInfo
        {
            uint32_t headIndex;
            IAssetFactory* factory;
        };

        struct AssetObjectHash
        {
            size_t operator()(const AssetObjectBase* k) const noexcept
            {
                return (size_t)(k->assetId);
            }
        };

    public:
        AssetDatabase(Sequencer* sequencer);
        ~AssetDatabase();

        template<typename T, typename ... Args>
        T* CreateVirtual(AssetID assetId, Args&& ... args)
        {
            auto object = CreateAssetObject<T>(assetId);
            PK_THROW_ASSERT(!object->isLoaded, "AssetDatabase.Register: (%s) already exists!", assetId.c_str());
            PK_LOG_VERBOSE_FUNC("%s, %s", typeid(T).name(), assetId.c_str());
            object->ConstructVirtual(std::forward<Args>(args)...);
            return &object->value;
        }

        template<typename T, typename ... Args>
        T* CreateVirtual(const char* name, Args&& ... args)
        {
            return CreateVirtual<T>(AssetID(name), std::forward<Args>(args)...);
        }

        template<typename T>
        void RegisterFactory(IAssetFactory* factory) { CreateTypeInfo<T>()->factory = factory; }

        template<typename T>
        T* Load(AssetID assetId, bool forceReload = false) 
        {
            FixedString256 filepath(assetId.c_str());
            PK_THROW_ASSERT(std::filesystem::exists(filepath.c_str()), "Asset not found at path: %s", filepath.c_str());
            AssetObject<T>* object = CreateAssetObject<T>(assetId);
            LoadAsset(object, forceReload);
            return &object->value;
        }

        template<typename T>
        T* Load(const char* filepath, bool forceReload = false) { return Load<T>(AssetID(filepath), forceReload); }

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
        void ReloadDirectoryByType(const char* directory) { ReloadDirectoryByType(std::type_index(typeid(T)), directory); }
       
        template<typename T>
        void ReloadByType(const std::type_index& typeIndex) { ReloadByType(std::type_index(typeid(T))); }

        template<typename T>
        void UnloadDirectoryByType(const std::type_index& typeIndex, const char* directory) { UnloadDirectoryByType(std::type_index(typeid(T)), directory); }
       
        template<typename T>
        void UnloadByType(const std::type_index& typeIndex) { UnloadByType(std::type_index(typeid(T))); }

        template<typename T>
        void LogAssetDirectoryByType(const std::type_index& typeIndex, const char* directory) { LogAssetDirectoryByType(std::type_index(typeid(T)), directory); }
       
        template<typename T>
        void LogAssetByType(const std::type_index& typeIndex) { LogAssetByType(std::type_index(typeid(T))); }

        template<typename T>
        T* Find(const char* name, bool doAssert = true) const
        {
            auto asset = Find(std::type_index(typeid(T)), name);
            PK_THROW_ASSERT(!doAssert || asset != nullptr, "Could not find asset with name %s", name);
            return asset ? static_cast<T*>(asset) : nullptr;
        }
        
        Asset* Find(const std::type_index& typeIndex, const char* keyword) const;

        void Reload(AssetID assetId);
        void ReloadDirectory(const char* directory);
        void ReloadDirectoryByType(const std::type_index& typeIndex, const char* directory);
        void ReloadByType(const std::type_index& typeIndex);
        void ReloadAll();

        void Unload(AssetID assetId);
        void UnloadDirectory(const char* directory);
        void UnloadDirectoryByType(const std::type_index& typeIndex, const char* directory);
        void UnloadByType(const std::type_index& typeIndex);
        void UnloadAll();

        void LogDirectory(const char* directory);
        void LogDirectoryByType(const std::type_index& typeIndex, const char* directory);
        void LogByType(const std::type_index& typeIndex);
        void LogAll();

    private:
        template<typename T>
        TypeInfo* CreateTypeInfo()
        {
            auto typeIndex = 0u;
            
            if (m_types.AddKey(std::type_index(typeid(T)), &typeIndex))
            {
                m_types[typeIndex].value.headIndex = INVALID_LINK;
                m_types[typeIndex].value.factory = nullptr;
                RegisterConsoleVariables(std::type_index(typeid(T)));
            }
            
            return &m_types[typeIndex].value;
        }

        template<typename T>
        AssetObject<T>* CreateAssetObject(AssetID assetId)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            auto assetIndex = m_assets.GetHashIndex((size_t)assetId);

            if (assetIndex == -1)
            {
                auto typeInfo = CreateTypeInfo<T>();
                auto newAsset = new AssetObject<T>();
                assetIndex = m_assets.Add(newAsset);
                newAsset->assetId = assetId;
                newAsset->version = 0u;
                newAsset->indexNext = typeInfo->headIndex;
                newAsset->cachingMode = AssetCachingMode::Persistent;
                newAsset->factory = static_cast<AssetFactory<T>*>(typeInfo->factory);
                newAsset->isLoaded = false;
                newAsset->isVirtual = false;
                typeInfo->headIndex = assetIndex;
                return newAsset;
            }

            return static_cast<AssetObject<T>*>(m_assets[assetIndex]);
        }

        uint32_t GetTypeHead(const std::type_index& typeIndex) const;
        void LoadAsset(AssetObjectBase* object, bool isReload);
        void RegisterConsoleVariables(const std::type_index& typeIndex);

        FastSet<AssetObjectBase*, AssetObjectHash> m_assets;
        FastMap<std::type_index, TypeInfo> m_types;
        Sequencer* m_sequencer;
    };
}
