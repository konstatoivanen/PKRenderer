#include "PrecompiledHeader.h"
#include "Core/Input/InputState.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/CLI/Log.h"
#include "App/FrameContext.h"
#include "EngineCommandInput.h"

namespace PK::App
{
    EngineCommandInput::EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig) :
        m_sequencer(sequencer)
    {
        m_inputKeyCommands.memory.CopyFrom(keyConfig->InputKeyCommands.memory);
        m_inputKeyCommands.count = keyConfig->InputKeyCommands.count;
        keyConfig->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }

    void EngineCommandInput::OnStepFrameUpdate(FrameContext* ctx)
    {
        if (ctx->input.lastDeviceState.device != ctx->window->GetNative())
        {
            return;
        }

        auto& input = ctx->input.lastDeviceState.state;
        auto bindings = m_inputKeyCommands.GetBindings();

        for (auto i = 0u; i < m_inputKeyCommands.count; ++i)
        {
            if (input.GetKeyDown(bindings[i].key))
            {
                m_sequencer->NextRoot<CArgumentConst>({ bindings[i].command });
            }
        }

        if (input.GetKeyDown(m_keyBeginInput))
        {
            StaticLog::LogNewLine();
            StaticLog::Log(PK_LOG_LVL_INFO, PK_LOG_COLOR_INPUT, "Awaiting command input...");

            std::string command;
            std::getline(std::cin, command);
            m_sequencer->NextRoot<CArgumentConst>({ command.c_str() });
        }
    }

    void EngineCommandInput::Step(AssetImportEvent<InputKeyConfig>* evt)
    {
        m_inputKeyCommands.memory.CopyFrom(evt->asset->InputKeyCommands.memory);
        m_inputKeyCommands.count = evt->asset->InputKeyCommands.count;
        evt->asset->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }
}
