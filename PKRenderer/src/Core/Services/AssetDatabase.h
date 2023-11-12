#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Core/Services/Log.h"
#include "Core/Services/StringHashID.h"
#include "Core/Services/Sequencer.h"
#include <filesystem>

namespace PK::Core::Services
{
    typedef uint32_t AssetID;

    class Asset : public Utilities::NoCopy
    {
        friend class AssetDatabase;

    public:
        virtual ~Asset() = default;
        virtual std::string GetMetaInfo() const { return std::string("Metadata info is not implemented for this asset type."); }
        constexpr AssetID GetAssetID() const { return m_assetId; }
        constexpr uint32_t GetAssetVersion() const { return m_version; }
        constexpr uint64_t GetAssetHash() const { return ((uint64_t)m_version << 32) | ((uint64_t)m_assetId & 0xFFFFFFFF); }
        inline const std::string& GetFileName() const { return StringHashID::IDToString(m_assetId); }
        bool operator==(const Asset& other) const { return m_assetId == ((Asset&)other).m_assetId; }

    private:
        AssetID m_assetId = 0u;
        uint32_t m_version = 0u;
    };

    template<typename ... Args>
    class IAssetImport : public Utilities::NoCopy
    {
        friend class AssetDatabase;
        virtual void Import(const char* filepath, Args ... args) = 0;
    };

    typedef IAssetImport<> IAssetImportSimple;

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
        [[nodiscard]] Utilities::Ref<T> Create();
    };

    class AssetDatabase : public IService
    {
    private:
        template<typename T, typename ... Args>
        [[nodiscard]] T* LoadInternal(const std::string& filepath, AssetID assetId, bool reload, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            static_assert(std::is_base_of<IAssetImport<Args...>, T>::value, "Template argument type does not derive from IAssetImport!");
            PK_THROW_ASSERT(std::filesystem::exists(filepath), "Asset not found at path: %s", filepath.c_str());

            auto importType = reload ? AssetImportType::RELOAD : AssetImportType::IMPORT;
            auto& collection = m_assets[std::type_index(typeid(T))];
            auto iter = collection.find(assetId);
            Utilities::Ref<T> asset = nullptr;

            if (iter != collection.end())
            {
                asset = std::static_pointer_cast<T>(iter->second);

                if (!reload)
                {
                    return asset.get();
                }
            }
            else
            {
                importType = AssetImportType::IMPORT;
                asset = AssetImporters::Create<T>();
                collection[assetId] = asset;
                std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;
            }

            std::static_pointer_cast<Asset>(asset)->m_version++;

            PK_LOG_VERBOSE("Asset Import: %s, %s", typeid(T).name(), filepath.c_str());
            PK_LOG_SCOPE_INDENT(asset);

            static_cast<IAssetImport<Args...>*>(asset.get())->Import(filepath.c_str(), std::forward<Args>(args)...);

            AssetImportToken<T> importToken = { this, asset.get() };
            m_sequencer->Next(this, &importToken, (int)importType);

            return asset.get();
        }

    public:
        AssetDatabase(Sequencer* sequencer) : m_sequencer(sequencer) {}

        template<typename T, typename ... Args>
        [[nodiscard]] T* CreateProcedural(std::string name, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");
            auto& collection = m_assets[std::type_index(typeid(T))];
            auto assetId = StringHashID::StringToID(name);

            PK_THROW_ASSERT(collection.count(assetId) < 1, "Procedural asset (%s) already exists", name.c_str());

            auto asset = Utilities::CreateRef<T>(std::forward<Args>(args)...);
            collection[assetId] = asset;
            std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;

            return asset.get();
        }

        template<typename T>
        [[nodiscard]] T* RegisterProcedural(std::string name, Utilities::Ref<T> asset)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            auto& collection = m_assets[std::type_index(typeid(T))];
            auto assetId = StringHashID::StringToID(name);

            PK_THROW_ASSERT(collection.count(assetId) < 1, "Procedural asset (%s) already exists", name.c_str());
            PK_LOG_VERBOSE("Asset Procedural Register: %s, %s", typeid(T).name(), name.c_str());

            collection[assetId] = asset;
            std::static_pointer_cast<Asset>(asset)->m_assetId = assetId;

            return asset.get();
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
                    auto& filename = StringHashID::IDToString(i.first);

                    if (filename.find(name) != std::string::npos)
                    {
                        return std::static_pointer_cast<T>(i.second).get();
                    }
                }
            }

            return nullptr;
        }

        template<typename T>
        [[nodiscard]] T* Find(const char* name) const
        {
            auto value = TryFind<T>(name);
            PK_THROW_ASSERT(value != nullptr, "Could not find asset with name %s", name);
            return value;
        }

        template<typename T, typename ... Args>
        T* Load(const std::string& filepath, Args&& ... args) { return LoadInternal<T>(filepath, StringHashID::StringToID(filepath), false, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        [[nodiscard]] T* Load(AssetID assetId, Args&& ... args) { return LoadInternal<T>(StringHashID::IDToString(assetId), assetId, false, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        T* Reload(const std::string& filepath, Args&& ... args) { return LoadInternal<T>(filepath, StringHashID::StringToID(filepath), true, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        [[nodiscard]] T* Reload(AssetID assetId, Args&& ... args) { return LoadInternal<T>(StringHashID::IDToString(assetId), assetId, true, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        void Reload(const T* asset, Args&& ... args)
        {
            auto assetId = asset->GetAssetID();
            LoadInternal<T>(StringHashID::IDToString(assetId), assetId, true, std::forward<Args>(args)...);
        }

        template<typename T, typename ... Args>
        void LoadDirectory(const std::string& directory, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            if (!std::filesystem::exists(directory))
            {
                return;
            }

            PK_LOG_VERBOSE("Asset Load Directory: %s, %s", typeid(T).name(), directory.c_str());
            PK_LOG_SCOPE_INDENT(load);

            for (const auto& entry : std::filesystem::directory_iterator(directory))
            {
                auto& path = entry.path();

                if (path.has_extension() && AssetImporters::IsValidExtension<T>(path.extension()))
                {
                    Load<T>(entry.path().string(), std::forward<Args>(args)...);
                }
            }
        }

        template<typename T, typename ... Args>
        void ReloadDirectory(const std::string& directory, Args&& ... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "Template argument type does not derive from Asset!");

            if (!std::filesystem::exists(directory))
            {
                return;
            }

            PK_LOG_VERBOSE("Asset Reload Directory: %s, %s", typeid(T).name(), directory.c_str());
            PK_LOG_SCOPE_INDENT(reload);

            for (const auto& entry : std::filesystem::directory_iterator(directory))
            {
                auto& path = entry.path();

                if (path.has_extension() && AssetImporters::IsValidExtension<T>(path.extension()))
                {
                    Reload<T>(entry.path().string(), std::forward<Args>(args)...);
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

        inline void Unload() { m_assets.clear(); };

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
        std::unordered_map<std::type_index, std::unordered_map<AssetID, Utilities::Ref<Asset>>> m_assets;
        Sequencer* m_sequencer;
    };
}