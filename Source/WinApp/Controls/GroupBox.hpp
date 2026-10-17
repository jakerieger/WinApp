#pragma once

#include "../Control.hpp"

namespace WinApp {
    /// A captioned frame for visually grouping related controls. Add it before the controls it contains.
    class GroupBox : public Control {
    public:
        GroupBox(std::wstring Text, const Rect& Bounds);

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
    };
}  // namespace WinApp
