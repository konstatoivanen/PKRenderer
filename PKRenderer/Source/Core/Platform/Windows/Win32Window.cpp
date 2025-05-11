#include "PrecompiledHeader.h"
#if PK_PLATFORM_WINDOWS
#include "Win32Driver.h"
#include "Win32Window.h"
#include "Windowsx.h"

namespace PK
{
    static void GetHMONITORContentScale(HMONITOR handle, float* xscale, float* yscale)
    {
        UINT xdpi, ydpi;
        *xscale = 0.f;
        *yscale = 0.f;

        if (PK_PLATFORM_WINDOWS_IS_8_1_OR_GREATER())
        {
            if (PK_GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) != S_OK)
            {
                return;
            }
        }
        else
        {
            const HDC dc = ::GetDC(NULL);
            xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
            ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
            ::ReleaseDC(NULL, dc);
        }

        *xscale = xdpi / (float)USER_DEFAULT_SCREEN_DPI;
        *yscale = ydpi / (float)USER_DEFAULT_SCREEN_DPI;
    }

    static int32_t GetRemappedScanCode(WPARAM wParam, LPARAM lParam)
    {
        int32_t scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));

        if (!scancode)
        {
            // NOTE: Some synthetic key messages have a scancode of zero
            // HACK: Map the virtual key back to a usable scancode
            scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
        }

        // HACK: Alt+PrtSc has a different scancode than just PrtSc
        if (scancode == 0x54)
            scancode = 0x137;

        // HACK: Ctrl+Pause has a different scancode than just Pause
        if (scancode == 0x146)
            scancode = 0x45;

        // HACK: CJK IME sets the extended bit for right Shift
        if (scancode == 0x136)
            scancode = 0x36;

        return scancode;
    }

    static bool HandleModifierKeys(WPARAM wParam, LPARAM lParam, InputKey* key)
    {
        if (wParam == VK_CONTROL)
        {
            if (HIWORD(lParam) & KF_EXTENDED)
            {
                // Right side keys have the extended key bit set
                *key = InputKey::RightControl;
            }
            else
            {
                // NOTE: Alt Gr sends Left Ctrl followed by Right Alt
                // HACK: We only want one event for Alt Gr, so if we detect
                //       this sequence we discard this Left Ctrl message now
                //       and later report Right Alt normally
                MSG next;
                const DWORD time = GetMessageTime();

                if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE) &&
                    (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP || next.message == WM_SYSKEYUP) &&
                    (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) && next.time == time))
                {
                    // Next message is Right Alt down so discard this
                    return false;
                }

                // This is a regular Left Ctrl message
                *key = InputKey::LeftControl;
            }
        }

        if (wParam == VK_PROCESSKEY)
        {
            return false;
        }

        return true;
    }

    static InputKey GetInputKeyFromMouseButton(UINT uMsg, WPARAM wParam)
    {
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
        {
            return InputKey::Mouse1;
        }

        if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
        {
            return InputKey::Mouse2;
        }

        if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
        {
            return InputKey::Mouse3;
        }

        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
        {
            return InputKey::Mouse4;
        }

        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
        {
            return InputKey::Mouse5;
        }

        if (GET_XBUTTON_WPARAM(wParam) == 0x0004)
        {
            return InputKey::Mouse6;
        }

        if (GET_XBUTTON_WPARAM(wParam) == 0x0008)
        {
            return InputKey::Mouse7;
        }

        return InputKey::Mouse8;
    }


    Win32Window::Win32Window(const PlatformWindowDescriptor& descriptor)
    {
        state.isPendingActivate = descriptor.activateOnFirstShow;
        options.useDpiScaling = descriptor.useDpiScaling;
        size_min[0] = descriptor.resolutionMin[0];
        size_min[1] = descriptor.resolutionMin[1];
        size_max[0] = descriptor.resolutionMax[0];
        size_max[1] = descriptor.resolutionMax[1];

        int32_t framePos[2]{};
        int32_t frameSize[2]{};

        DWORD style = WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        DWORD styleEx = WS_EX_APPWINDOW;

        if (descriptor.isFloating)
        {
            styleEx |= WS_EX_TOPMOST;
        }

        if (descriptor.isResizable)
        {
            style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        }

        {
            RECT rect = { 0, 0, descriptor.resolution[0], descriptor.resolution[1] };
            ::AdjustWindowRectEx(&rect, style, FALSE, styleEx);
            framePos[0] = descriptor.position[0] == -1 ? CW_USEDEFAULT : descriptor.position[0] + rect.left;
            framePos[1] = descriptor.position[1] == -1 ? CW_USEDEFAULT : descriptor.position[1] + rect.top;
            frameSize[0] = rect.right - rect.left;
            frameSize[1] = rect.bottom - rect.top;
        }

        auto wideTitle = Parse::ToWideString(descriptor.title.c_str(), descriptor.title.length());

        handle = ::CreateWindowExW
        (
            styleEx,
            Win32Window::CLASS_MAIN,
            wideTitle.c_str(),
            style,
            framePos[0], framePos[1],
            frameSize[0], frameSize[1],
            NULL,
            NULL,
            PlatformDriver::GetNative<Win32Driver>()->instance,
            (LPVOID)&descriptor
        );

        if (!handle)
        {
            throw std::runtime_error("Failed to create a window through: CreateWindowExW.");
        }

        ::SetPropW(handle, Win32Window::WINDOW_PROP, this);

        PK_ChangeWindowMessageFilterEx(handle, WM_DROPFILES, MSGFLT_ALLOW, NULL);
        PK_ChangeWindowMessageFilterEx(handle, WM_COPYDATA, MSGFLT_ALLOW, NULL);
        PK_ChangeWindowMessageFilterEx(handle, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);

        {
            RECT rect = { 0, 0, descriptor.resolution[0], descriptor.resolution[1] };
            WINDOWPLACEMENT wp = { sizeof(wp) };

            const HMONITOR mh = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);

            if (descriptor.useDpiScaling)
            {
                float xscale, yscale;
                GetHMONITORContentScale(mh, &xscale, &yscale);
                rect.right = xscale > 0.0f ? (int)(rect.right * xscale) : rect.right;
                rect.bottom = yscale > 0.0f ? (int)(rect.bottom * yscale) : rect.bottom;
            }

            ::AdjustWindowRectEx(&rect, style, FALSE, styleEx);
            ::GetWindowPlacement(handle, &wp);
            ::OffsetRect(&rect, wp.rcNormalPosition.left - rect.left, wp.rcNormalPosition.top - rect.top);

            wp.rcNormalPosition = rect;
            wp.showCmd = SW_HIDE;
            ::SetWindowPlacement(handle, &wp);
        }

        ::DragAcceptFiles(handle, TRUE);

        {
            RECT rect;
            ::GetClientRect(handle, &rect);
            cached_clientsize[0] = rect.right;
            cached_clientsize[1] = rect.bottom;
        }

        if (descriptor.isVisible)
        {
            SetVisible(true);
        }
    }

    Win32Window::~Win32Window()
    {
        state.isVisible = false;
        OnFocusChanged(false);
        UpdateCursor();

        if (handle)
        {
            ::RemovePropW(handle, Win32Window::WINDOW_PROP);
            ::DestroyWindow(handle);
            handle = nullptr;
        }

        if (icon)
        {
            ::DestroyIcon(icon);
        }
    }


    int4 Win32Window::GetRect() const
    {
        POINT pos = { 0, 0 };
        ::ClientToScreen(handle, &pos);
        return { pos.x, pos.y, cached_clientsize[0], cached_clientsize[1] };
    }

    int2 Win32Window::GetMonitorResolution() const
    {
        auto monitor = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        ::GetMonitorInfoW(monitor, &mi);
        return { mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top };
    }

    int2 Win32Window::GetCursorPosition() const
    {
        POINT pos{ 0, 0 };
        ::GetCursorPos(&pos);
        ::ScreenToClient(handle, &pos);
        return { pos.x, pos.y };
    }


    void Win32Window::SetRect(const int4& newRect)
    {
        const auto currentRect = GetRect();
        const bool changePosition = currentRect.x != newRect.x || currentRect.y != newRect.y;
        const bool changeSize = currentRect.z != newRect.z || currentRect.w != newRect.w;

        if (!changePosition && !changeSize)
        {
            return;
        }
        
        auto x = newRect.x;
        auto y = newRect.y;
        auto width = newRect.z;
        auto height = newRect.w;

        if (changeSize)
        {
            cached_clientsize[0] = newRect.x;
            cached_clientsize[1] = newRect.y;
            DispatchWindowEvent(PlatformWindowEvent::Resize);
        }

        if (!options.isBorderless)
        {
            // Get window info
            WINDOWINFO winInfo;
            ZeroMemory(&winInfo, sizeof(WINDOWINFO));
            winInfo.cbSize = sizeof(winInfo);
            ::GetWindowInfo(handle, &winInfo);

            RECT rect = { 0, 0, width, height };
            // Adjust rectangle from client size to window size
            ::AdjustWindowRectEx(&rect, winInfo.dwStyle, FALSE, winInfo.dwExStyle);
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;

            // Little hack but works great
            rect = { x, y, width, height };
            AdjustWindowRectEx(&rect, winInfo.dwStyle, FALSE, winInfo.dwExStyle);
            x = rect.left;
            y = rect.top;
        }

        ::SetWindowPos(handle, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

        {
            RECT rect;
            ::GetClientRect(handle, &rect);
            cached_clientsize[0] = rect.right;
            cached_clientsize[1] = rect.bottom;
        }
    }

    void Win32Window::SetCursorPosition(const int2& position)
    {
        auto isLocked = options.cursorLock && GetIsFocused();
        auto isVisible = !(options.cursorHide && GetIsFocused());
        auto isDisabled = isLocked && !isVisible;

        if (isDisabled)
        {
            virt_cursorpos[0] = position.x;
            virt_cursorpos[1] = position.y;
        }
        else
        {
            cached_cursorpos[0] = position.x;
            cached_cursorpos[1] = position.y;
            POINT pos = { position.x, position.y };
            ::ClientToScreen(handle, &pos);
            ::SetCursorPos(pos.x, pos.y);
        }
    }

    void Win32Window::SetCursorLock(bool lock, bool hide)
    {
        options.cursorLock = lock;
        options.cursorHide = hide;
        UpdateCursor();
    }

    void Win32Window::SetRawMouseInput(bool value)
    {
        options.useRawMouseInput = value;
        UpdateCursor();
    }

    void Win32Window::SetIcon(unsigned char* pixels, const int2& resolution)
    {
        HICON iconBig = NULL;
        HICON iconSmall = NULL;

        if (pixels)
        {
            int i;
            HDC dc;
            HBITMAP color, mask;
            BITMAPV5HEADER bi;
            ICONINFO ii;
            unsigned char* target = NULL;
            unsigned char* source = pixels;

            ZeroMemory(&bi, sizeof(bi));
            bi.bV5Size = sizeof(bi);
            bi.bV5Width = resolution.x;
            bi.bV5Height = -resolution.y;
            bi.bV5Planes = 1;
            bi.bV5BitCount = 32;
            bi.bV5Compression = BI_BITFIELDS;
            bi.bV5RedMask = 0x00ff0000;
            bi.bV5GreenMask = 0x0000ff00;
            bi.bV5BlueMask = 0x000000ff;
            bi.bV5AlphaMask = 0xff000000;

            dc = ::GetDC(NULL);
            color = ::CreateDIBSection(dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&target, NULL, (DWORD)0);
            ::ReleaseDC(NULL, dc);

            if (!color)
            {
                return;
            }

            mask = ::CreateBitmap(resolution.x, resolution.y, 1, 1, NULL);

            if (!mask)
            {
                ::DeleteObject(color);
                return;
            }

            for (i = 0; i < resolution.x * resolution.y; i++)
            {
                target[0] = source[2];
                target[1] = source[1];
                target[2] = source[0];
                target[3] = source[3];
                target += 4;
                source += 4;
            }

            ZeroMemory(&ii, sizeof(ii));
            ii.fIcon = true;
            ii.xHotspot = 0;
            ii.yHotspot = 0;
            ii.hbmMask = mask;
            ii.hbmColor = color;

            iconSmall = ::CreateIconIndirect(&ii);
            iconBig = iconSmall;
            ::DeleteObject(color);
            ::DeleteObject(mask);
        }
        else
        {
            iconBig = (HICON)::GetClassLongPtrW(handle, GCLP_HICON);
            iconSmall = (HICON)::GetClassLongPtrW(handle, GCLP_HICONSM);
        }

        ::SendMessageW(handle, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
        ::SendMessageW(handle, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);

        if (icon)
        {
            ::DestroyIcon(icon);
        }

        if (pixels)
        {
            icon = iconSmall;
        }
    }


    void Win32Window::SetVisible(bool value)
    {
        if (value == state.isVisible)
        {
            return;
        }

        if (value)
        {
            auto showCommand = SW_SHOWNA;

            if (state.isPendingActivate)
            {
                STARTUPINFOW si = { sizeof(si) };
                GetStartupInfoW(&si);
                if (si.dwFlags & STARTF_USESHOWWINDOW)
                {
                    showCommand = si.wShowWindow;
                }
            }

            ::ShowWindow(handle, showCommand);
            state.isVisible = true;
            state.isPendingActivate = false;
            DispatchWindowEvent(PlatformWindowEvent::Visible);
        }
        else
        {
            ::ShowWindow(handle, SW_HIDE);
            state.isVisible = false;
            OnFocusChanged(false);
            UpdateCursor();
            DispatchWindowEvent(PlatformWindowEvent::Invisible);
        }
    }

    void Win32Window::Minimize()
    {
        ::ShowWindow(handle, SW_MINIMIZE);
    }

    void Win32Window::Maximize()
    {
        scope.inMaximize = true;
        ::ShowWindow(handle, SW_MAXIMIZE);
        scope.inMaximize = false;
    }

    void Win32Window::SetBorderless(bool isBorderless, bool maximize)
    {
        SetFullScreen(false);

        if (state.isMaximized)
        {
            Restore();
        }

        options.isBorderless = isBorderless;

        if (::IsIconic(handle))
        {
            ::ShowWindow(handle, SW_RESTORE);
        }
        else
        {
            ::SetActiveWindow(handle);
        }

        if (isBorderless)
        {
            LONG lStyle = GetWindowLong(handle, GWL_STYLE);
            lStyle &= ~(WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_CAPTION);
            lStyle |= WS_POPUP;
            lStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

            SetWindowLong(handle, GWL_STYLE, lStyle);
            SetWindowPos(handle, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            if (maximize)
            {
                ::ShowWindow(handle, SW_SHOWMAXIMIZED);
            }
            else
            {
                ::ShowWindow(handle, SW_SHOW);
            }
        }
        else
        {
            LONG lStyle = GetWindowLong(handle, GWL_STYLE);
            lStyle &= ~(WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
            lStyle |= WS_OVERLAPPED | WS_SYSMENU | WS_BORDER | WS_CAPTION;

            SetWindowLong(handle, GWL_STYLE, lStyle);
            const auto clientSize = GetResolution();
            const auto desktopSize = Platform::GetDesktopSize();
            // Move window and half size if it is larger than desktop size
            if (clientSize.x >= desktopSize.x && clientSize.y >= desktopSize.y)
            {
                const auto halfSize = desktopSize / 2;
                const auto middlePos = halfSize / 2;
                ::SetWindowPos(handle, nullptr, middlePos.x, middlePos.y, halfSize.x, halfSize.y, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else
            {
                ::SetWindowPos(handle, nullptr, 0, 0, clientSize.x, clientSize.y, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }

            if (maximize)
            {
                Maximize();
            }
            else
            {
                ::ShowWindow(handle, SW_SHOW);
            }
        }
    }

    void Win32Window::SetFullScreen(bool value)
    {
        scope.inFullScreen = true;

        if (state.isFullScreen != value)
        {
            state.isFullScreen = value;
            DispatchWindowEvent(value ? PlatformWindowEvent::FullScreenRequest : PlatformWindowEvent::FullScreenExit);
        }

        if (!state.isFullScreen)
        {
            ShowWindow(handle, SW_NORMAL);
        }

        scope.inFullScreen = false;
    }
    
    void Win32Window::Restore()
    { 
        ::ShowWindow(handle, SW_RESTORE);
    }


    void Win32Window::OnPollEvents()
    {
        const int32_t vks[4]{ VK_LSHIFT, VK_RSHIFT, VK_LWIN, VK_RWIN };
        const InputKey inputKeys[4]{ InputKey::LeftShift, InputKey::RightShift, InputKey::LeftSuper, InputKey::RightSuper };

        for (auto i = 0u; i < 4u; ++i)
        {
            const auto vk = vks[i];
            const auto inputKey = inputKeys[i];

            if (!(GetKeyState(vk) & 0x8000) && cached_keystates[(uint32_t)inputKey])
            {
                DispatchKeyEvent(inputKey, false);
            }
        }
    }

    void Win32Window::OnWaitEvents()
    {

    }


    void Win32Window::ValidateResolution()
    {
        if (state.isMinimized)
        {
            return;
        }

        RECT rect;
        ::GetClientRect(handle, &rect);
        auto width = glm::max(rect.right - rect.left, 0L);
        auto height = glm::max(rect.bottom - rect.top, 0L);

        // Check for windows maximized size and see if it needs to adjust position if needed
        if (state.isMaximized)
        {
            // Pick the current monitor data for sizing
            const HMONITOR monitor = MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFO);
            ::GetMonitorInfoW(monitor, &monitorInfo);

            auto cwidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
            auto cheight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
            if (width > cwidth && height > cheight)
            {
                width = cwidth;
                height = cheight;
                ::SetWindowPos(handle, HWND_TOP, monitorInfo.rcWork.left, monitorInfo.rcWork.top, width, height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            }
        }

        auto hasChanged = cached_clientsize[0] != width || cached_clientsize[1] != height;
        cached_clientsize[0] = width;
        cached_clientsize[1] = height;

        if (width > 0 && height > 0 && hasChanged)
        {
            DispatchWindowEvent(PlatformWindowEvent::Resize);
        }
    }

    void Win32Window::BeginLockingCursor()
    {
        RECT clipRect;
        ::GetClientRect(handle, &clipRect);
        ::ClientToScreen(handle, (POINT*)&clipRect.left);
        ::ClientToScreen(handle, (POINT*)&clipRect.right);
        ::ClipCursor(&clipRect);
        Win32WindowManager::Get()->lockedCursorWindow = this;
    }

    void Win32Window::EndLockingCursor()
    {
        ::ClipCursor(NULL);
        Win32WindowManager::Get()->lockedCursorWindow = nullptr;
    }

    void Win32Window::UpdateCursor()
    {
        auto manager = Win32WindowManager::Get();

        auto isLocked = options.cursorLock & GetIsFocused();
        auto isVisible = !(options.cursorHide && GetIsFocused());
        auto isDisabled = isLocked && !isVisible;
        auto isRawMotion = isDisabled && GetIsFocused() && options.useRawMouseInput;

        if (isRawMotion != state.isUsingRawMotion)
        {
            const RAWINPUTDEVICE rid_enable = { 0x01, 0x02, 0, handle };
            const RAWINPUTDEVICE rid_disable = { 0x01, 0x02, RIDEV_REMOVE, NULL };
            const auto rid = isRawMotion ? rid_enable : rid_disable;
            ::RegisterRawInputDevices(&rid, 1, sizeof(rid));
            state.isUsingRawMotion = isRawMotion;
        }

        ::SetCursor(isVisible ? LoadCursorW(NULL, IDC_ARROW) : NULL);

        if (isDisabled && manager->disabledCursorWindow != this)
        {
            manager->restore_cursorpos = GetCursorPosition();
            manager->disabledCursorWindow = this;
        }

        if (!isDisabled && manager->disabledCursorWindow == this)
        {
            SetCursorPosition(manager->restore_cursorpos);
            manager->disabledCursorWindow = nullptr;
        }

        if (isLocked && manager->lockedCursorWindow != this)
        {
            SetCursorPosToCenter();
            BeginLockingCursor();
        }

        if (!isLocked && manager->lockedCursorWindow == this)
        {
            EndLockingCursor();
        }
    }

    void Win32Window::CursorMovedEvent(const int2& position)
    {
        if (virt_cursorpos[0] != position.x || virt_cursorpos[1] != position.y)
        {
            virt_cursorpos[0] = position.x;
            virt_cursorpos[1] = position.y;
            DispatchMouseMoveEvent(position);
        }
    }

    void Win32Window::UpdateRawMouseInput(LPARAM lParam)
    {
        auto manager = Win32WindowManager::Get();

        if (manager->disabledCursorWindow != this || !state.isUsingRawMotion)
        {
            return;
        }

        auto& pRawInput = manager->rawInput;
        auto& pRawInputSize = manager->rawInputSize;

        UINT size = 0u;
        ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

        if (size > pRawInputSize)
        {
            if (pRawInput)
            {
                free(pRawInput);
            }
            
            pRawInput = (RAWINPUT*)calloc(size, 1);
            pRawInputSize = size;
        }

        size = pRawInputSize;

        if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
        {
            return;
        }

        int32_t dx = 0;
        int32_t dy = 0;

        if (pRawInput->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            POINT pos = { 0 };
            int width, height;

            if (pRawInput->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)
            {
                pos.x += ::GetSystemMetrics(SM_XVIRTUALSCREEN);
                pos.y += ::GetSystemMetrics(SM_YVIRTUALSCREEN);
                width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
                height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
            }
            else
            {
                width = ::GetSystemMetrics(SM_CXSCREEN);
                height = ::GetSystemMetrics(SM_CYSCREEN);
            }

            pos.x += (int32_t)((pRawInput->data.mouse.lLastX / 65535.f) * width);
            pos.y += (int32_t)((pRawInput->data.mouse.lLastY / 65535.f) * height);
            ::ScreenToClient(handle, &pos);
            dx = pos.x - cached_cursorpos[0];
            dy = pos.y - cached_cursorpos[1];
        }
        else
        {
            dx = pRawInput->data.mouse.lLastX;
            dy = pRawInput->data.mouse.lLastY;
        }

        CursorMovedEvent({ virt_cursorpos[0] + dx, virt_cursorpos[1] + dy });
        cached_cursorpos[0] += dx;
        cached_cursorpos[1] += dy;
    }


    void Win32Window::DispatchWindowEvent(PlatformWindowEvent evt)
    {
        for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
        {
            m_windowListeners[i]->IPlatformWindow_OnEvent(this, evt);
        }
    }

    void Win32Window::DispatchKeyEvent(InputKey key, bool isDown)
    {
        if (isDown != cached_keystates[(uint32_t)key])
        {
            cached_keystates[(uint32_t)key] = isDown;
            
            for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
            {
                m_inputListeners[i]->IPlatformWindowInput_OnKey(this, key, isDown);
            }
        }
    }

    void Win32Window::DispatchMouseMoveEvent(const int2& position)
    {
        for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
        {
            m_inputListeners[i]->IPlatformWindowInput_OnMouseMove(this, position);
        }
    }

    void Win32Window::DispatchScrollEvent(const float2& offset)
    {
        for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
        {
            m_inputListeners[i]->IPlatformWindowInput_OnScroll(this, offset);
        }
    }

    void Win32Window::DispatchCharacterEvent(uint32_t character)
    {
        // @TODO what is this
        if (character < 32 || (character > 126 && character < 160))
        {
            return;
        }

        for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
        {
            m_inputListeners[i]->IPlatformWindowInput_OnCharacter(this, character);
        }
    }

    void Win32Window::DispatchDrop(const char* const* paths, uint32_t count)
    {
        for (auto i = 0u; i < m_windowListeners.GetCount(); ++i)
        {
            m_inputListeners[i]->IPlatformWindowInput_OnDrop(this, paths, count);
        }
    }

    void Win32Window::OnFocusChanged(bool value)
    {
        if (state.isFocused == value)
        {
            return;
        }

        state.isFocused = value;

        DispatchWindowEvent(value ? PlatformWindowEvent::Focus : PlatformWindowEvent::Unfocus);
        
        if (!value)
        {
            for (auto i = 0u; i < (uint32_t)InputKey::Count; ++i)
            {
                if (cached_keystates[i])
                {
                    DispatchKeyEvent((InputKey)i, false);
                }
            }
        }
    }

    void Win32Window::OnClose()
    {
        state.isClosing = true;
        DispatchWindowEvent(PlatformWindowEvent::Close);
    }

    bool Win32Window::IsAnyMouseKeyDown() const
    {
        for (auto i = (uint32_t)InputKey::Mouse1; i <= (uint32_t)InputKey::Mouse8; ++i)
        {
            if (cached_keystates[i])
            {
                return true;
            }
        }

        return false;
    }


    LRESULT Win32Window::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto manager = Win32WindowManager::Get();

        switch (uMsg)
        {
            case WM_MOUSEACTIVATE:
            {
                if (HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) != HTCLIENT)
                {
                    scope.inFrameAction = true;
                }
                break;
            }
            case WM_CAPTURECHANGED:
            {
                if (lParam == 0 && scope.inFrameAction)
                {
                    UpdateCursor();
                    scope.inFrameAction = false;
                }
                break;
            }
            case WM_SETCURSOR:
            {
                if (LOWORD(lParam) == HTCLIENT)
                {
                    UpdateCursor();
                    return true;
                }
                break;
            }
            case WM_MOUSEMOVE:
            {
                const auto x = GET_X_LPARAM(lParam);
                const auto y = GET_Y_LPARAM(lParam);

                if (!state.isCursorTracked)
                {
                    TRACKMOUSEEVENT tme;
                    ZeroMemory(&tme, sizeof(tme));
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = handle;
                    TrackMouseEvent(&tme);
                    state.isCursorTracked = true;
                    DispatchWindowEvent(PlatformWindowEvent::CursorEnter);
                }

                if (options.cursorLock && options.cursorHide)
                {
                    if (manager->disabledCursorWindow != this || state.isUsingRawMotion)
                    {
                        break;
                    }

                    const auto dx = x - cached_cursorpos[0];
                    const auto dy = y - cached_cursorpos[1];
                    CursorMovedEvent({ virt_cursorpos[0] + dx, virt_cursorpos[1] + dy });
                }
                else
                {
                    CursorMovedEvent({ x, y });
                }

                cached_cursorpos[0] = x;
                cached_cursorpos[1] = x;
                return 0;
            }
            case WM_MOUSELEAVE:
            {
                state.isCursorTracked = false;
                DispatchWindowEvent(PlatformWindowEvent::CursorExit);
                return 0;
            }

            case WM_GETMINMAXINFO:
            {
                RECT borderRect = { 0 };
                const DWORD style = GetWindowLongW(handle, GWL_STYLE);
                const DWORD styleEx = GetWindowLongW(handle, GWL_EXSTYLE);
                ::AdjustWindowRectEx(&borderRect, style, FALSE, styleEx);

                auto borderWidth = options.isBorderless ? 0 : borderRect.right - borderRect.left;
                auto borderHeight = options.isBorderless ? 0 : borderRect.bottom - borderRect.top;

                const auto minmax = reinterpret_cast<MINMAXINFO*>(lParam);
                if (size_min[0] != -1) minmax->ptMinTrackSize.x = size_min[0] + borderWidth;
                if (size_min[1] != -1) minmax->ptMinTrackSize.x = size_min[1] + borderHeight;
                if (size_min[0] != -1) minmax->ptMaxTrackSize.x = size_max[0] + borderWidth;
                if (size_min[1] != -1) minmax->ptMaxTrackSize.x = size_max[1] + borderHeight;

                // Include Windows task bar size into maximized tool window
                WINDOWPLACEMENT wmp;

                if (!state.isFullScreen && ((::GetWindowPlacement(handle, &wmp) && (wmp.showCmd == SW_SHOWMAXIMIZED || wmp.showCmd == SW_SHOWMINIMIZED)) || scope.inMaximize))
                {
                    // Adjust the maximized size and position to fit the work area of the correct monitor
                    const HMONITOR monitor = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);

                    if (monitor != nullptr)
                    {
                        MONITORINFO monitorInfo;
                        monitorInfo.cbSize = sizeof(MONITORINFO);
                        if (::GetMonitorInfoW(monitor, &monitorInfo))
                        {
                            minmax->ptMaxPosition.x = glm::abs(monitorInfo.rcWork.left - monitorInfo.rcMonitor.left);
                            minmax->ptMaxPosition.y = glm::abs(monitorInfo.rcWork.top - monitorInfo.rcMonitor.top);
                            minmax->ptMaxSize.x = glm::abs(monitorInfo.rcWork.right - monitorInfo.rcWork.left);
                            minmax->ptMaxSize.y = glm::abs(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
                        }
                    }
                }

                return 0;
            }
            case WM_ENTERMENULOOP:
            case WM_ENTERSIZEMOVE:
            {
                scope.inResize = true;
                break;
            }
            case WM_EXITMENULOOP:
            case WM_EXITSIZEMOVE:
            {
                scope.inResize = false;
                ValidateResolution();
                if (!scope.inFrameAction)
                {
                    UpdateCursor();
                }
                break;
            }
            case WM_SIZE:
            {
                RECT rcCurrentClient;
                GetClientRect(handle, &rcCurrentClient);
                const auto isMinimize = SIZE_MINIMIZED == wParam;
                const auto isValid = rcCurrentClient.top != 0 && rcCurrentClient.bottom != 0 && !isMinimize;
                const auto isMaximize = SIZE_MAXIMIZED == wParam && isValid;
                const auto isRestore = SIZE_RESTORED == wParam && isValid && (state.isMaximized || state.isMinimized || (!scope.inResize && !scope.inFullScreen));

                if (isMinimize)
                {
                    state.isMinimized = true;
                    state.isMaximized = false;
                }

                if (isMaximize)
                {
                    state.isMinimized = false;
                    state.isMaximized = true;
                    ValidateResolution();
                }

                if (isRestore)
                {
                    state.isMaximized = false;
                    state.isMinimized = false;
                    ValidateResolution();
                }
                return 0;
            }
            case WM_DPICHANGED:
            {
                if (!state.isFullScreen && (options.useDpiScaling || PK_PLATFORM_WINDOWS_IS_10_1703_OR_GREATER()))
                {
                    RECT* rect = (RECT*)lParam;
                    SetWindowPos(handle, HWND_TOP, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                break;
            }
            case WM_MOVE:
            {
                UpdateCursor();
                return 0;
            }

            case WM_SETFOCUS:
            {
                OnFocusChanged(true);
                if (!scope.inFrameAction)
                {
                    UpdateCursor();
                }
                break;
            }
            case WM_KILLFOCUS:
            {
                OnFocusChanged(false);
                UpdateCursor();
                break;
            }
            case WM_ACTIVATEAPP:
            {
                if (wParam == TRUE && !GetIsFocused())
                {
                    OnFocusChanged(true);
                }
                else if (wParam == FALSE && GetIsFocused())
                {
                    OnFocusChanged(false);
                    if (state.isFullScreen && !scope.inFullScreen)
                    {
                        SetFullScreen(false);
                    }
                }
                UpdateCursor();
                break;
            }

            case WM_SYSCOMMAND:
            {
                if (state.isFullScreen)
                {
                    switch ((wParam & 0xFFF0))
                    {
                        case SC_MOVE:
                        case SC_SIZE:
                        case SC_MAXIMIZE:
                        case SC_KEYMENU:
                        case SC_SCREENSAVE:
                        case SC_MONITORPOWER:
                            return 0;
                    }
                }
                break;
            }
            case WM_SYSKEYDOWN:
            {
                if (wParam == VK_F4)
                {
                    OnClose();
                    return 0;
                }
                break;
            }
            case WM_CLOSE:
            {
                OnClose();
                return 0;
            }
            case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }
            case WM_ERASEBKGND:
            {
                return TRUE;
            }
            case WM_CREATE:
            {
                return 0;
            }

            case WM_NCACTIVATE:
            case WM_NCPAINT:
            {
                if (!options.isBorderless)
                {
                    return TRUE;
                }
                break;
            }
        }

        // Input events
        switch (uMsg)
        {
            case WM_CHAR:
            case WM_SYSCHAR:
            {
                if (wParam >= 0xd800 && wParam <= 0xdbff)
                {
                    cached_lastHighSurrogate = (WCHAR)wParam;
                }
                else
                {
                    uint32_t codepoint = 0;

                    if (wParam >= 0xdc00 && wParam <= 0xdfff)
                    {
                        if (cached_lastHighSurrogate)
                        {
                            codepoint += (cached_lastHighSurrogate - 0xd800) << 10;
                            codepoint += (WCHAR)wParam - 0xdc00;
                            codepoint += 0x10000;
                        }
                    }
                    else
                    {
                        codepoint = (WCHAR)wParam;
                    }

                    cached_lastHighSurrogate = 0;
                    DispatchCharacterEvent(uMsg != WM_SYSCHAR ? codepoint : 0);
                }
                return 0;
            }
            case WM_UNICHAR:
            {
                if (wParam == UNICODE_NOCHAR)
                {
                    return TRUE;
                }

                DispatchCharacterEvent((uint32_t)wParam);
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const bool isDown = !(HIWORD(lParam) & KF_UP);
                const auto scancode = GetRemappedScanCode(wParam, lParam);
                auto key = manager->native_to_keycode[scancode];

                if (!HandleModifierKeys(wParam, lParam, &key))
                {
                    break;
                }

                if (!isDown && wParam == VK_SHIFT)
                {
                    // HACK: Release both Shift keys on Shift up event, as when both
                    //       are pressed the first release does not emit any event
                    // NOTE: The other half of this is in _glfwPollEventsWin32
                    DispatchKeyEvent(InputKey::LeftShift, false);
                    DispatchKeyEvent(InputKey::RightShift, false);
                    break;
                }

                else if (wParam == VK_SNAPSHOT)
                {
                    // HACK: Key down is not reported for the Print Screen key
                    DispatchKeyEvent(key, true);
                    DispatchKeyEvent(key, false);
                    break;
                }
                
                DispatchKeyEvent(key, isDown);
                break;
            }

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                const auto isDown = uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN;
                const auto key = GetInputKeyFromMouseButton(uMsg, wParam);

                if (!IsAnyMouseKeyDown())
                {
                    ::SetCapture(handle);
                }

                DispatchKeyEvent(key, isDown);

                if (!IsAnyMouseKeyDown())
                {
                    ::ReleaseCapture();
                }

                return uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP ? TRUE : FALSE;
            }

            case WM_INPUT:
            {
                UpdateRawMouseInput(lParam);
                break;
            }
            case WM_MOUSEWHEEL:
            {
                DispatchScrollEvent({ 0.0f, (SHORT)HIWORD(wParam) / (float)WHEEL_DELTA });
                return 0;
            }
            case WM_MOUSEHWHEEL:
            {
                DispatchScrollEvent({ -(SHORT)HIWORD(wParam) / (float)WHEEL_DELTA, 0.0f });
                return 0;
            }

            case WM_DROPFILES:
            {
                const auto count = ::DragQueryFileW((HDROP)wParam, 0xffffffff, NULL, 0);
                auto paths = (char**)calloc(count, sizeof(char*));

                // Move the mouse to the position of the drop
                POINT point;
                ::DragQueryPoint((HDROP)wParam, &point);
                CursorMovedEvent({ point.x, point.y });

                for (auto i = 0u; i < count; ++i)
                {
                    const auto length = ::DragQueryFileW((HDROP)wParam, i, NULL, 0);
                    auto src = (WCHAR*)calloc(length + 1u, sizeof(WCHAR));
                    auto dst = (char*)calloc(length + 1u, sizeof(char));

                    DragQueryFileW((HDROP)wParam, i, src, length + 1);
                    wcstombs(dst, src, length);
                    free(src);
                }

                DispatchDrop(paths, count);

                for (auto i = 0u; i < count; ++i)
                {
                    free(paths[i]);
                }

                free(paths);

                ::DragFinish((HDROP)wParam);
                return 0;
            }
        }

        return DefWindowProc(handle, uMsg, wParam, lParam);
    }
}

#endif
