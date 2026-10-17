#pragma once

#include "../Control.hpp"

namespace WinApp {
    enum class TextAlign { Left, Center, Right };

    /// Static, non-interactive text.
    class Label : public Control {
    public:
        Label(std::wstring Text, const Rect& Bounds, TextAlign Align = TextAlign::Left);

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;

    private:
        TextAlign _Align;
    };
}  // namespace WinApp
