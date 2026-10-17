#include "Label.hpp"

namespace WinApp {
    Label::Label(std::wstring Text, const Rect& Bounds, TextAlign Align, TextOverflow Overflow)
        : Control(std::move(Text), Bounds), _Align(Align), _Overflow(Overflow) {}

    Control::CreateInfo Label::GetCreateInfo() const {
        DWORD Style = SS_LEFT;
        if (_Align == TextAlign::Center) { Style = SS_CENTER; }
        if (_Align == TextAlign::Right) { Style = SS_RIGHT; }
        if (_Overflow == TextOverflow::End) { Style |= SS_ENDELLIPSIS; }
        if (_Overflow == TextOverflow::Path) { Style |= SS_PATHELLIPSIS; }
        // SS_NOTIFY makes the label hit-testable so it can have its own context menu.
        return {.ClassName = WC_STATICW, .Style = Style | SS_NOTIFY};
    }
}  // namespace WinApp
