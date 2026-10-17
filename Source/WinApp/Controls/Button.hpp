#pragma once

#include "../Control.hpp"

#include <functional>

namespace WinApp {
    class Button : public Control {
    public:
        Button(std::wstring Text, const Rect& Bounds);

        void OnClick(std::function<void()> Handler) { _ClickHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCommand(WORD Code) override;

    private:
        std::function<void()> _ClickHandler;
    };
}  // namespace WinApp
