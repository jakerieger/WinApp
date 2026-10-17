#pragma once

#include "../Control.hpp"

namespace WinApp {
    enum class ProgressState { Normal, Paused, Error };

    class ProgressBar : public Control {
    public:
        explicit ProgressBar(const Rect& Bounds, int Min = 0, int Max = 100);

        void SetRange(int Min, int Max);
        void SetValue(int Value);
        [[nodiscard]] int GetValue() const;

        /// Normal (green), Paused (yellow) or Error (red).
        void SetState(ProgressState State);
        /// Indeterminate "busy" animation instead of a value.
        void SetMarquee(bool Enabled);

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCreated() override;

    private:
        int _InitialMin;
        int _InitialMax;
    };
}  // namespace WinApp
