#pragma once
namespace PK::Core::Assets
{
    template<typename T>
    struct AssetImportEvent
    {
        class AssetDatabase* assetDatabase;
        T* asset;
    };
}