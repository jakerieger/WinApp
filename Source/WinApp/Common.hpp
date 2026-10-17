#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <Windows.h>
#include <CommCtrl.h>

// Opt every executable that links WinApp into Common Controls v6 (themed, modern controls) without requiring the
// consumer to ship their own manifest. The linker merges this into the generated application manifest.
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,                                                                                                \
                "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' "        \
                "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace WinApp {
    /// A rectangle in device-independent pixels (96 DPI). WinApp scales it to the window's actual DPI.
    struct Rect {
        int X      = 0;
        int Y      = 0;
        int Width  = 0;
        int Height = 0;
    };

    /// A size in device-independent pixels (96 DPI).
    struct Size {
        int Width  = 0;
        int Height = 0;
    };
}  // namespace WinApp
