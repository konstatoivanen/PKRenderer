#include "PrecompiledHeader.h"
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/CVariableRegister.h"
#include "AssetDatabase.h"

namespace PK
{
    AssetDatabase::AssetDatabase(Sequencer* sequencer) :
        m_assets(512u),
        m_typeHeads(32u),
        m_sequencer(sequencer)
    {
        CVariableRegister::Create<CVariableFuncSimple>("AssetDatabase.Query.Loaded", [this](){LogAssetsAll();});
    }

    void AssetDatabase::Unload(const std::type_index& typeIndex)
    {
        auto count = (int32_t)m_assets.GetCount();

        for (auto i = count - 1; i >= 0; --i)
        {
            if (m_assets[i].value.type == typeIndex)
            {
                UnloadInternal(i);
            }
        }
    }

    void AssetDatabase::Unload(AssetID assetId)
    {
        auto index = m_assets.GetIndex(assetId);

        if (index != -1)
        {
            UnloadInternal((uint32_t)index);
        }
    }

    void AssetDatabase::Unload()
    {
        for (auto i = 0u; i < m_assets.GetCount(); ++i)
        {
            m_assets[i].value.asset = nullptr;
            m_assets[i].value.reload = nullptr;
        }

        m_assets.Clear();
    }

    void AssetDatabase::UnloadInternal(uint32_t index)
    {
        auto reference = &m_assets[index].value;
        if (reference->prevIdx == INVALID_LINK) m_typeHeads.SetValue(reference->type, reference->nextIdx);
        if (reference->nextIdx != INVALID_LINK) m_assets[reference->nextIdx].value.prevIdx = reference->prevIdx;
        if (reference->prevIdx != INVALID_LINK) m_assets[reference->prevIdx].value.nextIdx = reference->nextIdx;

        auto removed = m_assets.GetCount() - 1u;

        if (index != removed)
        {
            auto other = &m_assets[removed].value;
            if (other->prevIdx == INVALID_LINK) m_typeHeads.SetValue(other->type, index);
            if (other->nextIdx != INVALID_LINK) m_assets[other->nextIdx].value.prevIdx = index;
            if (other->prevIdx != INVALID_LINK) m_assets[other->prevIdx].value.nextIdx = index;
        }

        reference->asset = nullptr;
        reference->reload = nullptr;
        m_assets.RemoveAt(index);
    }

    void AssetDatabase::LogAssetsAll() const
    {
        PK_LOG_HEADER_SCOPE("AssetDatabase.Log.All:");

        for (auto i = 0u; i < m_assets.GetCount(); ++i)
        {
            PK_LOG_INFO(m_assets[i].key.c_str());
        }
    }

    void AssetDatabase::LogAssetsOfTypeInternal(const std::type_index& typeIndex) const
    {
        PK_LOG_HEADER_SCOPE("AssetDatabase.Log.Type: %s", typeIndex.name());

        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index].value.nextIdx)
        {
            PK_LOG_INFO(m_assets[index].key.c_str());
        }
    }

    void AssetDatabase::ReloadCachedAllInternal(const std::type_index& typeIndex)
    {
        PK_LOG_VERBOSE_FUNC("%s", typeIndex.name());
        
        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index].value.nextIdx)
        {
            if (m_assets[index].value.reload)
            {
                m_assets[index].value.reload();
            }
        }
    }

    void AssetDatabase::ReloadCached(AssetID assetId)
    {
        auto reference = m_assets.GetValuePtr(assetId);

        if (reference != nullptr && reference->reload)
        {
            reference->reload();
        }
    }

    void AssetDatabase::ReloadCachedDirectoryInternal(const std::type_index& typeIndex, const std::string& directory)
    {
        if (std::filesystem::exists(directory))
        {
            PK_LOG_VERBOSE_FUNC("%s, %s", typeIndex.name(), directory.c_str());
            
            for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index].value.nextIdx)
            {
                auto key = &m_assets[index].key;
                auto reference = &m_assets[index].value;

                if (reference->reload && strncmp(directory.c_str(), key->c_str(), directory.length()) == 0)
                {
                    reference->reload();
                }
            }
        }
    }

    Ref<Asset> AssetDatabase::FindInternal(const std::type_index& typeIndex, const char* name) const
    {
        // @TODO potentially super slow. fix it.
        for (auto index = GetTypeHead(typeIndex); index != INVALID_LINK; index = m_assets[index].value.nextIdx)
        {
            if (strstr(m_assets[index].key.c_str(), name) != nullptr)
            {
                return m_assets[index].value.asset;
            }
        }

        return nullptr;
    }

    void AssetDatabase::RegisterMetaFunctionality(const std::type_index& typeIndex)
    {
        // We already bound cvars for this type.
        if (m_typeHeads.Contains(typeIndex))
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

        CVariableRegister::Create<CVariableFunc>(cvarnameReload.c_str(), [this](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCached(AssetID(args[0]));
            }, "Expected a filepath argument", 1u);

        CVariableRegister::Create<CVariableFunc>(cvarnameReloadDirectory.c_str(), [this, typeIndex](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                ReloadCachedDirectoryInternal(typeIndex, std::string(args[0]));
            }, "Expected a directory argument", 1u);
    }
    
    bool AssetDatabase::GetOrCreateAssetReference(const std::type_index& typeIndex, AssetID assetId, AssetReference** outReference)
    {
        auto index = 0u;
        
        if (m_assets.AddKey(assetId, &index))
        {
            *outReference = &m_assets[index].value;
            **outReference = AssetReference();
            
            auto headIndex = 0u;
            auto isNew = m_typeHeads.AddKey(typeIndex, &headIndex);
            auto& head = m_typeHeads[headIndex].value;

            if (isNew || head == INVALID_LINK)
            {
                head = index;
            }
            else
            {
                m_assets[head].value.prevIdx = index;
                (*outReference)->nextIdx = head;
                head = index;
            }

            return true;
        }

        *outReference = &m_assets[index].value;
        return false;
    }

    uint32_t AssetDatabase::GetTypeHead(const std::type_index& typeIndex) const
    {
        auto headPtr = m_typeHeads.GetValuePtr(typeIndex);
        return headPtr != nullptr ? *headPtr : INVALID_LINK;
    }
}