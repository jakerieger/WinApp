#include "Font.hpp"

#include <utility>

namespace WinApp {
    Font::Font(Font&& Other) noexcept : _Handle(std::exchange(Other._Handle, nullptr)) {}

    Font& Font::operator=(Font&& Other) noexcept {
        if (this != &Other) {
            Reset();
            _Handle = std::exchange(Other._Handle, nullptr);
        }
        return *this;
    }

    Font Font::System(UINT Dpi) {
        NONCLIENTMETRICSW Metrics {};
        Metrics.cbSize = sizeof(Metrics);
        if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(Metrics), &Metrics, 0, Dpi)) {
            return Font(CreateFontIndirectW(&Metrics.lfMessageFont));
        }

        LOGFONTW Fallback {};
        Fallback.lfHeight  = -MulDiv(9, static_cast<int>(Dpi), 72);
        Fallback.lfWeight  = FW_NORMAL;
        Fallback.lfCharSet = DEFAULT_CHARSET;
        Fallback.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(Fallback.lfFaceName, L"Segoe UI");
        return Font(CreateFontIndirectW(&Fallback));
    }

    void Font::Reset() {
        if (_Handle) {
            DeleteObject(_Handle);
            _Handle = nullptr;
        }
    }
}  // namespace WinApp
