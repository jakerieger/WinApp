#include "Menu.hpp"
#include "App.hpp"

#include <system_error>

namespace WinApp {
    namespace {
        UINT NextCommandId() {
            // WM_COMMAND only carries 16-bit IDs. Stay clear of the small IDs (IDOK, IDCANCEL, ...) that
            // IsDialogMessage sends for Enter / Escape.
            static UINT Next = 0x1000;
            if (Next > 0xFFFF) { Next = 0x1000; }
            return Next++;
        }

        std::wstring KeyName(UINT Key) {
            if ((Key >= 'A' && Key <= 'Z') || (Key >= '0' && Key <= '9')) { return std::wstring(1, static_cast<wchar_t>(Key)); }
            if (Key >= VK_F1 && Key <= VK_F24) { return L"F" + std::to_wstring(Key - VK_F1 + 1); }

            bool Extended = false;
            switch (Key) {
                case VK_INSERT:
                case VK_DELETE:
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:
                case VK_LEFT:
                case VK_RIGHT:
                case VK_UP:
                case VK_DOWN:
                    Extended = true;
                    break;
                default:
                    break;
            }

            LONG ScanCode = static_cast<LONG>(MapVirtualKeyW(Key, MAPVK_VK_TO_VSC)) << 16;
            if (Extended) { ScanCode |= 1 << 24; }
            wchar_t Name[64] {};
            return GetKeyNameTextW(ScanCode, Name, static_cast<int>(std::size(Name))) > 0 ? Name : std::wstring {};
        }

        std::wstring DisplayText(const std::wstring& Text, const Shortcut& Keys) {
            return Keys.IsEmpty() ? Text : Text + L"\t" + Keys.ToString();
        }
    }  // namespace

    std::wstring Shortcut::ToString() const {
        if (IsEmpty()) { return {}; }
        std::wstring Text;
        if (HasModifier(Mods, Modifiers::Ctrl)) { Text += L"Ctrl+"; }
        if (HasModifier(Mods, Modifiers::Shift)) { Text += L"Shift+"; }
        if (HasModifier(Mods, Modifiers::Alt)) { Text += L"Alt+"; }
        return Text + KeyName(Key);
    }

    // MenuItem ------------------------------------------------------------------------------------------------------

    MenuItem::MenuItem(HMENU Parent, UINT Id, Shortcut Keys, bool Checkable)
        : _Parent(Parent), _Id(Id), _Shortcut(Keys), _Checkable(Checkable) {}

    void MenuItem::SetText(const std::wstring& Text) {
        auto Display = DisplayText(Text, _Shortcut);
        MENUITEMINFOW Info {};
        Info.cbSize     = sizeof(Info);
        Info.fMask      = MIIM_STRING;
        Info.dwTypeData = Display.data();
        SetMenuItemInfoW(_Parent, _Id, FALSE, &Info);
    }

    void MenuItem::SetEnabled(bool Enabled) {
        EnableMenuItem(_Parent, _Id, MF_BYCOMMAND | (Enabled ? MF_ENABLED : MF_GRAYED));
    }

    bool MenuItem::IsEnabled() const {
        return (GetMenuState(_Parent, _Id, MF_BYCOMMAND) & (MF_GRAYED | MF_DISABLED)) == 0;
    }

    void MenuItem::SetChecked(bool Checked) {
        CheckMenuItem(_Parent, _Id, MF_BYCOMMAND | (Checked ? MF_CHECKED : MF_UNCHECKED));
    }

    bool MenuItem::IsChecked() const {
        return (GetMenuState(_Parent, _Id, MF_BYCOMMAND) & MF_CHECKED) != 0;
    }

    void MenuItem::Invoke() {
        if (_Checkable) { SetChecked(!IsChecked()); }
        if (_Handler) { _Handler(); }
    }

    // Menu ----------------------------------------------------------------------------------------------------------

