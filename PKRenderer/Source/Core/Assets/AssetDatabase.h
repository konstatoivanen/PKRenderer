#pragma once
#include <filesystem>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/Hash.h"
#include "Core/Assets/Asset.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"

namespace PK
{
    class AssetDatabase : public ISingleton<AssetDatabase>
    {
        struct AssetReference
        {
            Ref<Asset> asset = nullptr;
            std::function<void()> reload = nullptr;
        };

    public:
        AssetDatabase(Sequencer* sequencer);

        template<typename T>
        [[nodiscard]] T* Register(const std::string& name, Ref<T> asset) { return RegisterInternal(name.c_str(), asset); }

        template<typename T, typename ... Args>
        [[nodiscard]] T* Create(const std::string& name, Args&& ... args) { return RegisterInternal(name.c_str(), CreateRef<T>(std::forward<Args>(args)...)); }

        template<typename T>
        [[nodiscard]] T* TryFind(const char* name) const
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            auto asset = FindInternal(std::type_index(typeid(T)), name);
            return asset != nullptr ? std::static_pointer_cast<T>(asset).get() : nullptr;
        }

        template<typename T>
        [[nodiscard]] T* Find(const char* name) const
        {
            auto value = TryFind<T>(name);
            PK_THROW_ASSERT(value != nullptr, "Could not find asset with name %s", name);
            return value;
        }

