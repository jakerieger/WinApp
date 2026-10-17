#include "App.hpp"

#include <objbase.h>
#include <windowsx.h>

#include <algorithm>
#include <system_error>

namespace WinApp {
    namespace {
        constexpr wchar_t kWindowClass[] = L"WinApp.AppWindow";

        // Registered rather than taken from the WM_APP range so it can never collide with the application's own messages.
        const UINT kPostMessage = RegisterWindowMessageW(L"WinApp.Post");

        [[noreturn]] void ThrowLastError(const char* What) {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), What);
        }

        void RegisterWindowClass(HINSTANCE Instance, WNDPROC Procedure) {
            WNDCLASSEXW Class {};
            Class.cbSize        = sizeof(Class);
            Class.lpfnWndProc   = Procedure;
            Class.hInstance     = Instance;
            Class.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
            Class.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
            Class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            Class.lpszClassName = kWindowClass;
            if (!RegisterClassExW(&Class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                ThrowLastError("RegisterClassExW");
            }
        }
    }  // namespace

    App::App(const AppOptions& Options) : _MinSize {Options.MinWidth, Options.MinHeight} {
        // Fails harmlessly if awareness was already set (e.g. by an application manifest).
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        INITCOMMONCONTROLSEX Controls {};
        Controls.dwSize = sizeof(Controls);
        Controls.dwICC  = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
        InitCommonControlsEx(&Controls);

        const HINSTANCE Instance = GetModuleHandleW(nullptr);
        RegisterWindowClass(Instance, WindowProc);

        DWORD Style = WS_OVERLAPPEDWINDOW;
        if (!Options.Resizable) { Style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX); }

        if (!CreateWindowExW(0,
                             kWindowClass,
                             Options.Title.c_str(),
                             Style,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             nullptr,
                             nullptr,
                             Instance,
                             this)) {
            ThrowLastError("CreateWindowExW");
        }

        _Font = Font::System(_Dpi);
        if (Options.Icon) { SetIcon(Options.Icon); }

        // Size the window so the *client area* matches the requested size at the monitor's DPI.
        RECT Frame {0, 0, Scale(Options.Width), Scale(Options.Height)};
        AdjustWindowRectExForDpi(&Frame, Style, FALSE, 0, _Dpi);
        const int FrameWidth  = Frame.right - Frame.left;
        const int FrameHeight = Frame.bottom - Frame.top;

        if (Options.Centered) {
            MONITORINFO Monitor {};
            Monitor.cbSize = sizeof(Monitor);
            GetMonitorInfoW(MonitorFromWindow(_Handle, MONITOR_DEFAULTTONEAREST), &Monitor);
            const RECT& Work = Monitor.rcWork;
            SetWindowPos(_Handle,
                         nullptr,
                         Work.left + std::max(0L, (Work.right - Work.left - FrameWidth) / 2),
                         Work.top + std::max(0L, (Work.bottom - Work.top - FrameHeight) / 2),
                         FrameWidth,
                         FrameHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        } else {
            SetWindowPos(_Handle, nullptr, 0, 0, FrameWidth, FrameHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Not needed by WinApp itself, but most modern Win32 APIs (file dialogs, shell, WIC, ...) expect an STA.
        _ComInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    }

    App::~App() {
        _ControlsByHandle.clear();
        _Controls.clear();
        if (_Handle) { DestroyWindow(_Handle); }
        if (_ComInitialized) { CoUninitialize(); }
        if (_LargeIcon) { DestroyIcon(_LargeIcon); }
        if (_SmallIcon) { DestroyIcon(_SmallIcon); }
    }

    int App::Run() {
        ShowWindow(_Handle, SW_SHOWDEFAULT);
        UpdateWindow(_Handle);

        MSG Message {};
        while (GetMessageW(&Message, nullptr, 0, 0) > 0) {
            if (HandleShortcut(Message)) { continue; }
            // IsDialogMessage gives us Tab / Shift+Tab / arrow-key navigation between controls for free.
            if (!_Handle || !IsDialogMessageW(_Handle, &Message)) {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }
        }

        if (_PendingException) { std::rethrow_exception(std::exchange(_PendingException, nullptr)); }
        return static_cast<int>(Message.wParam);
    }

    void App::Close() {
        PostMessageW(_Handle, WM_CLOSE, 0, 0);
    }

    void App::Post(std::function<void()> Callback) {
        std::lock_guard Lock(_PostMutex);
        if (!_Handle) { return; }
        _Posted.push_back(std::move(Callback));
        // One message drains the whole queue, so only post when the queue was empty.
        if (_Posted.size() == 1) { PostMessageW(_Handle, kPostMessage, 0, 0); }
    }

    void App::RunPosted() {
        std::vector<std::function<void()>> Callbacks;
        {
            std::lock_guard Lock(_PostMutex);
            Callbacks.swap(_Posted);
        }
        for (const auto& Callback : Callbacks) { Callback(); }
    }

    void App::SetIcon(int Resource) {
        const HINSTANCE Instance = GetModuleHandleW(nullptr);
        if (SUCCEEDED(LoadIconMetric(Instance, MAKEINTRESOURCEW(Resource), LIM_LARGE, &_LargeIcon))) {
            SendMessageW(_Handle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(_LargeIcon));
        }
        if (SUCCEEDED(LoadIconMetric(Instance, MAKEINTRESOURCEW(Resource), LIM_SMALL, &_SmallIcon))) {
            SendMessageW(_Handle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_SmallIcon));
        }
    }

    void App::Register(std::unique_ptr<Control> Owned) {
        Owned->Create(*this, _NextId++);
        _ControlsByHandle.emplace(Owned->GetHandle(), Owned.get());
        _Controls.push_back(std::move(Owned));
    }

    void App::Remove(Control& Target) {
        const auto It = std::ranges::find_if(_Controls, [&](const auto& Owned) { return Owned.get() == &Target; });
        if (It == _Controls.end()) { return; }
        if (_LastFocus == Target.GetHandle()) { _LastFocus = nullptr; }
        _ControlsByHandle.erase(Target.GetHandle());
        _Controls.erase(It);
    }

    Control* App::FindControl(HWND Handle) const {
        const auto It = _ControlsByHandle.find(Handle);
        return It != _ControlsByHandle.end() ? It->second : nullptr;
    }

    void App::SetTitle(const std::wstring& Title) {
        SetWindowTextW(_Handle, Title.c_str());
    }

    std::wstring App::GetTitle() const {
        std::wstring Title(static_cast<size_t>(GetWindowTextLengthW(_Handle)), L'\0');
        if (!Title.empty()) {
            Title.resize(static_cast<size_t>(GetWindowTextW(_Handle, Title.data(), static_cast<int>(Title.size()) + 1)));
        }
        return Title;
    }

    Size App::GetClientSize() const {
        RECT Client {};
        GetClientRect(_Handle, &Client);
        return {Unscale(Client.right), Unscale(Client.bottom)};
    }

    void App::ShowMessage(const std::wstring& Heading, const std::wstring& Text, MessageIcon Icon) const {
        PCWSTR IconResource = nullptr;
        switch (Icon) {
            case MessageIcon::Information:
                IconResource = TD_INFORMATION_ICON;
                break;
            case MessageIcon::Warning:
                IconResource = TD_WARNING_ICON;
                break;
            case MessageIcon::Error:
                IconResource = TD_ERROR_ICON;
                break;
            case MessageIcon::None:
                break;
        }
        const auto Title = GetTitle();
        TaskDialog(_Handle,
                   nullptr,
                   Title.c_str(),
                   Heading.c_str(),
                   Text.empty() ? nullptr : Text.c_str(),
                   TDCBF_OK_BUTTON,
                   IconResource,
                   nullptr);
    }

    bool App::Confirm(const std::wstring& Heading, const std::wstring& Text) const {
        const auto Title = GetTitle();
        int Pressed      = 0;
        TaskDialog(_Handle,
                   nullptr,
                   Title.c_str(),
                   Heading.c_str(),
                   Text.empty() ? nullptr : Text.c_str(),
                   TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                   nullptr,
                   &Pressed);
        return Pressed == IDYES;
    }

    Menu& App::GetMenuBar() {
        if (!_MenuBar) {
            RECT Client {};
            GetClientRect(_Handle, &Client);

            _MenuBar.reset(new Menu(Menu::MenuBarTag {}, _Handle));
            SetMenu(_Handle, _MenuBar->GetHandle());

            // Attaching a menu bar eats into the client area; grow the window to compensate.
            RECT Frame = Client;
            AdjustWindowRectExForDpi(&Frame,
                                     static_cast<DWORD>(GetWindowLongPtrW(_Handle, GWL_STYLE)),
                                     TRUE,
                                     static_cast<DWORD>(GetWindowLongPtrW(_Handle, GWL_EXSTYLE)),
                                     _Dpi);
            SetWindowPos(_Handle,
                         nullptr,
                         0,
                         0,
                         Frame.right - Frame.left,
                         Frame.bottom - Frame.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return *_MenuBar;
    }

    void App::ShowPopup(Menu& Popup, POINT ScreenPosition) {
        // Without this the menu won't close when the user clicks elsewhere (see TrackPopupMenu docs).
        SetForegroundWindow(_Handle);

        UINT Flags = TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN;
        Flags |= GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;

        _ActivePopup = &Popup;
        const auto Id = static_cast<UINT>(
          TrackPopupMenuEx(Popup.GetHandle(), Flags, ScreenPosition.x, ScreenPosition.y, _Handle, nullptr));
        _ActivePopup = nullptr;
        PostMessageW(_Handle, WM_NULL, 0, 0);

        if (auto* Item = Id ? Popup.FindItem(Id) : nullptr) { Item->Invoke(); }
    }

    void App::ShowContextMenu(HWND Source, POINT ScreenPosition) {
        Menu* Target = nullptr;
        if (auto* Owner = FindControl(Source)) {
            Target = Owner->_ContextMenu;
        } else if (Source == _Handle) {
            Target = _ContextMenu;
        }
        if (!Target) { return; }

        // Shift+F10 / the Menu key report (-1, -1); open at the top-left of whatever was focused instead.
        if (ScreenPosition.x == -1 && ScreenPosition.y == -1) {
            RECT Bounds {};
            if (Source == _Handle) {
                GetClientRect(_Handle, &Bounds);
                MapWindowPoints(_Handle, nullptr, reinterpret_cast<POINT*>(&Bounds), 2);
            } else {
                GetWindowRect(Source, &Bounds);
            }
            ScreenPosition = {Bounds.left, Bounds.top};
        }
        ShowPopup(*Target, ScreenPosition);
    }

    bool App::HandleShortcut(const MSG& Message) {
        if (!_MenuBar || (Message.message != WM_KEYDOWN && Message.message != WM_SYSKEYDOWN)) { return false; }
        if (Message.hwnd != _Handle && !IsChild(_Handle, Message.hwnd)) { return false; }

        Modifiers Mods = Modifiers::None;
        if (GetKeyState(VK_CONTROL) < 0) { Mods = Mods | Modifiers::Ctrl; }
        if (GetKeyState(VK_SHIFT) < 0) { Mods = Mods | Modifiers::Shift; }
        if (GetKeyState(VK_MENU) < 0) { Mods = Mods | Modifiers::Alt; }

        auto* Item = _MenuBar->FindItem(Shortcut {Mods, static_cast<UINT>(Message.wParam)});
        if (!Item) { return false; }
        // A disabled item still swallows its shortcut, matching how accelerator tables behave.
        if (Item->IsEnabled()) { Item->Invoke(); }
        return true;
    }

    void App::RefreshFont() {
        _Font = Font::System(_Dpi);
        for (const auto& Owned : _Controls) { Owned->ApplyFont(_Font.GetHandle()); }
    }

    void App::RestoreFocus() {
        HWND Target = _LastFocus;
        if (!Target || !IsWindow(Target) || !IsChild(_Handle, Target)) {
            Target = GetNextDlgTabItem(_Handle, nullptr, FALSE);
        }
        if (Target) { SetFocus(Target); }
    }

    LRESULT CALLBACK App::WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam) {
        App* Self = nullptr;
        if (Message == WM_NCCREATE) {
            Self          = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(LParam)->lpCreateParams);
            Self->_Handle = Handle;
            Self->_Dpi    = GetDpiForWindow(Handle);
            SetWindowLongPtrW(Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Self));
        } else {
            Self = reinterpret_cast<App*>(GetWindowLongPtrW(Handle, GWLP_USERDATA));
        }

        if (!Self) { return DefWindowProcW(Handle, Message, WParam, LParam); }

        try {
            return Self->HandleMessage(Message, WParam, LParam);
        } catch (...) {
            if (!Self->_PendingException) { Self->_PendingException = std::current_exception(); }
            PostQuitMessage(EXIT_FAILURE);
            return DefWindowProcW(Handle, Message, WParam, LParam);
        }
    }

    LRESULT App::HandleMessage(UINT Message, WPARAM WParam, LPARAM LParam) {
        if (Message == kPostMessage) {
            RunPosted();
            return 0;
        }

        switch (Message) {
            case WM_COMMAND:
                if (auto* Target = FindControl(reinterpret_cast<HWND>(LParam))) {
                    Target->OnCommand(HIWORD(WParam));
                    return 0;
                }
                // Menu-bar clicks arrive with no control handle and a notification code of 0.
                if (LParam == 0 && HIWORD(WParam) == 0 && _MenuBar) {
                    if (auto* Item = _MenuBar->FindItem(static_cast<UINT>(LOWORD(WParam)))) {
                        Item->Invoke();
                        return 0;
                    }
                }
                break;

            case WM_INITMENUPOPUP: {
                const auto Popup = reinterpret_cast<HMENU>(WParam);
                Menu* Target     = _MenuBar ? _MenuBar->FindMenu(Popup) : nullptr;
                if (!Target && _ActivePopup) { Target = _ActivePopup->FindMenu(Popup); }
                if (Target && Target->_OpenHandler) {
                    Target->_OpenHandler();
                    return 0;
                }
                break;
            }

            case WM_CONTEXTMENU: {
                const auto Source = reinterpret_cast<HWND>(WParam);
                POINT Position {GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam)};
                // Right-clicks on the title bar / frame keep showing the system menu.
                if (Source == _Handle && !(Position.x == -1 && Position.y == -1)) {
                    POINT Client = Position;
                    ScreenToClient(_Handle, &Client);
                    RECT Bounds {};
                    GetClientRect(_Handle, &Bounds);
                    if (!PtInRect(&Bounds, Client)) { break; }
                }
                ShowContextMenu(Source, Position);
                return 0;
            }

            case WM_NOTIFY: {
                const auto* Header = reinterpret_cast<NMHDR*>(LParam);
                if (auto* Target = FindControl(Header->hwndFrom)) {
                    return Target->OnNotify(reinterpret_cast<NMHDR*>(LParam));
                }
                break;
            }

            case WM_HSCROLL:
            case WM_VSCROLL:
                if (auto* Target = FindControl(reinterpret_cast<HWND>(LParam))) {
                    Target->OnScroll(LOWORD(WParam));
                    return 0;
                }
                break;

            // Static-style controls (labels, check boxes, radio buttons, group boxes, sliders) default to the
            // button-face color; paint them with the window color so they blend into the background.
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORBTN:
                SetBkColor(reinterpret_cast<HDC>(WParam), GetSysColor(COLOR_WINDOW));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));

            case WM_SIZE:
                if (_ResizeHandler && WParam != SIZE_MINIMIZED) {
                    _ResizeHandler({Unscale(LOWORD(LParam)), Unscale(HIWORD(LParam))});
                }
                return 0;

            case WM_GETMINMAXINFO:
                if (_MinSize.Width > 0 || _MinSize.Height > 0) {
                    RECT Frame {0, 0, Scale(_MinSize.Width), Scale(_MinSize.Height)};
                    AdjustWindowRectExForDpi(&Frame,
                                             static_cast<DWORD>(GetWindowLongPtrW(_Handle, GWL_STYLE)),
                                             _MenuBar ? TRUE : FALSE,
                                             static_cast<DWORD>(GetWindowLongPtrW(_Handle, GWL_EXSTYLE)),
                                             _Dpi);
                    auto* Info = reinterpret_cast<MINMAXINFO*>(LParam);
                    if (_MinSize.Width > 0) { Info->ptMinTrackSize.x = Frame.right - Frame.left; }
                    if (_MinSize.Height > 0) { Info->ptMinTrackSize.y = Frame.bottom - Frame.top; }
                    return 0;
                }
                break;

            case WM_DPICHANGED: {
                _Dpi = HIWORD(WParam);
                RefreshFont();
                for (const auto& Owned : _Controls) { Owned->ApplyLayout(); }
                const auto* Suggested = reinterpret_cast<RECT*>(LParam);
                SetWindowPos(_Handle,
                             nullptr,
                             Suggested->left,
                             Suggested->top,
                             Suggested->right - Suggested->left,
                             Suggested->bottom - Suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }

            case WM_SETTINGCHANGE:
                if (WParam == SPI_SETNONCLIENTMETRICS) { RefreshFont(); }
                break;

            case WM_ACTIVATE:
                if (LOWORD(WParam) == WA_INACTIVE) {
                    const HWND Focused = GetFocus();
                    if (Focused && IsChild(_Handle, Focused)) { _LastFocus = Focused; }
                }
                break;

            case WM_SETFOCUS:
                RestoreFocus();
                return 0;

            case WM_CLOSE:
                if (_CloseHandler && !_CloseHandler()) { return 0; }
                DestroyWindow(_Handle);
                return 0;

            case WM_DESTROY:
                // DestroyWindow would destroy an attached menu bar; detach it so _MenuBar keeps sole ownership.
                SetMenu(_Handle, nullptr);
                PostQuitMessage(0);
                return 0;

            case WM_NCDESTROY: {
                // Child windows are gone at this point; make sure the controls don't try to destroy them again.
                for (const auto& Owned : _Controls) { Owned->_Handle = nullptr; }
                _ControlsByHandle.clear();
                SetWindowLongPtrW(_Handle, GWLP_USERDATA, 0);
                HWND Handle = nullptr;
                {
                    std::lock_guard Lock(_PostMutex);
                    Handle = std::exchange(_Handle, nullptr);
                    _Posted.clear();
                }
                return DefWindowProcW(Handle, Message, WParam, LParam);
            }

            default:
                break;
        }

        return DefWindowProcW(_Handle, Message, WParam, LParam);
    }
}  // namespace WinApp
