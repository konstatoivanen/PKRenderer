#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/StringHashID.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Ref.h"
#include "Utilities/Log.h"
#include "Core/NoCopy.h"
#include "ECS/Sequencer.h"
#include <filesystem>

namespace PK::Core
{
    using namespace PK::Utilities;

    typedef uint32_t AssetID;
    
    class Asset : public NoCopy
    {
        friend class AssetDatabase;
    
        public:
            virtual ~Asset() = default;
    
            virtual void Import(const char* filepath) = 0;

            inline AssetID GetAssetID() const { return m_assetId; }
    
            inline const std::string& GetFileName() const { return StringHashID::IDToString(m_assetId); }
    
            inline bool IsFileAsset() const { return m_assetId != 0; }
    
            bool operator==(const Asset& other) const { return m_assetId == ((Asset&)other).m_assetId; }
    
        private:
            AssetID m_assetId = 0;
    };

    enum class AssetImportType
    {
        IMPORT,
        RELOAD
    };

    template<typename T>
    struct AssetImportToken
    {
        AssetDatabase* assetDatabase;
        T* asset;
    };
    
    namespace AssetImporters
    {
        template<typename T>
        bool IsValidExtension(const std::filesystem::path& extension);

        template<typename T>
        [[nodiscard]] Ref<T> Create();
    };
    
    class AssetDatabase : public IService
    {
        private:
            template<typename T>
            [[nodiscard]] T* Load(const std::string& filepath, AssetID assetId)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                PK_THROW_ASSERT(std::filesystem::exists(filepath), "Asset not found at path: %s", filepath.c_str());
    
                auto& collection = m_assets[std::type_index(typeid(T))];
    
                if (collection.count(assetId) > 0)
                {
                    return std::static_pointer_cast<T>(collection.at(assetId)).get();
                }
    
                auto asset = AssetImporters::Create<T>();
                collection[assetId] = asset;
                std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;

                asset->Import(filepath.c_str());
    
                AssetImportToken<T> importToken = { this, asset.get() };
                m_sequencer->Next(this, &importToken, (int)AssetImportType::IMPORT);

                return asset.get();
            }
    
            template<typename T>
            [[nodiscard]] T* Reload(const std::string& filepath, AssetID assetId)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                PK_THROW_ASSERT(std::filesystem::exists(filepath), "Asset not found at path: %s", filepath.c_str());
                
                auto& collection = m_assets[std::type_index(typeid(T))];
                Ref<T> asset = nullptr;
    
                if (collection.count(assetId) > 0)
                {
                    asset = std::static_pointer_cast<T>(collection.at(assetId));
                }
                else
                {
                    auto asset = AssetImporters::Create<T>();
                    collection[assetId] = asset;
                    std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;
                }
    
                asset->Import(filepath.c_str());
    
                AssetImportToken<T> importToken = { this, asset.get() };
                m_sequencer->Next(this, &importToken, (int)AssetImportType::RELOAD);

