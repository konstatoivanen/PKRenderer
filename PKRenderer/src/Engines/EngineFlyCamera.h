#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Input/InputKeyStructMacros.h"
#include "Core/IService.h"
#include "Core/TimeFrameInfo.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Input, struct InputDevice)

namespace PK::Engines
{
    PK_INPUTKEY_STRUCT_BEGIN(EngineFlyCameraInputKeys)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Forward, W)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Backward, S)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Left, A)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Right, D)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Up, Q)
        PK_INPUTKEY_STRUCT_MEMBER(Move, Down, E)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, LookDrag, Mouse1)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, SpeedUp, MouseScrollUp)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, SpeedDown, MouseScrollDown)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, FovAdd, MouseScrollDown)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, FovSub, MouseScrollUp)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, FovControl, LeftControl)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, ResetSmoothing, LeftShift)
        PK_INPUTKEY_STRUCT_MEMBER(Camera, DollyZoom, LeftShift)
    PK_INPUTKEY_STRUCT_END()

    class EngineFlyCamera : public Core::IService,
        public Core::ControlFlow::IStep<Core::Input::InputDevice*>,
        public Core::ControlFlow::IStep<Core::TimeFrameInfo*>,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>*>
    {
    public:
        EngineFlyCamera(class ECS::EntityDatabase* entityDb, Core::Input::InputKeyConfig* keyConfig);
        virtual void Step(Core::Input::InputDevice* input) final;
        virtual void Step(Core::TimeFrameInfo* time) final { m_timeFrameInfo = *time; }
        virtual void Step(Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>* evt) final { m_keys.SetKeysFrom(evt->asset); }

        void TransformsLog() const;
        void TransformsReset();

    private:
        ECS::EntityDatabase* m_entityDb;
        Core::TimeFrameInfo m_timeFrameInfo{};
        EngineFlyCameraInputKeys m_keys{};
    };
}