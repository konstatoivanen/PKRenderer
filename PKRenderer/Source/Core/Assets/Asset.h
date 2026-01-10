#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NameID.h"

namespace PK
{
    typedef NameID AssetID;
        
    enum class AssetCachingMode
    {
        Auto = 0u,
        GC = 1u,
        Persistent = 2u
    };

    class Asset : public NoCopy
    {
        friend class AssetDatabase;

    public:
        struct SharedObject : public SharedObjectBase
        {
            AssetID assetId;
            uint32_t version;
            AssetCachingMode cachingMode;
        };

        virtual ~Asset() = default;
        virtual std::string GetMetaInfo() const { return std::string("Metadata info is not implemented for this asset type."); }
        constexpr AssetID GetAssetID() const { return m_sharedObject ? m_sharedObject->assetId : AssetID(0u); }
        constexpr uint32_t GetAssetVersion() const { return m_sharedObject ? m_sharedObject->version : 0u; }
        constexpr uint64_t GetAssetHash() const { return ((uint64_t)GetAssetVersion() << 32) | ((uint64_t)GetAssetID() & 0xFFFFFFFF); }
        inline const char* GetFileName() const { return GetAssetID().c_str(); }
        bool operator==(const Asset& other) const { return GetAssetID() == ((Asset&)other).GetAssetID(); }

        template<typename T>
        Ref<T> CreateAliasRef(T* object) { return Ref<T>(m_referenceObject, object); }

        template<typename T>
        static bool IsValidExtension(const char* extension);

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
}
