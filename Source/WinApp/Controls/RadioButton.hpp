#pragma once

#include "../Control.hpp"

#include <functional>

namespace WinApp {
    /// Radio buttons added consecutively form one mutually-exclusive group. The group ends at the next non-radio
    /// control, or at a radio button created with StartsGroup = true.
    class RadioButton : public Control {
    public:
        RadioButton(std::wstring Text, const Rect& Bounds, bool StartsGroup = false);

        void SetChecked(bool Checked);
        [[nodiscard]] bool IsChecked() const;

        /// Called when the user selects this option.
        void OnSelect(std::function<void()> Handler) { _SelectHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCommand(WORD Code) override;

    private:
        bool _StartsGroup;
        std::function<void()> _SelectHandler;
    };
}  // namespace WinApp
