#pragma once
#include "Core/ApplicationConfig.h"
#include "Core/Services/AssetDataBase.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Tokens/GizmosToken.h"

namespace PK::ECS::Engines
{
    class EngineDebug : public Core::Services::IService, 
                        public Core::Services::ISimpleStep,
                        public Core::Services::IStep<Tokens::IGizmosRenderer>
    {
        public:
            EngineDebug(Core::Services::AssetDatabase* assetDatabase, EntityDatabase* entityDb, const Core::ApplicationConfig* config);
            void Step(int condition) final;
            void Step(Tokens::IGizmosRenderer* gizmos) final;

        private:
            EntityDatabase* m_entityDb;
            Core::Services::AssetDatabase* m_assetDatabase;
            Utilities::Ref<Rendering::Objects::Mesh> m_virtualBaseMesh = nullptr;
    };
}