                return asset.get();
            }
    
        public:
            AssetDatabase(ECS::Sequencer* sequencer) : m_sequencer(sequencer) {}

            template<typename T, typename ... Args>
            [[nodiscard]] T* CreateProcedural(std::string name, Args&& ... args)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                auto& collection = m_assets[std::type_index(typeid(T))];
                auto assetId = StringHashID::StringToID(name);
    
                PK_THROW_ASSERT(collection.count(assetId) < 1, "Procedural asset (%s) already exists", name.c_str());
    
                auto asset = CreateRef<T>(std::forward<Args>(args)...);
                collection[assetId] = asset;
                std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;
    
                return asset.get();
            }
    
            template<typename T, typename ... Args>
            [[nodiscard]] T* RegisterProcedural(std::string name, Ref<T> asset)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                auto& collection = m_assets[std::type_index(typeid(T))];
                auto assetId = StringHashID::StringToID(name);
    
                PK_THROW_ASSERT(collection.count(assetId) < 1, "Procedural asset (%s) already exists", name.c_str());
    
                collection[assetId] = asset;
                std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;
    
                return asset.get();
            }
    
            template<typename T>
            [[nodiscard]] T* Find(const char* name) const
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                auto type = std::type_index(typeid(T));
    
                if (m_assets.count(type) > 0)
                {
                    auto& collection = m_assets.at(type);
    
                    for (auto& i : collection)
                    {
                        auto filename = Utilities::String::ReadFileName(StringHashID::IDToString(i.first));
    
                        if (filename.find(name) != std::string::npos)
                        {
                            return std::static_pointer_cast<T>(i.second).get();
                        }
                    }
                }
    
                PK_THROW_ERROR("Could not find asset with name %s", name);
            }

            template<typename T>
            [[nodiscard]] T* TryFind(const char* name) const
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                auto type = std::type_index(typeid(T));

                if (m_assets.count(type) > 0)
                {
                    auto& collection = m_assets.at(type);

                    for (auto& i : collection)
                    {
                        auto filename = Utilities::String::ReadFileName(StringHashID::IDToString(i.first));

                        if (filename.find(name) != std::string::npos)
                        {
                            return std::static_pointer_cast<T>(i.second).get();
                        }
                    }
                }

                return nullptr;
            }
            
            template<typename T>
            T* Load(const std::string& filepath) { return Load<T>(filepath, StringHashID::StringToID(filepath)); }
    
            template<typename T>
            [[nodiscard]] T* Load(AssetID assetId) { return Load<T>(StringHashID::IDToString(assetId), assetId); }
    
            template<typename T>
            T* Reload(const std::string& filepath) { return Reload<T>(filepath, StringHashID::StringToID(filepath)); }
    
            template<typename T>
            [[nodiscard]] T* Reload(AssetID assetId) { return Reload<T>(StringHashID::IDToString(assetId), assetId); }
    
            template<typename T>
            void Reload(const T* asset) 
            {
                auto assetId = asset->GetAssetID();
                Reload<T>(StringHashID::IDToString(assetId), assetId); 
            }
    
            template<typename T>
            void LoadDirectory(const std::string& directory)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                if (!std::filesystem::exists(directory))
                {
                    return;
                }
    
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    auto& path = entry.path();
    
                    if (path.has_extension() && AssetImporters::IsValidExtension<T>(path.extension()))
                    {
                        Load<T>(entry.path().string());
                    }
                }
            }
    
            template<typename T>
            void ReloadDirectory(const std::string& directory)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                if (!std::filesystem::exists(directory))
                {
                    return;
                }
    
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    auto& path = entry.path();
    
                    if (path.has_extension() && AssetImporters::IsValidExtension<T>(path.extension()))
                    {
                        Reload<T>(entry.path().string());
                    }
                }
            }
    
            template<typename T>
            void UnloadDirectory(const std::string& directory)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                if (!std::filesystem::exists(directory))
                {
                    return;
                }
    
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    auto& path = entry.path();
    
                    if (path.has_extension() && AssetImporters::IsValidExtension<T>(path.extension()))
                    {
                        Unload<T>(entry.path().string());
                    }
                }
            }
    
            template<typename T>
            void Unload(AssetID assetId)
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                auto& collection = m_assets[std::type_index(typeid(T))];
                collection.erase(assetId);
            }
    
            template<typename T>
            inline void Unload(const std::string& filepath) 
            { 
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                Unload<T>(StringHashID::StringToID(filepath)); 
            }
    
            template<typename T>
            inline void Unload() 
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
                m_assets.erase(std::type_index(typeid(T))); 
            }
    
            inline void Unload() { m_assets.clear();  };
    
            template<typename T>
            void ListAssetsOfType()
            {
                static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

                auto type = std::type_index(typeid(T));
                auto& collection = m_assets[type];

                PK_LOG_HEADER("Listing loaded assets of type: %s", type.name());

                for (auto& kv : collection)
                {
                    PK_LOG_INFO(StringHashID::IDToString(kv.first).c_str());
                }
            }

            void ListAssets()
            {
                for (auto& typecollection : m_assets)
                {
                    PK_LOG_HEADER("Listing loaded assets of type: %s", typecollection.first.name());

                    for (auto& kv : typecollection.second)
                    {
                        PK_LOG_INFO(StringHashID::IDToString(kv.first).c_str());
                    }
                }
            }

        private:
            std::unordered_map<std::type_index, std::unordered_map<AssetID, Ref<Asset>>> m_assets;
            ECS::Sequencer* m_sequencer;
    };
}