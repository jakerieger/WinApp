#include "Button.hpp"

namespace WinApp {
    Button::Button(std::wstring Text, const Rect& Bounds) : Control(std::move(Text), Bounds) {}

    Control::CreateInfo Button::GetCreateInfo() const {
        return {.ClassName = WC_BUTTONW, .Style = BS_PUSHBUTTON | WS_TABSTOP};
    }

    void Button::OnCommand(WORD Code) {
        if (Code == BN_CLICKED && _ClickHandler) { _ClickHandler(); }
    }
}  // namespace WinApp
