#include "TextBox.hpp"

namespace WinApp {
    TextBox::TextBox(const Rect& Bounds, TextBoxOptions Options)
        : Control(Options.Text, Bounds), _Options(std::move(Options)) {
        _Options.Text.clear();
    }

    Control::CreateInfo TextBox::GetCreateInfo() const {
        DWORD Style = WS_TABSTOP;
        if (_Options.Multiline) {
            Style |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL;
        } else {
            Style |= ES_AUTOHSCROLL;
        }
        if (_Options.Password) { Style |= ES_PASSWORD; }
        if (_Options.ReadOnly) { Style |= ES_READONLY; }
        return {.ClassName = WC_EDITW, .Style = Style, .ExStyle = WS_EX_CLIENTEDGE};
    }

    void TextBox::OnCreated() {
        if (!_Options.Placeholder.empty()) { SetPlaceholder(_Options.Placeholder); }
    }

    void TextBox::SetPlaceholder(const std::wstring& Placeholder) {
        Send(EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(Placeholder.c_str()));
    }

    void TextBox::SetReadOnly(bool ReadOnly) {
        Send(EM_SETREADONLY, ReadOnly);
    }

    bool TextBox::IsReadOnly() const {
        return (GetWindowLongPtrW(GetHandle(), GWL_STYLE) & ES_READONLY) != 0;
    }

    void TextBox::SetMaxLength(int Length) {
        Send(EM_SETLIMITTEXT, static_cast<WPARAM>(Length));
    }

    void TextBox::AppendText(const std::wstring& Text) {
        const auto End = static_cast<WPARAM>(GetWindowTextLengthW(GetHandle()));
        Send(EM_SETSEL, End, static_cast<LPARAM>(End));
        Send(EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(Text.c_str()));
        Send(EM_SCROLLCARET);
    }

    void TextBox::SelectAll() {
        Send(EM_SETSEL, 0, -1);
    }

    void TextBox::OnCommand(WORD Code) {
        if (Code == EN_CHANGE && _ChangeHandler) { _ChangeHandler(); }
    }
}  // namespace WinApp
