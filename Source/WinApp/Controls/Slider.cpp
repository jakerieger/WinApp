#include "Slider.hpp"

namespace WinApp {
    Slider::Slider(const Rect& Bounds, int Min, int Max, int Value)
        : Control({}, Bounds), _InitialMin(Min), _InitialMax(Max), _LastValue(Value) {}

    Control::CreateInfo Slider::GetCreateInfo() const {
        return {.ClassName = TRACKBAR_CLASSW, .Style = TBS_HORZ | TBS_NOTICKS | WS_TABSTOP};
    }

    void Slider::OnCreated() {
        const int InitialValue = _LastValue;  // SetRange overwrites _LastValue
        SetRange(_InitialMin, _InitialMax);
        SetValue(InitialValue);
    }

    void Slider::SetRange(int Min, int Max) {
        Send(TBM_SETRANGEMIN, FALSE, Min);
        Send(TBM_SETRANGEMAX, TRUE, Max);
        _LastValue = GetValue();
    }

    void Slider::SetValue(int Value) {
        Send(TBM_SETPOS, TRUE, Value);
        _LastValue = GetValue();
    }

    int Slider::GetValue() const {
        return static_cast<int>(Send(TBM_GETPOS));
    }

    void Slider::OnScroll(WORD /*Code*/) {
        // Track bars send several scroll codes per interaction; only report actual changes.
        const int Value = GetValue();
        if (Value == _LastValue) { return; }
        _LastValue = Value;
        if (_ValueHandler) { _ValueHandler(Value); }
    }
}  // namespace WinApp
