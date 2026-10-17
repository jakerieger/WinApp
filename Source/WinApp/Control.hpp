#pragma once

#include "Common.hpp"

#include <string>

namespace WinApp {
    class App;
    class Menu;

    /// Base class for every child control. Controls are created and owned by App (see App::Add), and their bounds are
    /// expressed in device-independent pixels so they stay correctly sized across DPI changes.
    class Control {
    public:
        virtual ~Control();

        Control(const Control&)            = delete;
        Control& operator=(const Control&) = delete;

        void SetText(const std::wstring& Text);
        [[nodiscard]] std::wstring GetText() const;

        void SetBounds(const Rect& Bounds);
        [[nodiscard]] const Rect& GetBounds() const { return _Bounds; }

        void SetEnabled(bool Enabled);
        [[nodiscard]] bool IsEnabled() const;

        void SetVisible(bool Visible);
        [[nodiscard]] bool IsVisible() const;

        void Focus();

        /// Menu shown when the control is right-clicked or Shift+F10 is pressed (nullptr to remove). Not owned; must
        /// outlive the control. Edit-based controls (TextBox) keep their built-in Cut/Copy/Paste menu instead.
        void SetContextMenu(Menu* ContextMenu) { _ContextMenu = ContextMenu; }

        [[nodiscard]] App& GetApp() const { return *_Owner; }
        /// Escape hatch for anything WinApp doesn't wrap yet.
        [[nodiscard]] HWND GetHandle() const { return _Handle; }

    protected:
        struct CreateInfo {
            const wchar_t* ClassName = nullptr;
            DWORD Style              = 0;
            DWORD ExStyle            = 0;
            /// Marks the start of a new keyboard-navigation group (WS_GROUP). Only radio buttons turn this off so
            /// consecutive radios form one arrow-key group.
            bool StartsGroup = true;
        };

        Control(std::wstring Text, const Rect& Bounds);

        [[nodiscard]] virtual CreateInfo GetCreateInfo() const = 0;
        virtual void OnCreated() {}
        virtual void OnCommand(WORD /*Code*/) {}
        virtual LRESULT OnNotify(NMHDR* /*Header*/) { return 0; }
        virtual void OnScroll(WORD /*Code*/) {}

        LRESULT Send(UINT Message, WPARAM WParam = 0, LPARAM LParam = 0) const;

    private:
        friend class App;

        void Create(App& Owner, int Id);
        void ApplyLayout() const;
        void ApplyFont(HFONT Font) const;

        App* _Owner  = nullptr;
        HWND _Handle = nullptr;
        Rect _Bounds;
        std::wstring _InitialText;
        Menu* _ContextMenu = nullptr;
    };
}  // namespace WinApp
