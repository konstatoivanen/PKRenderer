#include "PrecompiledHeader.h"
#include "Core/Input/InputState.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/Rendering/Font.h"
#include "Core/ControlFlow/Sequencer.h"
#include "App/FrameContext.h"
#include "App/Renderer/IGUIRenderer.h"
#include "EngineCommandInput.h"

namespace PK::App
{
    EngineCommandInput::EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig) :
        m_sequencer(sequencer)
    {
        m_inputKeyCommands.memory.Copy(keyConfig->InputKeyCommands.memory);
        m_inputKeyCommands.count = keyConfig->InputKeyCommands.count;
        keyConfig->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }

    void EngineCommandInput::Step(IGUIRenderer* gui)
    {
        if (m_waitingInput)
        {
            FixedString256 text(">:%s%c",m_commandInput.c_str(), m_caretBlink ? '|' : ' ');
            constexpr auto COLOR_BG = color32(0, 0, 0, 192);
            constexpr auto COLOR_FG = color32(255, 255, 255, 127);
            constexpr auto COLOR_TEXT = color32(255, 255, 255, 127);
            const auto renderArea = gui->GUIGetRenderAreaRect();
            const auto rectWindow = short4(renderArea.x + 4, renderArea.y + 4, renderArea.z - 8, 32);
            const auto rectInput = short4(rectWindow.x + 8, rectWindow.y + 4, rectWindow.z - 16, rectWindow.w - 8);
            gui->GUIDrawRect(COLOR_BG, rectWindow);
            gui->GUIDrawWireRect(COLOR_FG, rectWindow, 1);
            gui->GUIDrawText(COLOR_TEXT, rectInput, text.c_str(), FontStyle().SetSize(16.0f).SetAlign({ 0.0f, 0.5f }).SetClip(true));
        }
    }

    void EngineCommandInput::OnStepFrameUpdate(FrameContext* ctx)
    {
        if (ctx->input.lastDeviceState.device != ctx->window->GetNative())
        {
            return;
        }

        auto& input = ctx->input.lastDeviceState.state;
        auto bindings = m_inputKeyCommands.GetBindings();
        auto isWaitingInput = m_waitingInput;
        
        if (input.GetKeyDown(m_keyBeginInput))
        {
            m_waitingInput ^= true;
        }

        // Skip first frame as we dont want to capture keys pressed during activation.
        if (isWaitingInput && m_waitingInput)
        {
            m_caretBlink = uint64_t(ctx->time.time * 2.0) & 0x1u;

            if (input.GetKeyDown(InputKey::Enter))
            {
                m_sequencer->NextRoot<CArgumentConst>({ m_commandInput.c_str() });
                m_commandHistory[m_commandHistoryHead % COMMAND_HISTORY_SIZE] = m_commandInput;
                m_commandHistoryView = ++m_commandHistoryHead;
                m_commandInput.Clear();
            }
            else if (input.GetKey(InputKey::Backspace))
            {
                m_commandInput.Pop();
            }
            else if (input.GetKeyDown(InputKey::Up))
            {
                m_commandInput = m_commandHistory[--m_commandHistoryView % COMMAND_HISTORY_SIZE];
            }
            else if (input.GetKeyDown(InputKey::Down))
            {
                m_commandInput = m_commandHistory[++m_commandHistoryView % COMMAND_HISTORY_SIZE];
            }
            else if (input.character != 0 && !m_commandInput.IsFull())
            {
                m_commandInput.Append((char)input.character);
            }

            return;
        }

        for (auto i = 0u; i < m_inputKeyCommands.count; ++i)
        {
            if (input.GetKeyDown(bindings[i].key))
            {
                m_sequencer->NextRoot<CArgumentConst>({ bindings[i].command });
            }
        }
    }

    void EngineCommandInput::Step(AssetImportEvent<InputKeyConfig>* evt)
    {
        m_inputKeyCommands.memory.Copy(evt->asset->InputKeyCommands.memory);
        m_inputKeyCommands.count = evt->asset->InputKeyCommands.count;
        evt->asset->CommandInputKeys.TryGetKey("Console.BeginInput", &m_keyBeginInput);
    }
}
