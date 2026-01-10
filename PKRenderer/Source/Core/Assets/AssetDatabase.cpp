#include "PrecompiledHeader.h"
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/CVariableRegister.h"
#include "AssetDatabase.h"

namespace PK
{
    IAssetFactory::~IAssetFactory() = default;


    AssetDatabase::AssetDatabase(Sequencer* sequencer) :
        m_assets(512u, 1u),
        m_types(32u, 1u),
        m_sequencer(sequencer)
    {
        CVariableRegister::Create<CVariableFuncSimple>("AssetDatabase.Query.Loaded", [this](){LogAll();});
    }

    AssetDatabase::~AssetDatabase()
    {
        for (int32_t i = m_assets.GetCount() - 1; i >= 0; --i)
        {
            m_assets[i]->Destroy();
            m_assets[i]->Delete();
        }
    }


    Asset* AssetDatabase::Find(const std::type_index& typeIndex, const char* keyword) const
    {
        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
        {
            if (strstr(m_assets[index]->assetId.c_str(), keyword) != nullptr)
            {
                return m_assets[index]->isLoaded ? m_assets[index]->GetAsset() : nullptr;
            }
        }

        return nullptr;
    }


    void AssetDatabase::Reload(AssetID assetId)
    {
        PK_LOG_VERBOSE_FUNC();

        auto index = m_assets.GetHashIndex((size_t)assetId);

        if (index != -1)
        {
            LoadAsset(m_assets[index], true);
        }
    }

