#pragma once
#include "Core/Math/Forward.h"
#include "Core/Assets/Asset.h"
#include "Core/Utilities/Mask.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct IESProfileAtlas : public NoCopy, public AssetFactory<IESProfile>
    {
        IESProfileAtlas(size_t initialCapacity);

        void Reserve(size_t capacity);
        void AssetConstruct(IESProfile* memory, const char* filepath) final;
        void AssetDestruct(uint32_t index);

        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;

        operator RHITexture* ();
        operator const RHITexture* () const;

    private:
        RHITextureRef m_texture = nullptr;
        HeapMask m_residency;
    };

    struct IESProfile : public Asset
    {
        IESProfile(IESProfileAtlas* parent, uint32_t index, float lumens, float candelaMax, float candelaAverage) :
            m_parent(parent), 
            m_index(index), 
            m_lumens(lumens),
            m_candelaMax(candelaMax),
            m_candelaAverage(candelaAverage)
        {
        }

        ~IESProfile() 
        {
            m_parent->AssetDestruct(m_index); 
            m_index = 0u; 
        }

        constexpr uint32_t GetAtlasIndex() const { return m_index; }
        constexpr float GetLumens() const { return m_lumens; }
        constexpr float GetCandelaMax() const { return m_candelaMax; }
        constexpr float GetCandelaAverage() const { return m_candelaAverage; }
    
    private:
        IESProfileAtlas* m_parent = nullptr;
        uint32_t m_index = 0u;
        float m_lumens = 0.0f;
        float m_candelaMax = 0.0f;
        float m_candelaAverage = 0.0f;
    };
}
