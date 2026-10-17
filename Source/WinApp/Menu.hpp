#pragma once

#include "Common.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace WinApp {
    class App;
    class Control;

    enum class Modifiers : uint8_t { None = 0, Ctrl = 1 << 0, Shift = 1 << 1, Alt = 1 << 2 };

    constexpr Modifiers operator|(Modifiers Left, Modifiers Right) {
        return static_cast<Modifiers>(static_cast<uint8_t>(Left) | static_cast<uint8_t>(Right));
    }

    constexpr bool HasModifier(Modifiers Set, Modifiers Flag) {
        return (static_cast<uint8_t>(Set) & static_cast<uint8_t>(Flag)) != 0;
    }

    /// A keyboard shortcut such as {Modifiers::Ctrl, 'S'} or {Modifiers::None, VK_F5}. Key is a virtual-key code;
    /// letters and digits are their uppercase character.
    struct Shortcut {
        Modifiers Mods = Modifiers::None;
        UINT Key       = 0;

        [[nodiscard]] bool IsEmpty() const { return Key == 0; }
        /// Display text, e.g. "Ctrl+Shift+S".
        [[nodiscard]] std::wstring ToString() const;

        bool operator==(const Shortcut&) const = default;
    };

    /// A clickable entry in a Menu. Created by Menu::AddItem / Menu::AddCheckItem and owned by its menu.
    class MenuItem {
    public:
        MenuItem(const MenuItem&)            = delete;
        MenuItem& operator=(const MenuItem&) = delete;

        void SetText(const std::wstring& Text);

        void SetEnabled(bool Enabled);
        [[nodiscard]] bool IsEnabled() const;

        void SetChecked(bool Checked);
        [[nodiscard]] bool IsChecked() const;

        [[nodiscard]] const Shortcut& GetShortcut() const { return _Shortcut; }

    private:
        friend class Menu;
        friend class App;

        MenuItem(HMENU Parent, UINT Id, Shortcut Keys, bool Checkable);
        void Invoke();

        HMENU _Parent;
        UINT _Id;
        Shortcut _Shortcut;
        bool _Checkable;
        std::function<void()> _Handler;
    };

    /// A drop-down menu: either a submenu of the window's menu bar (App::GetMenuBar) or a standalone popup/context
    /// menu that you show with Show() or attach with SetContextMenu().
    ///
    /// Item text may contain '&' to mark the Alt-key mnemonic, e.g. L"&Open".
    class Menu {
    public:
        Menu();
        ~Menu();

        Menu(const Menu&)            = delete;
        Menu& operator=(const Menu&) = delete;

        MenuItem& AddItem(const std::wstring& Text, std::function<void()> OnClick);
        /// Shortcuts on menu-bar items work app-wide; on popup menus they're only displayed.
        MenuItem& AddItem(const std::wstring& Text, Shortcut Keys, std::function<void()> OnClick);

        /// An item that toggles its check mark when clicked, then reports the new state.
        MenuItem& AddCheckItem(const std::wstring& Text, bool Checked, std::function<void(bool)> OnToggle);
        MenuItem& AddCheckItem(const std::wstring& Text, Shortcut Keys, bool Checked, std::function<void(bool)> OnToggle);

        Menu& AddSubmenu(const std::wstring& Text);
        void AddSeparator();

        /// Removes every item, separator and submenu. References to them become invalid.
        void Clear();

        /// Called right before the menu opens, e.g. to enable/disable or check items based on current state.
        void OnOpen(std::function<void()> Handler) { _OpenHandler = std::move(Handler); }

        /// Shows the menu as a popup at the mouse cursor and blocks until it closes.
        void Show(App& Owner);
        /// Shows the menu as a popup just below a control (for drop-down buttons) and blocks until it closes.
        void Show(App& Owner, const Control& Anchor);

        [[nodiscard]] HMENU GetHandle() const { return _Handle; }

    private:
        friend class App;

        struct MenuBarTag {};
        Menu(MenuBarTag, HWND Window);

        MenuItem& Append(const std::wstring& Text, Shortcut Keys, bool Checkable);
        void Refresh() const;

        [[nodiscard]] MenuItem* FindItem(UINT Id) const;
        [[nodiscard]] MenuItem* FindItem(const Shortcut& Keys) const;
        [[nodiscard]] Menu* FindMenu(HMENU Handle);

        HMENU _Handle    = nullptr;
        HWND _Window     = nullptr;  // Set for the menu bar so changes can be redrawn.
        bool _OwnsHandle = true;     // Submenus are destroyed along with their parent's HMENU.
        std::vector<std::unique_ptr<MenuItem>> _Items;
        std::vector<std::unique_ptr<Menu>> _Submenus;
        std::function<void()> _OpenHandler;
    };
}  // namespace WinApp
