#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NameID.h"

namespace PK
{
    typedef NameID AssetID;

    class Asset : public NoCopy
    {
        friend class AssetDatabase;

    public:
        virtual ~Asset() = default;
        virtual std::string GetMetaInfo() const { return std::string("Metadata info is not implemented for this asset type."); }
        constexpr AssetID GetAssetID() const { return m_assetId; }
        constexpr uint32_t GetAssetVersion() const { return m_version; }
        constexpr uint64_t GetAssetHash() const { return ((uint64_t)m_version << 32) | ((uint64_t)m_assetId & 0xFFFFFFFF); }
        inline const char* GetFileName() const { return m_assetId.c_str(); }
        bool operator==(const Asset& other) const { return m_assetId == ((Asset&)other).m_assetId; }

        template<typename T>
        static bool IsValidExtension(const char* extension);

        template<typename T>
        [[nodiscard]] static Ref<T> Create();

    private:
        AssetID m_assetId = 0u;
        uint32_t m_version = 0u;
    };

    template<typename ... Args>
    class IAssetImport : public NoCopy
    {
        friend class AssetDatabase;
        virtual void AssetImport(const char* filepath, Args ... args) = 0;
    };

    template<typename ... Args>
    class AssetWithImport : public Asset, public IAssetImport<Args...> {};
}