        template<typename T, typename ... Args>
        T* Load(const std::string& filepath, Args&& ... args) { return LoadInternal<T>(AssetID(filepath.c_str()), false, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        T* Load(const char* filepath, Args&& ... args) { return LoadInternal<T>(AssetID(filepath), false, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        T* Reload(const std::string& filepath, Args&& ... args) { return LoadInternal<T>(AssetID(filepath.c_str()), true, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        [[nodiscard]] T* Reload(AssetID assetId, Args&& ... args) { return LoadInternal<T>(assetId, true, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        void Reload(const T* asset, Args&& ... args) { LoadInternal<T>(asset->GetAssetID(), true, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        void LoadDirectory(const std::string& directory, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            if (std::filesystem::exists(directory))
            {
                PK_LOG_VERBOSE("AssetDatabase.Load.Directory: %s, %s", typeid(T).name(), directory.c_str());
                PK_LOG_SCOPE_INDENT(load);

                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (ValidateExtension<T>(entry.path()))
                    {
                        Load<T>(entry.path().string(), std::forward<Args>(args)...);
                    }
                }
            }
        }

        template<typename T, typename ... Args>
        void ReloadDirectory(const std::string& directory, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            if (std::filesystem::exists(directory))
            {
                PK_LOG_VERBOSE("AssetDatabase.Reload.Directory: %s, %s", typeid(T).name(), directory.c_str());
                PK_LOG_SCOPE_INDENT(reload);

                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (ValidateExtension<T>(entry.path()))
                    {
                        Reload<T>(entry.path().string(), std::forward<Args>(args)...);
                    }
                }
            }
        }

        template<typename T>
        inline void ReloadCached(AssetID assetId) { ReloadCachedInternal(std::type_index(typeid(T)), assetId); }

        template<typename T>
        inline void ReloadCachedAll() { ReloadCachedAllInternal(std::type_index(typeid(T))); }

        template<typename T>
        inline void ReloadCachedDirectory(const std::string& directory) { ReloadCachedInternal(std::type_index(typeid(T)), directory); }

        template<typename T>
        std::vector<T*> GetAssetsOfType()
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            std::vector<T*> result;

            auto collectionIter = m_assets.find(std::type_index(typeid(T)));

            if (collectionIter != m_assets.end())
            {
                auto& collection = collectionIter->second;

                for (auto& kv : collection)
                {
                    result.push_back(std::static_pointer_cast<T>(kv.second.asset).get());
                }
            }

            return result;
        }

        template<typename T>
        void UnloadDirectory(const std::string& directory)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            if (std::filesystem::exists(directory))
            {
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (ValidateExtension<T>(entry.path()))
                    {
                        Unload<T>(entry.path().string());
                    }
                }
            }
        }

        template<typename T>
        void Unload(AssetID assetId)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            auto collectionIter = m_assets.find(std::type_index(typeid(T)));

            if (collectionIter != m_assets.end())
            {
                auto& collection = collectionIter->second;
                collection.erase(assetId);
            }
        }

        template<typename T>
        inline void Unload(const std::string& filepath)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            Unload<T>(AssetID(filepath));
        }

        template<typename T>
        inline void Unload()
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            m_assets.erase(std::type_index(typeid(T)));
        }

        inline void Unload() { m_assets.clear(); };

        template<typename T>
        void LogAssetsOfType()
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            LogAssetsOfTypeInternal(std::type_index(typeid(T)));
        }

        void LogAssetsAll();

    private:
        void LogAssetsOfTypeInternal(const std::type_index& type) const;
        void ReloadCachedAllInternal(const std::type_index& typeIndex);
        void ReloadCachedInternal(const std::type_index& typeIndex, AssetID assetId);
        void ReloadCachedDirectoryInternal(const std::type_index& typeIndex, const std::string& directory);
        [[nodiscard]] Ref<Asset> FindInternal(const std::type_index& typeIndex, const char* keyword) const;
        void RegisterMetaFunctionality(const std::type_index& typeIndex);

        template<typename T, typename ... Args>
        [[nodiscard]] T* LoadInternal(AssetID assetId, bool isReload, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            // Copy intentional as mapped names can be moved. 
            FixedString256 filepath(assetId.c_str());

            PK_THROW_ASSERT(std::filesystem::exists(filepath.c_str()), "Asset not found at path: %s", filepath.c_str());
            PK_LOG_VERBOSE("AssetDatabase.Load: %s, %s", typeid(T).name(), filepath.c_str());
            PK_LOG_SCOPE_INDENT(asset);

            auto typeIndex = std::type_index(typeid(T));

            RegisterMetaFunctionality(typeIndex);

            auto& collection = m_assets[typeIndex];
            AssetReference* reference = nullptr;
            Ref<T> asset = nullptr;

            auto iter = collection.find(assetId);

            if (iter != collection.end())
            {
                reference = &iter->second;
                asset = std::static_pointer_cast<T>(reference->asset);

                if (isReload == false)
                {
                    return asset.get();
                }
            }
            else
            {
                asset = Asset::Create<T>();
                reference = &collection[assetId];
                reference->asset = asset;
                std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;
            }

            std::static_pointer_cast<Asset>(asset)->m_version++;
            std::static_pointer_cast<IAssetImport<Args...>>(asset)->AssetImport(filepath.c_str(), std::forward<Args>(args)...);
            AssetImportEvent<T> importToken = { this, asset.get() };
            m_sequencer->Next(this, &importToken);

            // Don't capture variadic asset loads as we cant ensure paremeter lifetimes.
            if (reference->reload == nullptr && sizeof ... (args) == 0)
            {
                reference->reload = [this, assetWeakRef = Weak<T>(asset)]()
                {
                    if (!assetWeakRef.expired())
                    {
                        auto asset = assetWeakRef.lock();
                        FixedString128 fileName = std::static_pointer_cast<Asset>(asset)->GetFileName();
                        std::static_pointer_cast<Asset>(asset)->m_version++;
                        PK_LOG_VERBOSE("AssetDatabase.Reload.Cached: %s, %s", typeid(T).name(), fileName.c_str());
                        PK_LOG_SCOPE_INDENT(asset);
                        std::dynamic_pointer_cast<IAssetImport<>>(asset)->AssetImport(fileName.c_str());
                        AssetImportEvent<T> importToken = { this, asset.get() };
                        m_sequencer->Next(this, &importToken);
                    }
                };
            }

            return asset.get();
        }

        template<typename T>
        [[nodiscard]] T* RegisterInternal(const char* name, Ref<T> asset)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            RegisterMetaFunctionality(typeid(T));

            auto& collection = m_assets[std::type_index(typeid(T))];
            auto assetId = AssetID(name);

            PK_THROW_ASSERT(collection.count(assetId) < 1, "AssetDatabase.Register: (%s) already exists!", name);
            PK_LOG_VERBOSE("AssetDatabase.Register: %s, %s", typeid(T).name(), name);

            collection[assetId].asset = asset;
            std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;

            return asset.get();
        }

        template<typename T>
        bool ValidateExtension(const std::filesystem::path& path) { return path.has_extension() && Asset::IsValidExtension<T>(path.extension().string().c_str()); }

        using AssetCollection = std::unordered_map<AssetID, AssetReference, Hash::TCastHash<AssetID>>;
        std::unordered_map<std::type_index, AssetCollection> m_assets;
        Sequencer* m_sequencer;
    };
}