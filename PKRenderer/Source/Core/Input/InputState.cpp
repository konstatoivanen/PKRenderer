#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "InputState.h"

namespace PK
{
    bool InputState::GetKeyDown(InputKey key) const { return keysCurrent[(uint32_t)key] && !keysPrevious[(uint32_t)key]; }
    bool InputState::GetKeyUp(InputKey key) const { return !keysCurrent[(uint32_t)key] && keysPrevious[(uint32_t)key]; }
    bool InputState::GetKey(InputKey key) const { return keysCurrent[(uint32_t)key]; }
    bool InputState::GetKeyRepeat(InputKey key) const { return GetKeyDown(key) || (GetKey(key) && keysRepeat[(uint32_t)key]); }
    uint32_t InputState::GetKeyTime(InputKey key) const { return keyTimers[(uint32_t)key]; }
    bool InputState::ConsumeKeyDown(InputKey key) { return !keysConsumed[(uint32_t)key].Exchange(true) && GetKeyDown(key); }
    bool InputState::ConsumeKeyUp(InputKey key) { return !keysConsumed[(uint32_t)key].Exchange(true) && GetKeyUp(key); }
    bool InputState::ConsumeKey(InputKey key) { return !keysConsumed[(uint32_t)key].Exchange(true) && GetKey(key); }

