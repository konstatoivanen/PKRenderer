#include "PrecompiledHeader.h"
#include "Core/Input/InputDevice.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/CLI/Log.h"
#include "EngineCommandInput.h"

namespace PK::App
{
    EngineCommandInput::EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig) :
        m_sequencer(sequencer)
    {
        m_inputKeyCommands = keyConfig->InputKeyCommands;
        keyConfig->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }

    void EngineCommandInput::Step(InputDevice* input)
    {
        for (auto& binding : m_inputKeyCommands)
        {
            if (input->GetKeyDown(binding.first))
            {
                m_sequencer->NextRoot<CArgumentConst>({ binding.second.c_str() });
            }
        }

        if (input->GetKeyDown(m_keyBeginInput))
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
        m_inputKeyCommands = evt->asset->InputKeyCommands;
        evt->asset->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }
}