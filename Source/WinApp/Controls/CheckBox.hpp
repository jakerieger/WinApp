#pragma once

#include "../Control.hpp"

#include <functional>

namespace WinApp {
    class CheckBox : public Control {
    public:
        CheckBox(std::wstring Text, const Rect& Bounds, bool Checked = false);

        void SetChecked(bool Checked);
        [[nodiscard]] bool IsChecked() const;

        /// Called when the user toggles the box, with the new checked state.
        void OnToggle(std::function<void(bool)> Handler) { _ToggleHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCreated() override;
        void OnCommand(WORD Code) override;

    private:
        bool _InitiallyChecked;
        std::function<void(bool)> _ToggleHandler;
    };
}  // namespace WinApp
