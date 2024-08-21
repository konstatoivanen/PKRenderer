#include "PrecompiledHeader.h"
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/FixedString.h"
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

        auto collectionIter = m_assets.find(typeIndex);

        if (collectionIter != m_assets.end())
        {
            auto& collection = collectionIter->second;

            for (auto& kv : collection)
            {
                PK_LOG_INFO(kv.first.c_str());
            }
        }
    }

    void AssetDatabase::ReloadCachedAllInternal(const std::type_index& typeIndex)
    {
        auto collectionIter = m_assets.find(typeIndex);

        if (collectionIter != m_assets.end())
        {
            auto& collection = collectionIter->second;

            PK_LOG_VERBOSE("AssetDatabase.Reload.Cached: %s", typeIndex.name());
            PK_LOG_SCOPE_INDENT(reload);

            for (auto& kv : collection)
            {
                if (kv.second.reload)
                {
                    kv.second.reload();
                }
            }
        }
    }

    void AssetDatabase::ReloadCachedInternal(const std::type_index& typeIndex, AssetID assetId)
    {
        auto collectionIter = m_assets.find(typeIndex);

        if (collectionIter != m_assets.end())
        {
            auto& collection = collectionIter->second;
            auto assetIter = collection.find(assetId);

            if (assetIter != collection.end() && assetIter->second.reload)
            {
                assetIter->second.reload();
            }
        }
    }

    void AssetDatabase::ReloadCachedDirectoryInternal(const std::type_index& typeIndex, const std::string& directory)
    {
        if (std::filesystem::exists(directory))
        {
            auto collectionIter = m_assets.find(typeIndex);

            if (collectionIter != m_assets.end())
            {
                auto& collection = collectionIter->second;

                PK_LOG_VERBOSE("AssetDatabase.Reload.Cached.Directory: %s, %s", typeIndex.name(), directory.c_str());
                PK_LOG_SCOPE_INDENT(reload);

                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    auto name = entry.path().string();
                    auto assetId = AssetID(entry.path().string().c_str());
                    auto assetIter = collection.find(assetId);

                    if (assetIter != collection.end() && assetIter->second.reload)
                    {
                        assetIter->second.reload();
                    }
                }
            }
        }
    }

    Ref<Asset> AssetDatabase::FindInternal(const std::type_index& typeIndex, const char* name) const
    {
        auto collectionIter = m_assets.find(typeIndex);

        if (collectionIter != m_assets.end())
        {
            auto& collection = collectionIter->second;

            for (const auto& kv : collection)
            {
                if (strstr(kv.first.c_str(), name) != nullptr)
                {
                    return kv.second.asset;
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

        auto name = Parse::GetTypeShortName(typeIndex);
        FixedString128 cvarnameMeta({ "AssetDatabase.Query.Meta.", name });
        FixedString128 cvarnameLoaded({ "AssetDatabase.Query.Loaded.", name });
        FixedString128 cvarnameReloadAll({ "AssetDatabase.Reload.Cached.All.", name });
        FixedString128 cvarnameReload({ "AssetDatabase.Reload.Cached.", name });
        FixedString128 cvarnameReloadDirectory({ "AssetDatabase.Reload.Cached.Directory.", name });

        CVariableRegister::Create<CVariableFunc>(cvarnameMeta.c_str(), [this, typeIndex, name](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                PK_LOG_NEWLINE();
                auto asset = FindInternal(typeIndex, args[0]);
                if (asset == nullptr)
                {
                    PK_LOG_WARNING("AssetDatabase.Query.Meta.%s Not Found With '%s'", name, args[0]);
                }
                PK_LOG_INFO(asset->GetMetaInfo().c_str());
                PK_LOG_NEWLINE();

            }, "Expected a keyword argument", 1u);

        CVariableRegister::Create<CVariableFuncSimple>(cvarnameLoaded.c_str(), [this, typeIndex]()
            {
                LogAssetsOfTypeInternal(typeIndex);
            });

        CVariableRegister::Create<CVariableFuncSimple>(cvarnameReloadAll.c_str(), [this, typeIndex]()
            {
                ReloadCachedAllInternal(typeIndex);
            });

        CVariableRegister::Create<CVariableFunc>(cvarnameReload.c_str(), [this, typeIndex](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCachedInternal(typeIndex, AssetID(args[0]));
            }, "Expected a filepath argument", 1u);

        CVariableRegister::Create<CVariableFunc>(cvarnameReloadDirectory.c_str(), [this, typeIndex](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCachedDirectoryInternal(typeIndex, std::string(args[0]));
            }, "Expected a directory argument", 1u);
    }
}