#pragma once
#include "Core/Services/AssetDataBase.h"
#include "Core/ApplicationConfig.h"
#include "ECS/EntityDatabase.h"

namespace PK::ECS::Engines
{
    class EngineDebug : public Core::Services::IService, public Core::Services::ISimpleStep
    {
    public:
        EngineDebug(Core::Services::AssetDatabase* assetDatabase, EntityDatabase* entityDb, const Core::ApplicationConfig* config);
        void Step(int condition) override;

    private:
        EntityDatabase* m_entityDb;
        Core::Services::AssetDatabase* m_assetDatabase;
        Utilities::Ref<Rendering::Objects::Mesh> m_virtualBaseMesh = nullptr;
    };
}