    void AssetDatabase::ReloadDirectory(const char* directory)
    {
        PK_LOG_VERBOSE_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto i = 0u; i < m_assets.GetCount(); ++i)
            {
                if (m_assets[i]->isLoaded && strncmp(directory, m_assets[i]->assetId.c_str(), directoryLen) == 0)
                {
                    LoadAsset(m_assets[i], true);
                }
            }
        }
    }

    void AssetDatabase::ReloadDirectoryByType(const std::type_index& typeIndex, const char* directory)
    {
        PK_LOG_VERBOSE_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
            {
                if (m_assets[index]->isLoaded && strncmp(directory, m_assets[index]->assetId.c_str(), directoryLen) == 0)
                {
                    LoadAsset(m_assets[index], true);
                }
            }
        }
    }

    void AssetDatabase::ReloadByType(const std::type_index& typeIndex)
    {
        PK_LOG_VERBOSE_FUNC();

        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
        {
            LoadAsset(m_assets[index], true);
        }
    }
    
    void AssetDatabase::ReloadAll()
    {
        PK_LOG_VERBOSE_FUNC();

        for (int32_t i = m_assets.GetCount() - 1; i >= 0; --i)
        {
            LoadAsset(m_assets[i], true);
        }
    }


    void AssetDatabase::Unload(AssetID assetId)
    {
        PK_LOG_VERBOSE_FUNC();

        auto index = m_assets.GetHashIndex((size_t)assetId);

        if (index != -1)
        {
            m_assets[index]->Destroy();
        }
    }
    
    void AssetDatabase::UnloadDirectory(const char* directory)
    {
        PK_LOG_VERBOSE_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto i = 0u; i < m_assets.GetCount(); ++i)
            {
                if (m_assets[i]->isLoaded && strncmp(directory, m_assets[i]->assetId.c_str(), directoryLen) == 0)
                {
                    m_assets[i]->Destroy();
                }
            }
        }
    }

    void AssetDatabase::UnloadDirectoryByType(const std::type_index& typeIndex, const char* directory)
    {
        PK_LOG_VERBOSE_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
            {
                if (m_assets[index]->isLoaded && strncmp(directory, m_assets[index]->assetId.c_str(), directoryLen) == 0)
                {
                    m_assets[index]->Destroy();
                }
            }
        }
    }
    
    void AssetDatabase::UnloadByType(const std::type_index& typeIndex)
    {
        PK_LOG_VERBOSE_FUNC();

        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
        {
            m_assets[index]->Destroy();
        }
    }

    void AssetDatabase::UnloadAll()
    {
        PK_LOG_VERBOSE_FUNC();

        for (int32_t i = m_assets.GetCount() - 1; i >= 0; --i)
        {
            m_assets[i]->Destroy();
        }
    }


    void AssetDatabase::LogDirectory(const char* directory)
    {
        PK_LOG_HEADER_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto i = 0u; i < m_assets.GetCount(); ++i)
            {
                if (m_assets[i]->isLoaded && strncmp(directory, m_assets[i]->assetId.c_str(), directoryLen) == 0)
                {
                    PK_LOG_INFO(m_assets[i]->assetId.c_str());
                }
            }
        }
    }

    void AssetDatabase::LogDirectoryByType(const std::type_index& typeIndex, const char* directory)
    {
        PK_LOG_HEADER_FUNC();

        if (std::filesystem::exists(directory))
        {
            auto directoryLen = strlen(directory);

            for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
            {
                if (m_assets[index]->isLoaded && strncmp(directory, m_assets[index]->assetId.c_str(), directoryLen) == 0)
                {
                    PK_LOG_INFO(m_assets[index]->assetId.c_str());
                }
            }
        }
    }
    
    void AssetDatabase::LogByType(const std::type_index& typeIndex)
    {
        PK_LOG_HEADER_FUNC();
        
        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index]->indexNext)
        {
            if (m_assets[index]->isLoaded)
            {
                PK_LOG_INFO(m_assets[index]->assetId.c_str());
            }
        }
    }
    
    void AssetDatabase::LogAll()
    {
        PK_LOG_HEADER_FUNC();

        for (auto i = 0u; i < m_assets.GetCount(); ++i)
        {
            if (m_assets[i]->isLoaded)
            {
                PK_LOG_INFO(m_assets[i]->assetId.c_str());
            }
        }
    }


    uint32_t AssetDatabase::GetTypeHead(const std::type_index& typeIndex) const
    {
        auto typeInfo = m_types.GetValuePtr(typeIndex);
        return typeInfo != nullptr ? typeInfo->headIndex : INVALID_LINK;
    }

    void AssetDatabase::LoadAsset(AssetObjectBase* object, bool isReload)
    {
        if (!object->isVirtual && (!object->isLoaded || isReload))
        {
            FixedString128 filepath = object->assetId.c_str();
            PK_LOG_VERBOSE_FUNC(": %s, %s", object->GetTypeName(), filepath.c_str());
            object->Destroy();
            object->Construct(this, filepath);
        }
    }

    void AssetDatabase::RegisterConsoleVariables(const std::type_index& typeIndex)
    {
        auto name = Parse::GetTypeShortName(typeIndex);
        FixedString128 cvarnameMeta({ "AssetDatabase.Query.Meta.", name });
        FixedString128 cvarnameLoaded({ "AssetDatabase.Query.Loaded.", name });
        FixedString128 cvarnameReloadAll({ "AssetDatabase.Reload.Cached.All.", name });
        FixedString128 cvarnameReload({ "AssetDatabase.Reload.Cached.", name });

        CVariableRegister::Create<CVariableFunc>(cvarnameMeta.c_str(), [this, typeIndex, name](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                PK_LOG_NEWLINE();
                auto asset = Find(typeIndex, args[0]);
                if (asset == nullptr)
                {
                    PK_LOG_WARNING("AssetDatabase.Query.Meta.%s Not Found With '%s'", name, args[0]);
                }
                PK_LOG_INFO(asset->GetMetaInfo().c_str());
                PK_LOG_NEWLINE();

            }, "Expected a keyword argument", 1u);

        CVariableRegister::Create<CVariableFuncSimple>(cvarnameLoaded.c_str(), [this, typeIndex]()
            {
                LogByType(typeIndex);
            });

        CVariableRegister::Create<CVariableFuncSimple>(cvarnameReloadAll.c_str(), [this, typeIndex]()
            {
                ReloadByType(typeIndex);
            });

        CVariableRegister::Create<CVariableFunc>(cvarnameReload.c_str(), [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                Reload(AssetID(args[0]));
            }, "Expected a filepath argument", 1u);
    }
}
