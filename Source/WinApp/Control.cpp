#include "Control.hpp"
#include "App.hpp"

#include <system_error>

namespace WinApp {
    Control::Control(std::wstring Text, const Rect& Bounds) : _Bounds(Bounds), _InitialText(std::move(Text)) {}

    Control::~Control() {
        if (_Handle) { DestroyWindow(_Handle); }
    }

    void Control::Create(App& Owner, int Id) {
        _Owner          = &Owner;
        const auto Info = GetCreateInfo();
        const DWORD Style = WS_CHILD | WS_VISIBLE | (Info.StartsGroup ? WS_GROUP : 0) | Info.Style;

        _Handle = CreateWindowExW(Info.ExStyle,
                                  Info.ClassName,
                                  _InitialText.c_str(),
                                  Style,
                                  Owner.Scale(_Bounds.X),
                                  Owner.Scale(_Bounds.Y),
                                  Owner.Scale(_Bounds.Width),
                                  Owner.Scale(_Bounds.Height),
                                  Owner.GetHandle(),
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(Id)),
                                  GetModuleHandleW(nullptr),
                                  nullptr);
        if (!_Handle) { throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CreateWindowExW"); }

        ApplyFont(Owner.GetFont());
        _InitialText = {};
        OnCreated();
    }

    void Control::ApplyLayout() const {
        SetWindowPos(_Handle,
                     nullptr,
                     _Owner->Scale(_Bounds.X),
                     _Owner->Scale(_Bounds.Y),
                     _Owner->Scale(_Bounds.Width),
                     _Owner->Scale(_Bounds.Height),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void Control::ApplyFont(HFONT Font) const {
        Send(WM_SETFONT, reinterpret_cast<WPARAM>(Font), TRUE);
    }

    LRESULT Control::Send(UINT Message, WPARAM WParam, LPARAM LParam) const {
        return SendMessageW(_Handle, Message, WParam, LParam);
    }

    void Control::SetText(const std::wstring& Text) {
        SetWindowTextW(_Handle, Text.c_str());
    }

    std::wstring Control::GetText() const {
        std::wstring Text(static_cast<size_t>(GetWindowTextLengthW(_Handle)), L'\0');
        if (!Text.empty()) {
            const int Copied = GetWindowTextW(_Handle, Text.data(), static_cast<int>(Text.size()) + 1);
            Text.resize(static_cast<size_t>(Copied));
        }
        return Text;
    }

    void Control::SetBounds(const Rect& Bounds) {
        _Bounds = Bounds;
        ApplyLayout();
    }

    void Control::SetEnabled(bool Enabled) {
        EnableWindow(_Handle, Enabled);
    }

    bool Control::IsEnabled() const {
        return IsWindowEnabled(_Handle) != FALSE;
    }

    void Control::SetVisible(bool Visible) {
        ShowWindow(_Handle, Visible ? SW_SHOW : SW_HIDE);
    }

    bool Control::IsVisible() const {
        return IsWindowVisible(_Handle) != FALSE;
    }

    void Control::Focus() {
        SetFocus(_Handle);
    }
}  // namespace WinApp
