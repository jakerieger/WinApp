#include "ComboBox.hpp"

namespace WinApp {
    ComboBox::ComboBox(const Rect& Bounds) : Control({}, Bounds) {}

    Control::CreateInfo ComboBox::GetCreateInfo() const {
        return {.ClassName = WC_COMBOBOXW, .Style = CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP};
    }

    int ComboBox::AddItem(const std::wstring& Text) {
        return static_cast<int>(Send(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Text.c_str())));
    }

    void ComboBox::AddItems(std::initializer_list<std::wstring> Items) {
        for (const auto& Item : Items) { AddItem(Item); }
    }

    void ComboBox::RemoveItem(int Index) {
        Send(CB_DELETESTRING, static_cast<WPARAM>(Index));
    }

    void ComboBox::Clear() {
        Send(CB_RESETCONTENT);
    }

    int ComboBox::GetCount() const {
        return static_cast<int>(Send(CB_GETCOUNT));
    }

    std::wstring ComboBox::GetItem(int Index) const {
        const auto Length = Send(CB_GETLBTEXTLEN, static_cast<WPARAM>(Index));
        if (Length == CB_ERR) { return {}; }
        std::wstring Text(static_cast<size_t>(Length), L'\0');
        Send(CB_GETLBTEXT, static_cast<WPARAM>(Index), reinterpret_cast<LPARAM>(Text.data()));
        return Text;
    }

    int ComboBox::GetSelectedIndex() const {
        return static_cast<int>(Send(CB_GETCURSEL));
    }

    void ComboBox::SetSelectedIndex(int Index) {
        Send(CB_SETCURSEL, static_cast<WPARAM>(Index));
    }

    void ComboBox::OnCommand(WORD Code) {
        if (Code == CBN_SELCHANGE && _SelectionHandler) { _SelectionHandler(GetSelectedIndex()); }
    }
}  // namespace WinApp