    float InputState::GetAxis(InputKey xneg, InputKey xpos) const { return GetKey(xpos) ? 1.0f : GetKey(xneg) ? -1.0f : 0.0f; }
    float2 InputState::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const { return { GetAxis(xneg, xpos), GetAxis(yneg, ypos) }; }
    float3 InputState::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const { return { GetAxis(xneg, xpos), GetAxis(yneg, ypos), GetAxis(zneg, zpos) }; }
    float InputState::GetAxisDown(InputKey xneg, InputKey xpos) const { return GetKeyDown(xpos) ? 1.0f : GetKeyDown(xneg) ? -1.0f : 0.0f; }
    float2 InputState::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const { return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos) }; }
    float3 InputState::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const { return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos), GetAxisDown(zneg, zpos) }; }
    float InputState::ConsumeAxis(InputKey xneg, InputKey xpos) { return ConsumeKey(xpos) ? 1.0f : ConsumeKey(xneg) ? -1.0f : 0.0f; }
    float2 InputState::ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) { return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos) }; }
    float3 InputState::ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) { return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos), ConsumeAxis(zneg, zpos) }; }
    float InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos) { return ConsumeKeyDown(xpos) ? 1.0f : ConsumeKeyDown(xneg) ? -1.0f : 0.0f; }
    float2 InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) { return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos) }; }
    float3 InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) { return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos), ConsumeAxisDown(zneg, zpos) }; }

    bool InputState::GetKeyDown(const InputShortcut& shortcut) const
    {
        auto value = GetKeyDown(shortcut.First);
        if (shortcut.Second != InputKey::None) value &= GetKeyDown(shortcut.Second);
        return value;
    }

    bool InputState::GetKeyUp(const InputShortcut& shortcut) const
    {
        auto value = GetKeyUp(shortcut.First);
        if (shortcut.Second != InputKey::None) value &= GetKeyUp(shortcut.Second);
        return value;
    }

    bool InputState::GetKey(const InputShortcut& shortcut) const
    {
        auto value = GetKey(shortcut.First);
        if (shortcut.Second != InputKey::None) value &= GetKey(shortcut.Second);
        return value;
    }

    bool InputState::GetKeyRepeat(const InputShortcut& shortcut) const
    {
        auto value = GetKeyRepeat(shortcut.First);
        if (shortcut.Second != InputKey::None) value &= GetKeyRepeat(shortcut.Second);
        return value;
    }

    bool InputState::ConsumeKeyDown(const InputShortcut& shortcut)
    {
        if (GetKeyDown(shortcut))
        {
            auto value = !keysConsumed[(uint32_t)shortcut.First].Exchange(true);
            if (shortcut.Second != InputKey::None) value &= !keysConsumed[(uint32_t)shortcut.Second].Exchange(true);
            return value;
        }

        return false;
    }

    bool InputState::ConsumeKeyUp(const InputShortcut& shortcut)
    {
        if (GetKeyUp(shortcut))
        {
            auto value = !keysConsumed[(uint32_t)shortcut.First].Exchange(true);
            if (shortcut.Second != InputKey::None) value &= !keysConsumed[(uint32_t)shortcut.Second].Exchange(true);
            return value;
        }

        return false;
    }

    bool InputState::ConsumeKey(const InputShortcut& shortcut)
    {
        if (GetKey(shortcut))
        {
            auto value = !keysConsumed[(uint32_t)shortcut.First].Exchange(true);
            if (shortcut.Second != InputKey::None) value &= !keysConsumed[(uint32_t)shortcut.Second].Exchange(true);
            return value;
        }

        return false;
    }


    bool InputState::GetKeyDown(const InputTriplet& triplet) const
    {
        if (GetKeyDown(triplet.Primary)) return true;
        if (GetKeyDown(triplet.Secondary)) return true;
        if (GetKeyDown(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::GetKeyUp(const InputTriplet& triplet) const
    {
        if (GetKeyUp(triplet.Primary)) return true;
        if (GetKeyUp(triplet.Secondary)) return true;
        if (GetKeyUp(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::GetKey(const InputTriplet& triplet) const
    {
        if (GetKey(triplet.Primary)) return true;
        if (GetKey(triplet.Secondary)) return true;
        if (GetKey(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::GetKeyRepeat(const InputTriplet& triplet) const
    {
        if (GetKeyRepeat(triplet.Primary)) return true;
        if (GetKeyRepeat(triplet.Secondary)) return true;
        if (GetKeyRepeat(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::ConsumeKeyDown(const InputTriplet& triplet)
    {
        if (ConsumeKeyDown(triplet.Primary)) return true;
        if (ConsumeKeyDown(triplet.Secondary)) return true;
        if (ConsumeKeyDown(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::ConsumeKeyUp(const InputTriplet& triplet)
    {
        if (ConsumeKeyUp(triplet.Primary)) return true;
        if (ConsumeKeyUp(triplet.Secondary)) return true;
        if (ConsumeKeyUp(triplet.Tertiary)) return true;
        return false;
    }

    bool InputState::ConsumeKey(const InputTriplet& triplet)
    {
        if (ConsumeKey(triplet.Primary)) return true;
        if (ConsumeKey(triplet.Secondary)) return true;
        if (ConsumeKey(triplet.Tertiary)) return true;
        return false;
    }


    float InputState::GetAxis(const InputTriplet& xneg, const InputTriplet& xpos) const
    {
       return GetKey(xpos) ? 1.0f : GetKey(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::GetAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos) };
    }

    float3 InputState::GetAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos), GetAxis(zneg, zpos) };
    }

    float InputState::GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos) const
    {
        return GetKeyDown(xpos) ? 1.0f : GetKeyDown(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos)  const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos) };
    }

    float3 InputState::GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos) const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos), GetAxisDown(zneg, zpos) };
    }

    float InputState::ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos)
    {
        return ConsumeKey(xpos) ? 1.0f : ConsumeKey(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos)
    {
        return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos) };
    }

    float3 InputState::ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos)
    {
        return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos), ConsumeAxis(zneg, zpos) };
    }

    float InputState::ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos)
    {
        return ConsumeKeyDown(xpos) ? 1.0f : ConsumeKeyDown(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos)
    {
        return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos) };
    }

    float3 InputState::ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos)
    {
        return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos), ConsumeAxisDown(zneg, zpos) };
    }


    void InputState::ConsumeAll()
    {
        keysConsumed.SetAll(true);
    }

    void InputState::SetKey(InputKey key, bool isDown)
    {
        if (!keysCurrent[(uint32_t)key])
        {
            keyTimers[(uint32_t)key] = 0u;
        }

        keysCurrent[(uint32_t)key] = isDown;
    }

    void InputState::SetCursor(const float2& position, const float2& size)
    {
        auto positionFlipY = float2(position.x, size.y - position.y);
        auto positionNormalized = positionFlipY / size;
        auto positionDelta = positionFlipY - this->cursorPosition;
        this->cursorPosition = positionFlipY;
        this->cursorPositionDelta += positionDelta;
        this->cursorPositionNormalized = positionNormalized;
    }

    void InputState::SwapBuffers(uint32_t deltaMillis)
    {
        keysPrevious = keysCurrent;
        keysConsumed.SetAll(false);
        character = 0;
        cursorPositionDelta = PK_FLOAT2_ZERO;

        for (auto i = 0u; i < (uint32_t)InputKey::EnumCount; ++i)
        {
            keyTimers[i] += deltaMillis;
            keysRepeat[i] = keysPrevious[i] && keyTimers[i] > REPEAT_DELAY && keysRepeat[i] ^ true;
        }
    }
}
