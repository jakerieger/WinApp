#pragma once

#include "Common.hpp"

namespace WinApp {
    /// Owning wrapper around an HFONT.
    class Font {
    public:
        Font() = default;
        explicit Font(HFONT Handle) : _Handle(Handle) {}
        ~Font() { Reset(); }

        Font(Font&& Other) noexcept;
        Font& operator=(Font&& Other) noexcept;
        Font(const Font&)            = delete;
        Font& operator=(const Font&) = delete;

        /// The system message font (Segoe UI 9pt on modern Windows), sized for the given DPI.
        static Font System(UINT Dpi);

        [[nodiscard]] HFONT GetHandle() const { return _Handle; }

    private:
        void Reset();

        HFONT _Handle = nullptr;
    };
}  // namespace WinApp
