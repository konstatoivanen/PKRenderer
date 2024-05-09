#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Utilities/NameID.h"

namespace PK::Core::Assets
{
    typedef Utilities::NameID AssetID;

    class Asset : public Utilities::NoCopy
    {
        friend class AssetDatabase;

    public:
        virtual ~Asset() = default;
        virtual std::string GetMetaInfo() const { return std::string("Metadata info is not implemented for this asset type."); }
        constexpr AssetID GetAssetID() const { return m_assetId; }
        constexpr uint32_t GetAssetVersion() const { return m_version; }
        constexpr uint64_t GetAssetHash() const { return ((uint64_t)m_version << 32) | ((uint64_t)m_assetId & 0xFFFFFFFF); }
        inline const std::string& GetFileName() const { return m_assetId.to_string(); }
        bool operator==(const Asset& other) const { return m_assetId == ((Asset&)other).m_assetId; }

        template<typename T>
        static bool IsValidExtension(const std::string& extension);

        template<typename T>
        [[nodiscard]] static Utilities::Ref<T> Create();

    private:
        AssetID m_assetId = 0u;
        uint32_t m_version = 0u;
    };

    template<typename ... Args>
    class IAssetImport : public Utilities::NoCopy
    {
        friend class AssetDatabase;
        virtual void AssetImport(const char* filepath, Args ... args) = 0;
    };

    template<typename ... Args>
    class AssetWithImport : public Asset, public IAssetImport<Args...> {};
}