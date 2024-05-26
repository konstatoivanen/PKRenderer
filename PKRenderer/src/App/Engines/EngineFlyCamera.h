#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Input/InputKeyStructMacros.h"
#include "Core/TimeFrameInfo.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputDevice)

namespace PK::App
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

    class EngineFlyCamera : 
        public IStep<InputDevice*>,
        public IStep<TimeFrameInfo*>,
        public IStep<AssetImportEvent<InputKeyConfig>*>
    {
    public:
        EngineFlyCamera(EntityDatabase* entityDb, InputKeyConfig* keyConfig);
        virtual void Step(InputDevice* input) final;
        virtual void Step(TimeFrameInfo* time) final { m_timeFrameInfo = *time; }
        virtual void Step(AssetImportEvent<InputKeyConfig>* evt) final { m_keys.SetKeysFrom(evt->asset); }

        void TransformsLog() const;
        void TransformsReset();

    private:
        EntityDatabase* m_entityDb;
        TimeFrameInfo m_timeFrameInfo{};
        EngineFlyCameraInputKeys m_keys{};
    };
}