#pragma once
namespace PK
{
    template<typename T>
    struct AssetImportEvent
    {
        class AssetDatabase* assetDatabase;
        T* asset;
    };
}