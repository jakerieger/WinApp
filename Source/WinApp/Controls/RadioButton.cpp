#include "RadioButton.hpp"

namespace WinApp {
    RadioButton::RadioButton(std::wstring Text, const Rect& Bounds, bool StartsGroup)
        : Control(std::move(Text), Bounds), _StartsGroup(StartsGroup) {}

    Control::CreateInfo RadioButton::GetCreateInfo() const {
        // Only the first radio of a group is a tab stop; arrow keys move within the group.
        return {.ClassName   = WC_BUTTONW,
                .Style       = static_cast<DWORD>(BS_AUTORADIOBUTTON | (_StartsGroup ? WS_TABSTOP : 0)),
                .StartsGroup = _StartsGroup};
    }

    void RadioButton::SetChecked(bool Checked) {
        Send(BM_SETCHECK, Checked ? BST_CHECKED : BST_UNCHECKED);
    }

    bool RadioButton::IsChecked() const {
        return Send(BM_GETCHECK) == BST_CHECKED;
    }

    void RadioButton::OnCommand(WORD Code) {
        if (Code == BN_CLICKED && _SelectHandler && IsChecked()) { _SelectHandler(); }
    }
}  // namespace WinApp
