#include "PrecompiledHeader.h"
#if PK_PLATFORM_WINDOWS
#include "Win32Driver.h"
#include "Win32Window.h"
#include "Windowsx.h"

namespace PK
{
    static void GetHMONITORContentScale(HMONITOR handle, float* xscale, float* yscale)
    {
        uint32_t dpix = 0u;
        uint32_t dpiy = 0u;
        *xscale = 0.0f;
        *yscale = 0.0f;

        if (PK_PLATFORM_WINDOWS_IS_8_1_OR_GREATER())
        {
            if (PK_GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &dpix, &dpix) != S_OK)
            {
                return;
            }
        }
        else
        {
            const HDC dc = ::GetDC(NULL);
            dpix = ::GetDeviceCaps(dc, LOGPIXELSX);
            dpiy = ::GetDeviceCaps(dc, LOGPIXELSY);
            ::ReleaseDC(NULL, dc);
        }

        *xscale = dpix / (float)USER_DEFAULT_SCREEN_DPI;
        *yscale = dpiy / (float)USER_DEFAULT_SCREEN_DPI;
    }

    static void AdjustWindowRectExDpiAware(HWND handle, _Inout_ LPRECT lpRect, _In_ DWORD dwStyle, _In_ BOOL bMenu, _In_ DWORD dwExStyle)
    {
        if (PK_PLATFORM_WINDOWS_IS_10_1607_OR_GREATER())
        {
            PK_AdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, PK_GetDpiForWindow(handle));
        }
        else
        {
            ::AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
        }
    }


    static int32_t GetRemappedScanCode(WPARAM wParam, LPARAM lParam)
    {
        int32_t scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));

        if (!scancode)
        {
            scancode = ::MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
        }

        // HACK: Alt+PrtSc has a different scancode than just PrtSc
        if (scancode == 0x54)
        {
            scancode = 0x137;
        }

        // HACK: Ctrl+Pause has a different scancode than just Pause
        if (scancode == 0x146)
        {
            scancode = 0x45;
        }

        // HACK: CJK IME sets the extended bit for right Shift
        if (scancode == 0x136)
        {
            scancode = 0x36;
        }

        return scancode;
    }

    static bool HandleModifierKeys(WPARAM wParam, LPARAM lParam, InputKey* key)
    {
        if (wParam != VK_CONTROL)
        {
            return wParam != VK_PROCESSKEY;
        }

        if (HIWORD(lParam) & KF_EXTENDED)
        {
            *key = InputKey::RightControl;
            return true;
        }

        const auto time = (DWORD)::GetMessageTime();
        
        MSG next;
        auto discard = ::PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE);
        discard &= (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP || next.message == WM_SYSKEYUP);
        discard &= (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) && next.time == time);

        if (discard)
        {
            return false;
        }

        *key = InputKey::LeftControl;

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
        m_isPendingActivate = descriptor.autoActivate;
        m_useDpiScaling = descriptor.useDpiScaling;
        m_isResizable = descriptor.isResizable;
        m_sizeMin = descriptor.sizemin;
        m_sizeMax = descriptor.sizemax;

        DWORD style = WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION;
        DWORD styleEx = WS_EX_APPWINDOW;

        if (descriptor.isFloating)
        {
            styleEx |= WS_EX_TOPMOST;
        }

        if (descriptor.isResizable)
        {
            style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        }

        auto frameRect = PK_INT4_ZERO;
        {
            RECT rect = { 0, 0, descriptor.size.x, descriptor.size.y };
            ::AdjustWindowRectEx(&rect, style, FALSE, styleEx);
            frameRect.x = descriptor.position.x == -1 ? CW_USEDEFAULT : (descriptor.position.x + rect.left);
            frameRect.y = descriptor.position.y == -1 ? CW_USEDEFAULT : (descriptor.position.y + rect.top);
            frameRect.z = rect.right - rect.left;
            frameRect.w = rect.bottom - rect.top;
        }

        auto wideTitle = Parse::ToWideString(descriptor.title, strnlen(descriptor.title, 0xFFu));

        m_handle = ::CreateWindowExW
        (
            styleEx,
            Win32Window::CLASS_MAIN,
            wideTitle.c_str(),
            style,
            frameRect.x,
            frameRect.y,
            frameRect.z,
            frameRect.w,
            nullptr,
            nullptr,
            (HINSTANCE)Platform::GetProcess(), 
            &m_useDpiScaling
        );

        if (!m_handle)
        {
            throw std::runtime_error("Failed to create a window through: CreateWindowExW.");
        }

        // Set default icon from rc.
        if (descriptor.useEmbeddedIcon)
        {
            HINSTANCE hInstance = (HINSTANCE)Win32Driver::Get()->GetProcess();
            HICON hIcon = ::LoadIconW(hInstance, PK_WIN32_EMBEDDED_ICON_NAME);
            ::SendMessageW(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            ::SendMessageW(m_handle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            m_icon = hIcon;
        }

        // Set dark title bar
        {
            if (PK_DwmSetWindowAttribute)
            {
                BOOL value = true;
                PK_DwmSetWindowAttribute(m_handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
            }
        }

        ::SetPropW(m_handle, Win32Window::WINDOW_PROP, this);

        PK_ChangeWindowMessageFilterEx(m_handle, WM_DROPFILES, MSGFLT_ALLOW, NULL);
        PK_ChangeWindowMessageFilterEx(m_handle, WM_COPYDATA, MSGFLT_ALLOW, NULL);
        PK_ChangeWindowMessageFilterEx(m_handle, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);

        {
            RECT rect = { 0, 0, descriptor.size.x, descriptor.size.y };
            WINDOWPLACEMENT wp = { sizeof(wp) };

            const HMONITOR mh = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);

            if (descriptor.useDpiScaling)
            {
                float xscale, yscale;
                GetHMONITORContentScale(mh, &xscale, &yscale);
                rect.right = xscale > 0.0f ? (int)(rect.right * xscale) : rect.right;
                rect.bottom = yscale > 0.0f ? (int)(rect.bottom * yscale) : rect.bottom;
            }

            AdjustWindowRectExDpiAware(m_handle, &rect, style, FALSE, styleEx);
            ::GetWindowPlacement(m_handle, &wp);
            ::OffsetRect(&rect, wp.rcNormalPosition.left - rect.left, wp.rcNormalPosition.top - rect.top);

            wp.rcNormalPosition = rect;
            wp.showCmd = SW_HIDE;
            ::SetWindowPlacement(m_handle, &wp);
        }

        ::DragAcceptFiles(m_handle, TRUE);

        {
            RECT rect;
            ::GetClientRect(m_handle, &rect);
            m_clientsize[0] = rect.right;
            m_clientsize[1] = rect.bottom;
        }

        if (descriptor.isVisible)
        {
            SetVisible(true);
            Focus();
        }
    }

    Win32Window::~Win32Window()
    {
        m_isVisible = false;
        OnFocusChanged(false);
        UpdateCursor();

        if (m_handle)
        {
            ::RemovePropW(m_handle, Win32Window::WINDOW_PROP);
            ::DestroyWindow(m_handle);
            m_handle = nullptr;
        }

        if (m_icon)
        {
            ::DestroyIcon(m_icon);
        }
    }


    int4 Win32Window::GetRect() const
    {
        POINT pos = { 0, 0 };
        ::ClientToScreen(m_handle, &pos);
        return { pos.x, pos.y, m_clientsize };
    }

    int2 Win32Window::GetMonitorResolution() const
    {
        auto monitor = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        ::GetMonitorInfoW(monitor, &mi);
        return { mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top };
    }

    float2 Win32Window::GetCursorPosition() const
    {
        POINT pos{ 0, 0 };
        ::GetCursorPos(&pos);
        ::ScreenToClient(m_handle, &pos);
        return { (float)pos.x, (float)pos.y };
    }

    
    float Win32Window::GetInputAnalogAxis(InputKey neg, InputKey pos) 
    {
        if (neg == InputKey::MouseScrollDown && pos == InputKey::MouseScrollUp)
        {
            return m_scroll[0].y;
        }

        if (neg == InputKey::MouseScrollLeft && pos == InputKey::MouseScrollRight)
        {
            return m_scroll[0].x;
        }

        return 0.0f;
    }

    void Win32Window::SetUseRawInput(bool value)
    {
        m_useRawInput = value;
        UpdateCursor();
    }


    void Win32Window::SetRect(const int4& newRect)
    {
        SetFullScreen(false);

        const auto currentRect = GetRect();
        const bool changePosition = currentRect.x != newRect.x || currentRect.y != newRect.y;
        const bool changeSize = currentRect.z != newRect.z || currentRect.w != newRect.w;

        if (changePosition || changeSize)
        {
            auto x = newRect.x;
            auto y = newRect.y;
            auto width = newRect.z;
            auto height = newRect.w;

            ::SetWindowPos(m_handle, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

            if (changeSize)
            {
                RECT rect;
                ::GetClientRect(m_handle, &rect);
                m_clientsize[0] = rect.right;
                m_clientsize[1] = rect.bottom;
                DispatchWindowOnEvent(PlatformWindowEvent::Resize);
            }
        }
    }

    void Win32Window::SetCursorPosition(const float2& position)
    {
        if (glm::any(glm::epsilonNotEqual(m_cursorpos, position, 1e-2f)))
        {
            m_cursorpos = position;
            POINT pos = { (int32_t)position.x, (int32_t)position.y };
            ::ClientToScreen(m_handle, &pos);
            ::SetCursorPos(pos.x, pos.y);
        }
    }

    void Win32Window::SetCursorLock(bool lock, bool visible)
    {
        if (m_cursorLock != lock || m_cursorHide != visible)
        {
            m_cursorLock = lock;
            m_cursorHide = !visible;
            UpdateCursor();
        }
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
            iconBig = (HICON)::GetClassLongPtrW(m_handle, GCLP_HICON);
            iconSmall = (HICON)::GetClassLongPtrW(m_handle, GCLP_HICONSM);
        }

        ::SendMessageW(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
        ::SendMessageW(m_handle, WM_SETICON, ICON_BIG, (LPARAM)iconBig);

        if (m_icon)
        {
            ::DestroyIcon(m_icon);
        }

        if (pixels)
        {
            m_icon = iconSmall;
        }
    }


    void Win32Window::SetVisible(bool value)
    {
        if (value != m_isVisible)
        {
            if (value)
            {
                auto showCommand = SW_SHOWNA;

                if (m_isPendingActivate)
                {
                    STARTUPINFOW si = { sizeof(si) };
                    ::GetStartupInfoW(&si);
                    if (si.dwFlags & STARTF_USESHOWWINDOW)
                    {
                        showCommand = si.wShowWindow;
                    }
                }

                ::ShowWindow(m_handle, showCommand);
                m_isVisible = true;
                m_isPendingActivate = false;
                DispatchWindowOnEvent(PlatformWindowEvent::Visible);
            }
            else
            {
                ::ShowWindow(m_handle, SW_HIDE);
                m_isVisible = false;
                OnFocusChanged(false);
                UpdateCursor();
                DispatchWindowOnEvent(PlatformWindowEvent::Invisible);
            }
        }
    }

    void Win32Window::SetFullScreen(bool value)
    {
        if (m_isFullScreen == value)
        {
            return;
        }

        if (m_isFullScreen && !value)
        {
            DispatchWindowOnEvent(PlatformWindowEvent::FullScreenExit);
        }

        if (m_isMaximized && !value)
        {
            Restore();
        }

        m_isFullScreen = value;

        if (::IsIconic(m_handle))
        {
            ::ShowWindow(m_handle, SW_RESTORE);
        }
        else
        {
            ::SetActiveWindow(m_handle);
        }

        if (!value)
        {
            auto style = ::GetWindowLongW(m_handle, GWL_STYLE);
            auto styleEx = ::GetWindowLongW(m_handle, GWL_EXSTYLE);
            style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
            style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

            if (m_isResizable)
            {
                style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
            }

            ::SetWindowLongW(m_handle, GWL_STYLE, style);

            auto restoreRect = m_restoreRect;
            auto desktopSize = Platform::GetDesktopSize();
            
            RECT borderRect = { 0 };
            AdjustWindowRectExDpiAware(m_handle, &borderRect, style, FALSE, styleEx);

            // Sanity check restore rect
            restoreRect.x = glm::min(restoreRect.x, desktopSize.x - restoreRect.z);
            restoreRect.y = glm::min(restoreRect.y, desktopSize.y - restoreRect.y);
            restoreRect.x = glm::max(restoreRect.x, 0);
            restoreRect.y = glm::max(restoreRect.y, 0);
            restoreRect.z = glm::min(restoreRect.z, desktopSize.x - restoreRect.x);
            restoreRect.w = glm::min(restoreRect.w, desktopSize.y - restoreRect.y);
            if (m_sizeMin.x >= 0) restoreRect.z = glm::max(restoreRect.z, m_sizeMin.x + (int32_t)(borderRect.right - borderRect.left));
            if (m_sizeMin.y >= 0) restoreRect.w = glm::max(restoreRect.w, m_sizeMin.y + (int32_t)(borderRect.bottom - borderRect.top));

            ::SetWindowPos(m_handle, nullptr, restoreRect.x, restoreRect.y, restoreRect.z, restoreRect.w, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
            ::ShowWindow(m_handle, SW_SHOW);
        }
        else
        {
            m_isAcquiringFullScreen = true;

            RECT rect;
            ::GetWindowRect(m_handle, &rect);
            m_restoreRect.x = rect.left;
            m_restoreRect.y = rect.top;
            m_restoreRect.z = rect.right - rect.left;
            m_restoreRect.w = rect.bottom - rect.top;

            auto style = ::GetWindowLongW(m_handle, GWL_STYLE);
            style &= ~(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
            style |= WS_POPUP;
            ::SetWindowLongW(m_handle, GWL_STYLE, style);

            const HMONITOR monitor = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            ::GetMonitorInfoW(monitor, &mi);

            ::SetWindowPos
            (
                m_handle, 
                HWND_NOTOPMOST,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE
            );

            ::ShowWindow(m_handle, SW_MAXIMIZE);

            auto acquired = DispatchWindowOnEvent(PlatformWindowEvent::FullScreenRequest);

            if (!acquired)
            {
                ::ShowWindow(m_handle, SW_NORMAL);
                m_isFullScreen = false;
            }

            m_isAcquiringFullScreen = false;
        }
    }

    void Win32Window::Minimize()
    {
        ::ShowWindow(m_handle, SW_MINIMIZE);
    }

    void Win32Window::Maximize()
    {
        m_isMaximizing = true;
        ::ShowWindow(m_handle, SW_MAXIMIZE);
        m_isMaximizing = false;
    }

    void Win32Window::Restore()
    { 
        ::ShowWindow(m_handle, SW_RESTORE);
    }

    void Win32Window::Focus()
    {
        ::BringWindowToTop(m_handle);
        ::SetForegroundWindow(m_handle);
        ::SetFocus(m_handle);
    }

    void Win32Window::Close()
    {
        if (!m_isClosing)
        {
            m_isClosing = true;
            DispatchWindowOnEvent(PlatformWindowEvent::Close);
        }
    }


    void Win32Window::OnPollEvents()
    {
        const int32_t vks[4]{ VK_LSHIFT, VK_RSHIFT, VK_LWIN, VK_RWIN };
        const InputKey inputKeys[4]{ InputKey::LeftShift, InputKey::RightShift, InputKey::LeftSuper, InputKey::RightSuper };

        for (auto i = 0u; i < 4u; ++i)
        {
            const auto vk = vks[i];
            const auto inputKey = inputKeys[i];

            if (!(::GetKeyState(vk) & 0x8000) && m_keyState[(uint32_t)inputKey])
            {
                DispatchInputOnKey(inputKey, false);
            }
        }

        for (auto axis = 0u; axis < 2u; ++axis)
        {
            auto hasValue0 = glm::abs(m_scroll[0][axis]) > 1e-4f;
            auto hasValue1 = glm::abs(m_scroll[1][axis]) > 1e-4f;

            if (hasValue1 && !hasValue0)
            {
                DispatchInputOnScroll(axis, 0.0f);
            }
        }

        m_scroll[1] = m_scroll[0];
        m_scroll[0] = PK_FLOAT2_ZERO;
    }


    void Win32Window::ValidateResolution()
    {
        RECT rect;
        ::GetClientRect(m_handle, &rect);
        auto width = glm::max(rect.right - rect.left, 0L);
        auto height = glm::max(rect.bottom - rect.top, 0L);

        // Check for windows maximized size and see if it needs to adjust position if needed
        if (m_isMaximized)
        {
            // Pick the current monitor data for sizing
            const HMONITOR monitor = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFO);
            ::GetMonitorInfoW(monitor, &monitorInfo);

            auto cwidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
            auto cheight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
            if (width > cwidth && height > cheight)
            {
                width = cwidth;
                height = cheight;
                ::SetWindowPos
                (
                    m_handle, 
                    HWND_TOP, 
                    monitorInfo.rcWork.left, 
                    monitorInfo.rcWork.top, 
                    width, 
                    height, 
                    SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER
                );
            }
        }

        auto hasChanged = m_clientsize.x != width || m_clientsize.y != height;
        m_clientsize.x = width;
        m_clientsize.y = height;

        if (hasChanged)
        {
            DispatchWindowOnEvent(PlatformWindowEvent::Resize);
        }
    }

    void Win32Window::UpdateCursor()
    {
        auto isLocked = m_cursorLock & IsFocused();
        auto isVisible = !(m_cursorHide && IsFocused());
        auto isDisabled = isLocked && !isVisible;
        auto isRawInput = isDisabled && IsFocused() && m_useRawInput;

        if (isRawInput != m_isUsingRawInput)
        {
            const RAWINPUTDEVICE rid_enable = { 0x01, 0x02, 0, m_handle };
            const RAWINPUTDEVICE rid_disable = { 0x01, 0x02, RIDEV_REMOVE, NULL };
            const auto rid = isRawInput ? rid_enable : rid_disable;
            ::RegisterRawInputDevices(&rid, 1, sizeof(rid));
            m_isUsingRawInput = isRawInput;
        }

        ::SetCursor(isVisible ? ::LoadCursorW(NULL, IDC_ARROW) : NULL);
        Win32Driver::SetDisabledCursorWindow(this, isDisabled, GetCursorPosition());
        Win32Driver::SetLockedCursorWindow(this, isLocked);
    }

    void Win32Window::UpdateRawInput(LPARAM lParam)
    {
        if (m_isUsingRawInput)
        {
            auto rawInput = Win32Driver::GetRawInput(this, lParam);

            if (rawInput != nullptr)
            {
                int32_t dx = 0;
                int32_t dy = 0;

                if (rawInput->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
                {
                    POINT pos = { 0 };
                    int width, height;

                    if (rawInput->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)
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

                    pos.x += (int32_t)((rawInput->data.mouse.lLastX / 65535.f) * width);
                    pos.y += (int32_t)((rawInput->data.mouse.lLastY / 65535.f) * height);
                    ::ScreenToClient(m_handle, &pos);
                    dx = pos.x - m_cursorpos.x;
                    dy = pos.y - m_cursorpos.y;
                }
                else
                {
                    dx = rawInput->data.mouse.lLastX;
                    dy = rawInput->data.mouse.lLastY;
                }

                DispatchInputOnMouseMoved({ m_cursorposVirtual.x + dx, m_cursorposVirtual.y + dy });
                m_cursorpos.x += dx;
                m_cursorpos.y += dy;
            }
        }
    }


    bool Win32Window::DispatchWindowOnEvent(PlatformWindowEvent evt)
    {
        return m_windowListener ? m_windowListener->IPlatformWindow_OnEvent(this, evt) : false;
    }

    void Win32Window::DispatchInputOnKey(InputKey key, bool isDown)
    {
        if (isDown != m_keyState[(uint32_t)key])
        {
            m_keyState[(uint32_t)key] = isDown;
            Win32Driver::DispatchInputOnKey(this, key, isDown);
        }
    }

    void Win32Window::DispatchInputOnMouseMoved(const float2& position)
    {
        if (glm::any(glm::epsilonNotEqual(m_cursorposVirtual, position, 1e-6f)))
        {
            m_cursorposVirtual = position;
            Win32Driver::DispatchInputOnMouseMoved(this, position, m_clientsize);
        }
    }

    void Win32Window::DispatchInputOnScroll(uint32_t axis, float offset)
    {
        InputKey axiskeys[2][2]
        {
            { InputKey::MouseScrollLeft, InputKey::MouseScrollRight },
            { InputKey::MouseScrollDown, InputKey::MouseScrollUp },
        };

        m_scroll[0][axis] = offset;
        DispatchInputOnKey(axiskeys[axis][0], offset < -0.5f);
        DispatchInputOnKey(axiskeys[axis][1], offset > +0.5f);
        Win32Driver::DispatchInputOnScroll(this, axis, offset);
    }

    void Win32Window::DispatchInputOnCharacter(uint32_t character)
    {
        Win32Driver::DispatchInputOnCharacter(this, character);
    }

    void Win32Window::DispatchInputOnDrop(WPARAM wParam)
    {
        const auto count = ::DragQueryFileW((HDROP)wParam, 0xffffffff, NULL, 0);
        auto paths = (char**)calloc(count, sizeof(char*));

        // Move the mouse to the position of the drop
        POINT point;
        ::DragQueryPoint((HDROP)wParam, &point);
        DispatchInputOnMouseMoved({ point.x, point.y });

        for (auto i = 0u; i < count; ++i)
        {
            const auto length = ::DragQueryFileW((HDROP)wParam, i, NULL, 0);
            auto src = (WCHAR*)calloc(length + 1u, sizeof(WCHAR));
            auto dst = (char*)calloc(length + 1u, sizeof(char));

            ::DragQueryFileW((HDROP)wParam, i, src, length + 1);
            wcstombs(dst, src, length);
            free(src);
        }

        Win32Driver::DispatchInputOnDrop(this, paths, count);

        for (auto i = 0u; i < count; ++i)
        {
            free(paths[i]);
        }

        free(paths);

        ::DragFinish((HDROP)wParam);
    }


    void Win32Window::OnFocusChanged(bool value)
    {
        if (m_isFocused != value)
        {
            m_isFocused = value;

            DispatchWindowOnEvent(value ? PlatformWindowEvent::Focus : PlatformWindowEvent::Unfocus);
            
            if (!value)
            {
                for (auto i = 0u; i < (uint32_t)InputKey::Count; ++i)
                {
                    if (m_keyState[i])
                    {
                        DispatchInputOnKey((InputKey)i, false);
                    }
                }
            }
        }
    }

    bool Win32Window::IsAnyMouseKeyDown() const
    {
        for (auto i = (uint32_t)InputKey::Mouse1; i <= (uint32_t)InputKey::Mouse8; ++i)
        {
            if (m_keyState[i])
            {
                return true;
            }
        }

        return false;
    }


    LRESULT Win32Window::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_MOUSEACTIVATE:
            {
                if (HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) != HTCLIENT)
                {
                    m_isInFrameAction = true;
                }
                break;
            }
            case WM_CAPTURECHANGED:
            {
                if (lParam == 0 && m_isInFrameAction)
                {
                    UpdateCursor();
                    m_isInFrameAction = false;
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

            case WM_GETMINMAXINFO:
            {
                RECT borderRect = { 0 };
                const auto style = GetWindowLongW(m_handle, GWL_STYLE);
                const auto styleEx = GetWindowLongW(m_handle, GWL_EXSTYLE);
                AdjustWindowRectExDpiAware(m_handle, &borderRect, style, FALSE, styleEx);

                auto borderWidth = borderRect.right - borderRect.left;
                auto borderHeight = borderRect.bottom - borderRect.top;

                const auto minmax = reinterpret_cast<MINMAXINFO*>(lParam);
                if (m_sizeMin.x != -1) minmax->ptMinTrackSize.x = m_sizeMin.x + borderWidth;
                if (m_sizeMin.y != -1) minmax->ptMinTrackSize.y = m_sizeMin.y + borderHeight;
                if (m_sizeMax.x != -1) minmax->ptMaxTrackSize.x = m_sizeMax.x + borderWidth;
                if (m_sizeMax.y != -1) minmax->ptMaxTrackSize.y = m_sizeMax.y + borderHeight;

                // Include Windows task bar size into maximized tool window
                const HMONITOR monitor = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);

                WINDOWPLACEMENT wmp;
                auto adjustMaximize = false;
                adjustMaximize |= ::GetWindowPlacement(m_handle, &wmp) && (wmp.showCmd == SW_SHOWMAXIMIZED || wmp.showCmd == SW_SHOWMINIMIZED);
                adjustMaximize |= m_isMaximizing;
                adjustMaximize &= !m_isFullScreen;
                adjustMaximize &= monitor != nullptr;

                // Adjust the maximized size and position to fit the work area of the correct monitor
                if (adjustMaximize)
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

                return 0;
            }
            case WM_ENTERMENULOOP:
            case WM_ENTERSIZEMOVE:
            {
                m_isResizing = true;
                break;
            }
            case WM_EXITMENULOOP:
            case WM_EXITSIZEMOVE:
            {
                m_isResizing = false;
                ValidateResolution();
                if (!m_isInFrameAction)
                {
                    UpdateCursor();
                }
                break;
            }
            case WM_SIZE:
            {
                RECT rcCurrentClient;
                GetClientRect(m_handle, &rcCurrentClient);
                const auto isMinimize = SIZE_MINIMIZED == wParam;
                const auto isValid = (rcCurrentClient.top != 0 || rcCurrentClient.bottom != 0) && !isMinimize;
                const auto isMaximize = SIZE_MAXIMIZED == wParam && isValid;
                const auto isRestore = SIZE_RESTORED == wParam && isValid && (m_isMaximized || m_isMinimized || (!m_isMaximizing && !m_isAcquiringFullScreen));

                if (isMinimize)
                {
                    m_isMinimized = true;
                    m_isMaximized = false;
                    ValidateResolution();
                }

                if (isMaximize)
                {
                    m_isMinimized = false;
                    m_isMaximized = true;
                    ValidateResolution();
                }

                if (isRestore)
                {
                    m_isMaximized = false;
                    m_isMinimized = false;
                    ValidateResolution();
                }
                return 0;
            }
            case WM_GETDPISCALEDSIZE:
            {
                if (!m_useDpiScaling && PK_PLATFORM_WINDOWS_IS_10_1703_OR_GREATER())
                {
                    RECT src{};
                    RECT dst{};
                    SIZE* size = (SIZE*)lParam;
                    const auto style = GetWindowLongW(m_handle, GWL_STYLE);
                    const auto styleEx = GetWindowLongW(m_handle, GWL_EXSTYLE);
                    PK_AdjustWindowRectExForDpi(&src, style, FALSE, styleEx, PK_GetDpiForWindow(m_handle));
                    PK_AdjustWindowRectExForDpi(&dst, style, FALSE, styleEx, LOWORD(wParam));
                    size->cx += (dst.right - dst.left) - (src.right - src.left);
                    size->cy += (dst.bottom - dst.top) - (src.bottom - src.top);
                    return TRUE;
                }
                break;
            }
            case WM_DPICHANGED:
            {
                if (!m_isFullScreen && !m_isAcquiringFullScreen && (m_useDpiScaling || PK_PLATFORM_WINDOWS_IS_10_1703_OR_GREATER()))
                {
                    RECT* rect = (RECT*)lParam;
                    SetWindowPos(m_handle, HWND_TOP, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
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
                if (!m_isInFrameAction)
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
                if ((wParam == TRUE || wParam == FALSE) && wParam != IsFocused())
                {
                    OnFocusChanged(wParam);
                }
                UpdateCursor();
                break;
            }

            case WM_SYSCOMMAND:
            {
                if (m_isFullScreen)
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
                    Close();
                    return 0;
                }
                break;
            }
            case WM_CLOSE:
            {
                Close();
                return 0;
            }
            case WM_DESTROY:
            {
                ::PostQuitMessage(0);
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
            case WM_PAINT:
            {
                if (m_isFullScreen)
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
                    m_lastHighSurrogate = (WCHAR)wParam;
                }
                else
                {
                    uint32_t codepoint = 0;

                    if (wParam >= 0xdc00 && wParam <= 0xdfff)
                    {
                        if (m_lastHighSurrogate)
                        {
                            codepoint += (m_lastHighSurrogate - 0xd800) << 10;
                            codepoint += (WCHAR)wParam - 0xdc00;
                            codepoint += 0x10000;
                        }
                    }
                    else
                    {
                        codepoint = (WCHAR)wParam;
                    }

                    m_lastHighSurrogate = 0;
                    DispatchInputOnCharacter(uMsg != WM_SYSCHAR ? codepoint : 0);
                }
                return 0;
            }
            case WM_UNICHAR:
            {
                if (wParam == UNICODE_NOCHAR)
                {
                    return TRUE;
                }

                DispatchInputOnCharacter((uint32_t)wParam);
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const bool isDown = !(HIWORD(lParam) & KF_UP);
                const auto scancode = GetRemappedScanCode(wParam, lParam);
                auto key = Win32Driver::NativeToInputKey(scancode);

                if (!HandleModifierKeys(wParam, lParam, &key))
                {
                    break;
                }

                if (!isDown && wParam == VK_SHIFT)
                {
                    DispatchInputOnKey(InputKey::LeftShift, false);
                    DispatchInputOnKey(InputKey::RightShift, false);
                    break;
                }

                else if (wParam == VK_SNAPSHOT)
                {
                    DispatchInputOnKey(key, true);
                    DispatchInputOnKey(key, false);
                    break;
                }
                
                DispatchInputOnKey(key, isDown);
                break;
            }

            case WM_MOUSEMOVE:
            {
                const auto pos = float2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

                if (!m_keyState[(uint32_t)InputKey::MouseHover])
                {
                    TRACKMOUSEEVENT tme;
                    ZeroMemory(&tme, sizeof(tme));
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = m_handle;
                    TrackMouseEvent(&tme);
                    DispatchInputOnKey(InputKey::MouseHover, true);
                }

                if (m_cursorLock && m_cursorHide)
                {
                    if (!Win32Driver::IsDisabledCursorWindow(this) || m_isUsingRawInput)
                    {
                        break;
                    }

                    DispatchInputOnMouseMoved(m_cursorposVirtual + (pos - m_cursorpos));
                }
                else
                {
                    DispatchInputOnMouseMoved(pos);
                }

                m_cursorpos = pos;
                return 0;
            }
            case WM_MOUSELEAVE:
            {
                DispatchInputOnKey(InputKey::MouseHover, false);
                return 0;
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
                    ::SetCapture(m_handle);
                }

                DispatchInputOnKey(key, isDown);

                if (!IsAnyMouseKeyDown())
                {
                    ::ReleaseCapture();
                }

                return uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP ? TRUE : FALSE;
            }

            case WM_INPUT:
            {
                UpdateRawInput(lParam);
                break;
            }
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            {
                DispatchInputOnScroll(uMsg == WM_MOUSEWHEEL ? 1 : 0, GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
                return 0;
            }
            
            case WM_DROPFILES:
            {
                DispatchInputOnDrop(wParam);
                return 0;
            }
        }

        return DefWindowProc(m_handle, uMsg, wParam, lParam);
    }
}

#endif
