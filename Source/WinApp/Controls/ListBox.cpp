#include "ListBox.hpp"

namespace WinApp {
    ListBox::ListBox(const Rect& Bounds) : Control({}, Bounds) {}

    Control::CreateInfo ListBox::GetCreateInfo() const {
        return {.ClassName = WC_LISTBOXW,
                .Style     = LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP,
                .ExStyle   = WS_EX_CLIENTEDGE};
    }

    int ListBox::AddItem(const std::wstring& Text) {
        return static_cast<int>(Send(LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Text.c_str())));
    }

    void ListBox::AddItems(std::initializer_list<std::wstring> Items) {
        for (const auto& Item : Items) { AddItem(Item); }
    }

    void ListBox::RemoveItem(int Index) {
        Send(LB_DELETESTRING, static_cast<WPARAM>(Index));
    }

    void ListBox::Clear() {
        Send(LB_RESETCONTENT);
    }

    int ListBox::GetCount() const {
        return static_cast<int>(Send(LB_GETCOUNT));
    }

    std::wstring ListBox::GetItem(int Index) const {
        const auto Length = Send(LB_GETTEXTLEN, static_cast<WPARAM>(Index));
        if (Length == LB_ERR) { return {}; }
        std::wstring Text(static_cast<size_t>(Length), L'\0');
        Send(LB_GETTEXT, static_cast<WPARAM>(Index), reinterpret_cast<LPARAM>(Text.data()));
        return Text;
    }

    int ListBox::GetSelectedIndex() const {
        return static_cast<int>(Send(LB_GETCURSEL));
    }

    void ListBox::SetSelectedIndex(int Index) {
        Send(LB_SETCURSEL, static_cast<WPARAM>(Index));
    }

    void ListBox::OnCommand(WORD Code) {
        if (Code == LBN_SELCHANGE && _SelectionHandler) { _SelectionHandler(GetSelectedIndex()); }
        if (Code == LBN_DBLCLK && _ActivateHandler) { _ActivateHandler(GetSelectedIndex()); }
    }
}  // namespace WinApp
