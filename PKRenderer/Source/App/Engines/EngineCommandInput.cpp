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
        m_sequencer(sequencer),
        m_isElevated(Platform::GetProcIsElevated())
    {
        m_inputKeyCommands.memory.Copy(keyConfig->InputKeyCommands.memory);
        m_inputKeyCommands.count = keyConfig->InputKeyCommands.count;
        keyConfig->CommandInputKeys.TryGetKey("Console.Toggle", &m_keyToggleConsole);
    }

    void EngineCommandInput::Step(IGUIRenderer* gui)
    {
        if (m_waitingInput)
        {
            const auto inputSymbol = m_isElevated ? '#' : '$';
            FixedString256 text("%c%s", inputSymbol, m_lines[m_lineEdit].c_str());
            FixedString256 hint("%c%s", inputSymbol, m_lineHint.c_str());
            constexpr color32 COLOR_BG(0, 0, 0, 192);
            constexpr color32 COLOR_FG(255, 255, 255, 127);
            constexpr color32 COLOR_HINT(127, 127, 127, 255);
            constexpr color32 COLOR_TEXT(255, 255, 255, 255);
            const auto renderArea = gui->GUIGetRenderAreaRect();
            const short4 rectWindow(renderArea.x + 4, renderArea.y + 4, renderArea.z - 8, 32);
            const short4 rectText(rectWindow.x + 8, rectWindow.y + 4, rectWindow.z - 16, rectWindow.w - 8);
            gui->GUIDrawRect(COLOR_BG, rectWindow);
            gui->GUIDrawWireRect(COLOR_FG, rectWindow, 1);
            
            auto rectTextOut = gui->GUIDrawText(COLOR_TEXT, rectText, text.c_str(), FontStyle().SetSize(16.0f).SetAlign({ 0.0f, 0.5f }).SetClip(true));
            auto rectTextHint = short4(rectTextOut.x + rectTextOut.z + 1, rectText.y, rectText.z - rectTextOut.z - 1, rectText.w);

            // Draw hint starting at the end of user input.
            if (hint.Length() > text.Length())
            {
                gui->GUIDrawText(COLOR_HINT, rectTextHint, hint.c_str() + text.Length(), FontStyle().SetSize(16.0f).SetAlign({0.0f, 0.5f}).SetClip(true));
            }

            // Draw wide box caret. Offsets hard coded as I can't be bothered to get the actual font data here. 
            if (m_caretTimer < 500u)
            {
                gui->GUIDrawRect(COLOR_TEXT, short4(rectText.x + text.Length() * 8, rectText.y + 5, 6, rectText.w - 10));
            }
        }
    }

    void EngineCommandInput::OnStepFrameUpdate(FrameContext* ctx)
    {
        if (ctx->input.lastDeviceState.device == ctx->window->GetNative())
        {
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
                m_caretTimer += (uint32_t)(ctx->time.deltaTime * 1000.0);
                m_caretTimer %= 1000u;

                if (ProcessConsoleInput(ctx))
                {
                    m_caretTimer = 0u;
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
    }

    void EngineCommandInput::Step(AssetImportEvent<Config<InputKeyConfig>>* evt)
    {
        m_inputKeyCommands.memory.Copy(evt->asset->InputKeyCommands.memory);
        m_inputKeyCommands.count = evt->asset->InputKeyCommands.count;
        evt->asset->CommandInputKeys.TryGetKey("Console.Toggle", &m_keyToggleConsole);
    }

    bool EngineCommandInput::ProcessConsoleInput(FrameContext* ctx)
    {
        auto& input = ctx->input.lastDeviceState.state;
        input->ConsumeAll();

        if (input->GetKeyDown(InputKey::Enter))
        {
            m_sequencer->NextRoot<CArgumentConst>({ m_lines[m_lineEdit].c_str() });
            m_lineEdit = (m_lineEdit + 1) % LINE_COUNT;
            m_lineHistory = m_lineEdit;
            m_lines[m_lineEdit].Clear();
            m_lineHint.Clear();
            return true;
        }

        if (input->GetKeyDown(InputKey::Tab) && m_lineHint.Length() > m_lines[m_lineEdit].Length())
        {
            auto tokenPos = m_lineHint.Find(m_lines[m_lineEdit].Length(), '.') + 1ll;
            tokenPos = tokenPos >= 0ll ? tokenPos : m_lineHint.Length();
            m_lines[m_lineEdit] = m_lineHint.Slice(0, (size_t)tokenPos);
            return true;
        }

        if (input->GetKeyRepeat(InputKey::Backspace) && m_lines[m_lineEdit].Length() > 0)
        {
            m_lines[m_lineEdit].Pop();

            if (m_lines[m_lineEdit].Length() == 0)
            {
                m_lineHint.Clear();
            }

            return true;
        }

        if (input->GetKeyRepeat(InputKey::Up) && m_lineHint.Length() > 0)
        {
            m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), ++m_hintIndex);
            return true;
        }

        if (input->GetKeyRepeat(InputKey::Down) && m_lineHint.Length() > 0)
        {
            m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), --m_hintIndex);
            return true;
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

            return true;
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

            return true;
        }

        if (input->character != 0 && !m_lines[m_lineEdit].IsFull())
        {
            m_lines[m_lineEdit].Append((char)input->character);

            if (m_lines[m_lineEdit].Length() > m_lineHint.Length() || 
                m_lines[m_lineEdit].Back() != m_lineHint[m_lines[m_lineEdit].Length() - 1u])
            {
                m_lineHint = CVariableRegister::FindAutoCompleteHint(m_lines[m_lineEdit].c_str(), m_hintIndex);
            }

            return true;
        }

        return false;
    }
}
