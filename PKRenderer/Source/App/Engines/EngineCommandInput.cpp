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
            FixedString256 text(">:%s%c", m_lines[m_lineEdit].c_str(), m_cursorBlink ? '|' : ' ');
            FixedString256 hint(">:%s",m_lineHint.c_str());
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

        if (input->GetKeyDown(m_keyToggleConsole))
        {
            m_waitingInput ^= true;
        }

        // Skip first frame as we dont want to capture keys pressed during activation.
        if (isWaitingInput && m_waitingInput)
        {
            m_cursorBlink = uint64_t(ctx->time.time * 2.0) & 0x1u;
            
            input->ConsumeAll();

            if (input->GetKeyDown(InputKey::Enter))
            {
                m_sequencer->NextRoot<CArgumentConst>({ m_lines[m_lineEdit].c_str() });
                m_lineEdit = (m_lineEdit + 1) % LINE_COUNT;
                m_lineHistory = m_lineEdit;
                m_lines[m_lineEdit].Clear();
                m_lineHint.Clear();
                return;
            }

            if (input->GetKeyDown(InputKey::Tab) && m_lineHint.Length() > m_lines[m_lineEdit].Length())
            {
                auto tokenPos = m_lineHint.FindPos(m_lines[m_lineEdit].Length(), '.') + 1ll;
                tokenPos = tokenPos >= 0ll ? tokenPos : m_lineHint.Length();
                m_lines[m_lineEdit] = m_lineHint.SubString(0, (size_t)tokenPos);
                return;
            }

            if (input->GetKeyRepeat(InputKey::Backspace) && m_lines[m_lineEdit].Length() > 0)
            {
                m_lines[m_lineEdit].Pop();

                if (m_lines[m_lineEdit].Length() == 0)
                {
                    m_lineHint.Clear();
                }

                return;
            }

            if (input->GetKeyRepeat(InputKey::Up) && m_lineHint.Length() > 0)
            {
                m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), ++m_hintIndex);
                return;
            }

            if (input->GetKeyRepeat(InputKey::Down) && m_lineHint.Length() > 0)
            {
                m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), --m_hintIndex);
                return;
            }

            if (input->GetKeyRepeat(InputKey::Up))
            {
                auto history = m_lineHistory;

                do
                {
                    history = (history - 1) % LINE_COUNT;
                }
                while (history != m_lineEdit && !m_lines[history][0]);

                if (history != m_lineEdit && m_lines[history][0])
                {
                    m_lines[m_lineEdit] = m_lines[history];
                    m_lineHistory = history;
                }

                return;
            }

            if (input->GetKeyRepeat(InputKey::Down) && m_lineEdit != m_lineHistory)
            {
                do
                {
                    m_lineHistory = (m_lineHistory + 1) % LINE_COUNT;
                } 
                while (m_lineHistory != m_lineEdit && !m_lines[m_lineHistory][0]);

                if (m_lineEdit == m_lineHistory)
                {
                    m_lines[m_lineEdit].Clear();
                }
                else
                {
                    m_lines[m_lineEdit] = m_lines[m_lineHistory];
                }

                return;
            }

            if (input->character != 0 && !m_lines[m_lineEdit].IsFull())
            {
                m_lines[m_lineEdit].Append((char)input->character);

                if (m_lines[m_lineEdit].Length() > m_lineHint.Length() || m_lines[m_lineEdit].Back() != m_lineHint.Back())
                {
                    m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), m_hintIndex);
                }

                return;
            }

            return;
        }

        for (auto i = 0u; i < m_inputKeyCommands.count; ++i)
        {
            if (input->GetKeyDown(bindings[i].key))
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
