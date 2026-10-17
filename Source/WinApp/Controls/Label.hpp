#pragma once

#include "../Control.hpp"

namespace WinApp {
    enum class TextAlign { Left, Center, Right };

    /// What happens to text that doesn't fit: Clip cuts it off, End replaces the end with "...", and Path replaces the
    /// middle with "..." so the tail of a file path stays visible. End and Path keep the text on a single line.
    enum class TextOverflow { Clip, End, Path };

    /// Static, non-interactive text.
    class Label : public Control {
    public:
        Label(std::wstring Text,
              const Rect& Bounds,
              TextAlign Align       = TextAlign::Left,
              TextOverflow Overflow = TextOverflow::Clip);

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;

    private:
        TextAlign _Align;
        TextOverflow _Overflow;
    };
}  // namespace WinApp
