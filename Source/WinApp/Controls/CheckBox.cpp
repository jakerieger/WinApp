#include "CheckBox.hpp"

namespace WinApp {
    CheckBox::CheckBox(std::wstring Text, const Rect& Bounds, bool Checked)
        : Control(std::move(Text), Bounds), _InitiallyChecked(Checked) {}

    Control::CreateInfo CheckBox::GetCreateInfo() const {
        return {.ClassName = WC_BUTTONW, .Style = BS_AUTOCHECKBOX | WS_TABSTOP};
    }

    void CheckBox::OnCreated() {
        SetChecked(_InitiallyChecked);
    }

    void CheckBox::SetChecked(bool Checked) {
        Send(BM_SETCHECK, Checked ? BST_CHECKED : BST_UNCHECKED);
    }

    bool CheckBox::IsChecked() const {
        return Send(BM_GETCHECK) == BST_CHECKED;
    }

    void CheckBox::OnCommand(WORD Code) {
        if (Code == BN_CLICKED && _ToggleHandler) { _ToggleHandler(IsChecked()); }
    }
}  // namespace WinApp
