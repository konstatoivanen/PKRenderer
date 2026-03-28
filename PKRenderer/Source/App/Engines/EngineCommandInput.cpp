#include "PrecompiledHeader.h"
#include "Core/Input/InputState.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/Rendering/Font.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/CLI/CVariableRegister.h"
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
        keyConfig->CommandInputKeys.TryGetKey("Console.Toggle", &m_keyToggleConsole);
    }

    void EngineCommandInput::Step(IGUIRenderer* gui)
    {
        if (m_waitingInput)
        {
            FixedString256 text(">:%s%c",m_commandInput.c_str(), m_caretBlink ? '|' : ' ');
            FixedString256 hint(">:%s",m_commandHint.c_str());
            constexpr auto COLOR_BG = color32(0, 0, 0, 192);
            constexpr auto COLOR_FG = color32(255, 255, 255, 127);
            constexpr auto COLOR_HINT = color32(255, 255, 255, 127);
            constexpr auto COLOR_TEXT = color32(255, 255, 255, 255);
            const auto renderArea = gui->GUIGetRenderAreaRect();
            const auto rectWindow = short4(renderArea.x + 4, renderArea.y + 4, renderArea.z - 8, 32);
            const auto rectText = short4(rectWindow.x + 8, rectWindow.y + 4, rectWindow.z - 16, rectWindow.w - 8);
            gui->GUIDrawRect(COLOR_BG, rectWindow);
            gui->GUIDrawWireRect(COLOR_FG, rectWindow, 1);
            gui->GUIDrawText(COLOR_HINT, rectText, hint.c_str(), FontStyle().SetSize(16.0f).SetAlign({ 0.0f, 0.5f }).SetClip(true));
            gui->GUIDrawText(COLOR_TEXT, rectText, text.c_str(), FontStyle().SetSize(16.0f).SetAlign({ 0.0f, 0.5f }).SetClip(true));
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
        
        if (input.GetKeyDown(m_keyToggleConsole))
        {
            m_waitingInput ^= true;
        }

        // Skip first frame as we dont want to capture keys pressed during activation.
        if (isWaitingInput && m_waitingInput)
        {
            m_caretBlink = uint64_t(ctx->time.time * 2.0) & 0x1u;
            auto scroll = input.GetKeyDown(InputKey::Up) ? 1 : input.GetKeyDown(InputKey::Down) ? -1 : 0;

            if (input.GetKeyDown(InputKey::Enter))
            {
                m_sequencer->NextRoot<CArgumentConst>({ m_commandInput.c_str() });
                m_commandHistory[m_commandHistoryHead % COMMAND_HISTORY_SIZE] = m_commandInput;
                m_commandHistoryView = ++m_commandHistoryHead;
                m_commandInput.Clear();
                m_commandHint.Clear();
            }
            else if (input.GetKey(InputKey::Backspace))
            {
                if (input.GetKeyDown(InputKey::Backspace))
                {
                    m_eraseTimer = ERASE_REPEAT_DELAY;
                    m_commandInput.Pop();
                }
                else if (m_eraseTimer <= 0.0)
                {
                    m_eraseTimer = m_eraseTimer < ERASE_REPEAT_RATE ? ERASE_REPEAT_RATE : m_eraseTimer;
                    m_commandInput.Pop();
                }

                m_eraseTimer -= ctx->time.deltaTime;

                if (m_commandInput.Length() == 0)
                {
                    m_commandHint.Clear();
                }
            }
            else if (scroll != 0)
            {
                if (m_commandHint.Length() > 0)
                {
                    m_commandHintOffset += scroll;
                    m_commandHint = CVariableRegister::FindAutoCompleteHint(m_commandInput.c_str(), m_commandInput.Length(), m_commandHintOffset);
                }
                else
                {
                    m_commandHistoryView -= scroll;
                    m_commandInput = m_commandHistory[m_commandHistoryView % COMMAND_HISTORY_SIZE];
                }
            }
            else if (input.GetKeyDown(InputKey::Tab) && m_commandHint.Length() > m_commandInput.Length())
            {
                m_commandInput = m_commandHint;
            }
            else if (input.character != 0 && !m_commandInput.IsFull())
            {
                m_commandInput.Append((char)input.character);

                if (m_commandInput.Length() > m_commandHint.Length() || m_commandInput[m_commandInput.Length() - 1u] != m_commandHint[m_commandInput.Length() - 1u])
                {
                    m_commandHint = CVariableRegister::FindAutoCompleteHint(m_commandInput.c_str(), m_commandInput.Length(), m_commandHintOffset);
                }
            }

            input.ConsumeAll();
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
        evt->asset->CommandInputKeys.TryGetKey("Console.Toggle", &m_keyToggleConsole);
    }
}
