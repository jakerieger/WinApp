#pragma once

#include "Common.hpp"
#include "Control.hpp"
#include "Font.hpp"
#include "Menu.hpp"

#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define APP_ENTRY int WINAPI wWinMain

namespace WinApp {
    struct AppOptions {
        std::wstring Title = L"WinApp";
        /// Initial client-area size in device-independent pixels.
        int Width  = 800;
        int Height = 600;
        /// Minimum client-area size in device-independent pixels (0 = no limit).
        int MinWidth   = 0;
        int MinHeight  = 0;
        bool Resizable = true;
        bool Centered  = true;
        /// Resource ID of the window's icon (0 = the default application icon).
        int Icon = 0;
    };

    enum class MessageIcon { None, Information, Warning, Error };

    /// The root of a WinApp application: owns the main window, its controls, and the message loop.
    class App {
    public:
        explicit App(const AppOptions& Options = {});
        ~App();

        App(const App&)            = delete;
        App& operator=(const App&) = delete;

        /// Shows the window and runs the message loop until the window is closed. Returns the process exit code.
        int Run();
        /// Requests the window to close (goes through the OnClose handler).
        void Close();

        /// Queues Callback to run on the UI thread, where it may touch controls. Safe to call from any thread while the
        /// App is alive; callbacks still queued when the window is destroyed are dropped.
        void Post(std::function<void()> Callback);

        /// Creates a control as a child of the main window. The App owns it; the returned reference stays valid until
        /// Remove() is called or the App is destroyed.
        template<std::derived_from<Control> T, class... Args>
        T& Add(Args&&... Arguments) {
            auto Owned = std::make_unique<T>(std::forward<Args>(Arguments)...);
            T& Ref     = *Owned;
            Register(std::move(Owned));
            return Ref;
        }

        /// Destroys a control previously created with Add(). Any reference to it becomes invalid.
        void Remove(Control& Target);

        void SetTitle(const std::wstring& Title);
        [[nodiscard]] std::wstring GetTitle() const;

        /// Client-area size in device-independent pixels.
        [[nodiscard]] Size GetClientSize() const;

        /// The window's menu bar, created on first use. Add top-level menus with AddSubmenu(). The window grows so the
        /// client area keeps its size.
        Menu& GetMenuBar();
        /// Context menu for right-clicks on the window background (nullptr to remove). Not owned; must outlive the App.
        void SetContextMenu(Menu* ContextMenu) { _ContextMenu = ContextMenu; }

        /// Called whenever the client area changes size, with the new size in device-independent pixels.
        void OnResize(std::function<void(Size)> Handler) { _ResizeHandler = std::move(Handler); }
        /// Called when the user tries to close the window. Return false to keep the window open.
        void OnClose(std::function<bool()> Handler) { _CloseHandler = std::move(Handler); }

        void ShowMessage(const std::wstring& Heading,
                         const std::wstring& Text = {},
                         MessageIcon Icon         = MessageIcon::Information) const;
        /// Shows a Yes/No prompt and returns true if the user picked Yes.
        [[nodiscard]] bool Confirm(const std::wstring& Heading, const std::wstring& Text = {}) const;

        [[nodiscard]] UINT GetDpi() const { return _Dpi; }
        /// Converts device-independent pixels to physical pixels at the window's current DPI.
        [[nodiscard]] int Scale(int Value) const { return MulDiv(Value, static_cast<int>(_Dpi), USER_DEFAULT_SCREEN_DPI); }
        /// Converts physical pixels at the window's current DPI to device-independent pixels.
        [[nodiscard]] int Unscale(int Value) const { return MulDiv(Value, USER_DEFAULT_SCREEN_DPI, static_cast<int>(_Dpi)); }

        [[nodiscard]] HFONT GetFont() const { return _Font.GetHandle(); }
        /// Escape hatch for anything WinApp doesn't wrap yet.
        [[nodiscard]] HWND GetHandle() const { return _Handle; }

    private:
        friend class Menu;

        static LRESULT CALLBACK WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);
        LRESULT HandleMessage(UINT Message, WPARAM WParam, LPARAM LParam);

        void Register(std::unique_ptr<Control> Owned);
        [[nodiscard]] Control* FindControl(HWND Handle) const;
        void RefreshFont();
        void RestoreFocus();
        void ShowPopup(Menu& Popup, POINT ScreenPosition);
        bool HandleShortcut(const MSG& Message);
        void ShowContextMenu(HWND Source, POINT ScreenPosition);
        void SetIcon(int Resource);
        void RunPosted();

        HWND _Handle = nullptr;
        UINT _Dpi    = USER_DEFAULT_SCREEN_DPI;
        Font _Font;
        Size _MinSize;
        bool _ComInitialized = false;
        HICON _LargeIcon     = nullptr;
        HICON _SmallIcon     = nullptr;

        std::vector<std::unique_ptr<Control>> _Controls;
        std::unordered_map<HWND, Control*> _ControlsByHandle;
        int _NextId     = 100;
        HWND _LastFocus = nullptr;

        std::unique_ptr<Menu> _MenuBar;
        Menu* _ContextMenu = nullptr;
        Menu* _ActivePopup = nullptr;

        std::function<void(Size)> _ResizeHandler;
        std::function<bool()> _CloseHandler;

        // Callbacks queued by Post(). Guarded by _PostMutex, as is _Handle whenever another thread may read it.
        std::mutex _PostMutex;
        std::vector<std::function<void()>> _Posted;

        // Exceptions must not unwind through system frames (DispatchMessage), so callbacks that throw are captured
        // here and rethrown from Run().
        std::exception_ptr _PendingException;
    };
}  // namespace WinApp
