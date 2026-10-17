#pragma once

#include "../Control.hpp"

#include <functional>

namespace WinApp {
    struct TextBoxOptions {
        std::wstring Text;
        /// Hint shown while the box is empty (single-line boxes only).
        std::wstring Placeholder;
        bool Multiline = false;
        bool Password  = false;
        bool ReadOnly  = false;
    };

    /// Editable text field. Multi-line boxes use "\r\n" line endings.
    class TextBox : public Control {
    public:
        explicit TextBox(const Rect& Bounds, TextBoxOptions Options = {});

        void SetPlaceholder(const std::wstring& Placeholder);
        void SetReadOnly(bool ReadOnly);
        [[nodiscard]] bool IsReadOnly() const;
        /// Limits the number of characters the user can type (0 = default system limit).
        void SetMaxLength(int Length);

        /// Appends text at the end and scrolls it into view.
        void AppendText(const std::wstring& Text);
        void SelectAll();

        /// Called whenever the text changes (including programmatic changes).
        void OnChange(std::function<void()> Handler) { _ChangeHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCreated() override;
        void OnCommand(WORD Code) override;

    private:
        TextBoxOptions _Options;
        std::function<void()> _ChangeHandler;
    };
}  // namespace WinApp
