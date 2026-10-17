#pragma once

#include "../Control.hpp"

#include <functional>
#include <initializer_list>

namespace WinApp {
    /// Scrollable single-selection list.
    class ListBox : public Control {
    public:
        explicit ListBox(const Rect& Bounds);

        int AddItem(const std::wstring& Text);
        void AddItems(std::initializer_list<std::wstring> Items);
        void RemoveItem(int Index);
        void Clear();
        [[nodiscard]] int GetCount() const;
        [[nodiscard]] std::wstring GetItem(int Index) const;

        /// Index of the selected item, or -1 if nothing is selected.
        [[nodiscard]] int GetSelectedIndex() const;
        /// Selects an item (-1 clears the selection). Does not raise OnSelectionChange.
        void SetSelectedIndex(int Index);

        /// Called when the user selects a different item, with its index.
        void OnSelectionChange(std::function<void(int)> Handler) { _SelectionHandler = std::move(Handler); }
        /// Called when the user double-clicks an item, with its index.
        void OnActivate(std::function<void(int)> Handler) { _ActivateHandler = std::move(Handler); }

    protected:
        [[nodiscard]] CreateInfo GetCreateInfo() const override;
        void OnCommand(WORD Code) override;

    private:
        std::function<void(int)> _SelectionHandler;
        std::function<void(int)> _ActivateHandler;
    };
}  // namespace WinApp
