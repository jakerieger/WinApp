#pragma once

#include "../Control.hpp"

#include <functional>

namespace WinApp {
    /// Horizontal track bar for picking an integer within a range.
    class Slider : public Control {
    public:
        explicit Slider(const Rect& Bounds, int Min = 0, int Max = 100, int Value = 0);

        void SetRange(int Min, int Max);
        /// Does not raise OnValueChange.
        void SetValue(int Value);
        [[nodiscard]] int GetValue() const;

        /// Called while the user drags or steps the slider, with the new value.
        void OnValueChange(std::function<void(int)> Handler) { _ValueHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCreated() override;
        void OnScroll(WORD Code) override;

    private:
        int _InitialMin;
        int _InitialMax;
        int _LastValue;
        std::function<void(int)> _ValueHandler;
    };
}  // namespace WinApp
