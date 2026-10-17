#include "ProgressBar.hpp"

namespace WinApp {
    ProgressBar::ProgressBar(const Rect& Bounds, int Min, int Max)
        : Control({}, Bounds), _InitialMin(Min), _InitialMax(Max) {}

    Control::CreateInfo ProgressBar::GetCreateInfo() const {
        return {.ClassName = PROGRESS_CLASSW};
    }

    void ProgressBar::OnCreated() {
        SetRange(_InitialMin, _InitialMax);
    }

    void ProgressBar::SetRange(int Min, int Max) {
        Send(PBM_SETRANGE32, static_cast<WPARAM>(Min), Max);
    }

    void ProgressBar::SetValue(int Value) {
        Send(PBM_SETPOS, static_cast<WPARAM>(Value));
    }

    int ProgressBar::GetValue() const {
        return static_cast<int>(Send(PBM_GETPOS));
    }

    void ProgressBar::SetState(ProgressState State) {
        WPARAM Native = PBST_NORMAL;
        if (State == ProgressState::Paused) { Native = PBST_PAUSED; }
        if (State == ProgressState::Error) { Native = PBST_ERROR; }
        Send(PBM_SETSTATE, Native);
    }

    void ProgressBar::SetMarquee(bool Enabled) {
        // PBS_MARQUEE has to be present before PBM_SETMARQUEE has any effect.
        auto Style = GetWindowLongPtrW(GetHandle(), GWL_STYLE);
        Style      = Enabled ? (Style | PBS_MARQUEE) : (Style & ~static_cast<LONG_PTR>(PBS_MARQUEE));
        SetWindowLongPtrW(GetHandle(), GWL_STYLE, Style);
        Send(PBM_SETMARQUEE, Enabled, 0);
    }
}  // namespace WinApp
