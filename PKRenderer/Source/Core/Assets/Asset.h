#pragma once
#include "Core/Base/NoCopy.h"
#include "Core/Base/Types/Ref.h"
#include "Core/Base/Types/NameID.h"

namespace PK
{
    typedef NameID AssetID;
        
    class Asset : public NoCopy
    {
        friend class AssetDatabase;

    public:
        struct SharedObject : public SharedObjectBase
        {
            AssetID assetId;
            uint32_t version;
        };

        virtual ~Asset() = default;
        virtual const char* GetMetaInfo() const { return "Metadata info is not implemented for this asset type."; }
        constexpr AssetID GetAssetID() const { return m_sharedObject ? m_sharedObject->assetId : AssetID(0u); }
        constexpr uint32_t GetAssetVersion() const { return m_sharedObject ? m_sharedObject->version : 0u; }
        constexpr uint64_t GetAssetHash() const { return ((uint64_t)GetAssetVersion() << 32ull) | (uint64_t)GetAssetID(); }
        inline const char* GetFileName() const { return GetAssetID().c_str(); }
        bool operator==(const Asset& other) const { return GetAssetID() == other.GetAssetID(); }

        template<typename T>
        Ref<T> CreateAliasRef(T* object) { return Ref<T>(m_sharedObject, object); }
    
    private:
        SharedObject* m_sharedObject = nullptr;
    };

    struct IAssetFactory
    {
        virtual ~IAssetFactory() = 0;
    };

    template<typename T>
    class AssetFactory : public IAssetFactory
    {
        friend class AssetDatabase;
        using TAsset = T;
        virtual void AssetConstruct(T* memory, const char* filepath) = 0;
    };

    // Add type traits here if needed.
    template<typename T>
    struct AssetTraits
    {
        constexpr static const char* Extension = "*.*";
    };
}
