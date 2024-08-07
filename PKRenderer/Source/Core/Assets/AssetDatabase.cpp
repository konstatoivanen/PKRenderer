#include "PrecompiledHeader.h"
#include "Core/Utilities/TypeInfo.h"
#include "Core/CLI/CVariableRegister.h"
#include "AssetDatabase.h"

namespace PK
{
    AssetDatabase::AssetDatabase(Sequencer* sequencer) :
        m_sequencer(sequencer)
    {
        CVariableRegister::Create<CVariableFuncSimple>("AssetDatabase.Query.Loaded", [this](){LogAssetsAll();});
    }

    void AssetDatabase::LogAssetsAll()
    {
        PK_LOG_HEADER("AssetDatabase.Log.All:");
        PK_LOG_SCOPE_INDENT(logassets);

        for (auto& typecollection : m_assets)
        {
            LogAssetsOfTypeInternal(typecollection.first);
        }
    }

    void AssetDatabase::LogAssetsOfTypeInternal(const std::type_index& typeIndex) const
    {
        PK_LOG_HEADER("AssetDatabase.Log.Type: %s", typeIndex.name());
        PK_LOG_SCOPE_INDENT(logassetsoftype);

        if (m_assets.count(typeIndex) > 0)
        {
            for (auto& kv : m_assets.at(typeIndex))
            {
                PK_LOG_INFO(kv.first.c_str());
            }
        }
    }

    void AssetDatabase::ReloadCachedAllInternal(const std::type_index& typeIndex)
    {
        auto collectionIter = m_assetReloadCaptures.find(typeIndex);

        if (collectionIter != m_assetReloadCaptures.end())
        {
            auto& collection = collectionIter->second;

            PK_LOG_VERBOSE("AssetDatabase.Reload.Cached: %s", typeIndex.name());
            PK_LOG_SCOPE_INDENT(reload);

            for (auto& capture : collection)
            {
                capture.second();
            }
        }
    }

    void AssetDatabase::ReloadCachedInternal(const std::type_index& typeIndex, AssetID assetId)
    {
        auto collectionIter = m_assetReloadCaptures.find(typeIndex);

        if (collectionIter != m_assetReloadCaptures.end())
        {
            auto& collection = collectionIter->second;
            auto assetIter = collection.find(assetId);

            if (assetIter != collection.end())
            {
                assetIter->second();
            }
        }
    }

    void AssetDatabase::ReloadCachedDirectoryInternal(const std::type_index& typeIndex, const std::string& directory)
    {
        if (!std::filesystem::exists(directory))
        {
            return;
        }

        auto collectionIter = m_assetReloadCaptures.find(typeIndex);

        if (collectionIter == m_assetReloadCaptures.end())
        {
            return;
        }

        auto& collection = collectionIter->second;

        PK_LOG_VERBOSE("AssetDatabase.Reload.Cached.Directory: %s, %s", typeIndex.name(), directory.c_str());
        PK_LOG_SCOPE_INDENT(reload);

        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            auto name = entry.path().string();
            auto assetId = AssetID(entry.path().string());
            auto assetIter = collection.find(assetId);

            if (assetIter != collection.end())
            {
                assetIter->second();
            }
        }
    }

    Ref<Asset> AssetDatabase::FindInternal(const std::type_index& typeIndex, const char* name) const
    {
        if (m_assets.count(typeIndex) > 0)
        {
            for (const auto& kv : m_assets.at(typeIndex))
            {
                if (kv.first.to_string().find(name) != std::string::npos)
                {
                    return kv.second;
                }
            }
        }

        return nullptr;
    }

    void AssetDatabase::RegisterMetaFunctionality(const std::type_index& typeIndex)
    {
        // We already bound cvars for this type.
        if (m_assets.count(typeIndex) > 0)
        {
            return;
        }

        auto name = GetTypeShortName(typeIndex);
        auto cvarnameMeta = std::string("AssetDatabase.Query.Meta.") + name;
        CVariableRegister::Create<CVariableFunc>(cvarnameMeta.c_str(), [this, typeIndex, name](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                PK_LOG_NEWLINE();
                auto asset = FindInternal(typeIndex, args[0]);
                if (asset == nullptr)
                {
                    PK_LOG_WARNING("AssetDatabase.Query.Meta.%s Not Found With '%s'", name.c_str(), args[0]);
                }
                PK_LOG_INFO(asset->GetMetaInfo().c_str());
                PK_LOG_NEWLINE();

            }, "Expected a keyword argument", 1u);

        auto cvarnameLoaded = std::string("AssetDatabase.Query.Loaded.") + name;
        CVariableRegister::Create<CVariableFuncSimple>(cvarnameLoaded.c_str(), [this, typeIndex]()
            {
                LogAssetsOfTypeInternal(typeIndex);
            });

        auto cvarnameReloadAll = std::string("AssetDatabase.Reload.Cached.All.") + name;
        CVariableRegister::Create<CVariableFuncSimple>(cvarnameReloadAll.c_str(), [this, typeIndex]()
            {
                ReloadCachedAllInternal(typeIndex);
            });

        auto cvarnameReload = std::string("AssetDatabase.Reload.Cached.") + name;
        CVariableRegister::Create<CVariableFunc>(cvarnameReload.c_str(), [this, typeIndex](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCachedInternal(typeIndex, AssetID(args[0]));
            }, "Expected a filepath argument", 1u);

        auto cvarnameReloadDirectory = std::string("AssetDatabase.Reload.Cached.Directory.") + name;
        CVariableRegister::Create<CVariableFunc>(cvarnameReloadDirectory.c_str(), [this, typeIndex](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCachedDirectoryInternal(typeIndex, std::string(args[0]));
            }, "Expected a directory argument", 1u);
    }
}