    Menu::Menu() : _Handle(CreatePopupMenu()) {
        if (!_Handle) { throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CreatePopupMenu"); }
    }

    Menu::Menu(MenuBarTag, HWND Window) : _Handle(CreateMenu()), _Window(Window) {
        if (!_Handle) { throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CreateMenu"); }
    }

    Menu::~Menu() {
        if (_OwnsHandle && _Handle) { DestroyMenu(_Handle); }
    }

    MenuItem& Menu::Append(const std::wstring& Text, Shortcut Keys, bool Checkable) {
        const UINT Id = NextCommandId();
        AppendMenuW(_Handle, MF_STRING, Id, DisplayText(Text, Keys).c_str());
        _Items.push_back(std::unique_ptr<MenuItem>(new MenuItem(_Handle, Id, Keys, Checkable)));
        Refresh();
        return *_Items.back();
    }

    MenuItem& Menu::AddItem(const std::wstring& Text, std::function<void()> OnClick) {
        return AddItem(Text, {}, std::move(OnClick));
    }

    MenuItem& Menu::AddItem(const std::wstring& Text, Shortcut Keys, std::function<void()> OnClick) {
        auto& Item    = Append(Text, Keys, false);
        Item._Handler = std::move(OnClick);
        return Item;
    }

    MenuItem& Menu::AddCheckItem(const std::wstring& Text, bool Checked, std::function<void(bool)> OnToggle) {
        return AddCheckItem(Text, {}, Checked, std::move(OnToggle));
    }

    MenuItem& Menu::AddCheckItem(const std::wstring& Text,
                                 Shortcut Keys,
                                 bool Checked,
                                 std::function<void(bool)> OnToggle) {
        auto& Item = Append(Text, Keys, true);
        Item.SetChecked(Checked);
        Item._Handler = [&Item, Handler = std::move(OnToggle)] {
            if (Handler) { Handler(Item.IsChecked()); }
        };
        return Item;
    }

    Menu& Menu::AddSubmenu(const std::wstring& Text) {
        auto Submenu         = std::make_unique<Menu>();
        Submenu->_OwnsHandle = false;
        AppendMenuW(_Handle, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(Submenu->_Handle), Text.c_str());
        _Submenus.push_back(std::move(Submenu));
        Refresh();
        return *_Submenus.back();
    }

    void Menu::AddSeparator() {
        AppendMenuW(_Handle, MF_SEPARATOR, 0, nullptr);
        Refresh();
    }

    void Menu::Clear() {
        // DeleteMenu also destroys submenu handles, which is why submenus don't own theirs.
        while (GetMenuItemCount(_Handle) > 0) { DeleteMenu(_Handle, 0, MF_BYPOSITION); }
        _Items.clear();
        _Submenus.clear();
        Refresh();
    }

    void Menu::Show(App& Owner) {
        POINT Cursor {};
        GetCursorPos(&Cursor);
        Owner.ShowPopup(*this, Cursor);
    }

    void Menu::Show(App& Owner, const Control& Anchor) {
        RECT Bounds {};
        GetWindowRect(Anchor.GetHandle(), &Bounds);
        Owner.ShowPopup(*this, {Bounds.left, Bounds.bottom});
    }

    void Menu::Refresh() const {
        if (_Window) { DrawMenuBar(_Window); }
    }

    MenuItem* Menu::FindItem(UINT Id) const {
        for (const auto& Item : _Items) {
            if (Item->_Id == Id) { return Item.get(); }
        }
        for (const auto& Submenu : _Submenus) {
            if (auto* Found = Submenu->FindItem(Id)) { return Found; }
        }
        return nullptr;
    }

    MenuItem* Menu::FindItem(const Shortcut& Keys) const {
        for (const auto& Item : _Items) {
            if (!Item->_Shortcut.IsEmpty() && Item->_Shortcut == Keys) { return Item.get(); }
        }
        for (const auto& Submenu : _Submenus) {
            if (auto* Found = Submenu->FindItem(Keys)) { return Found; }
        }
        return nullptr;
    }

    Menu* Menu::FindMenu(HMENU Handle) {
        if (_Handle == Handle) { return this; }
        for (const auto& Submenu : _Submenus) {
            if (auto* Found = Submenu->FindMenu(Handle)) { return Found; }
        }
        return nullptr;
    }
}  // namespace WinApp
