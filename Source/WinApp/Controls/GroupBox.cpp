#include "GroupBox.hpp"

namespace WinApp {
    GroupBox::GroupBox(std::wstring Text, const Rect& Bounds) : Control(std::move(Text), Bounds) {}

    Control::CreateInfo GroupBox::GetCreateInfo() const {
        return {.ClassName = WC_BUTTONW, .Style = BS_GROUPBOX};
    }
}  // namespace WinApp
