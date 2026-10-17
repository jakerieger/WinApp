add_library(WinApp STATIC
        WinApp.hpp
        Common.hpp
        App.cpp
        App.hpp
        Control.cpp
        Control.hpp
        FileDialog.cpp
        FileDialog.hpp
        Font.cpp
        Font.hpp
        Menu.cpp
        Menu.hpp
        Controls/Button.cpp
        Controls/Button.hpp
        Controls/CheckBox.cpp
        Controls/CheckBox.hpp
        Controls/ComboBox.cpp
        Controls/ComboBox.hpp
        Controls/GroupBox.cpp
        Controls/GroupBox.hpp
        Controls/Label.cpp
        Controls/Label.hpp
        Controls/ListBox.cpp
        Controls/ListBox.hpp
        Controls/ProgressBar.cpp
        Controls/ProgressBar.hpp
        Controls/RadioButton.cpp
        Controls/RadioButton.hpp
        Controls/Slider.cpp
        Controls/Slider.hpp
        Controls/TextBox.cpp
        Controls/TextBox.hpp
)

target_include_directories(WinApp PUBLIC ${PROJECT_SOURCE_DIR}/Source)

# Public so consumers see Windows.h configured exactly the way the library was built.
target_compile_definitions(WinApp
        PUBLIC
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(WinApp PUBLIC comctl32 ole32 shell32)

if (MSVC)
    target_compile_options(WinApp PRIVATE /W4 /permissive- /utf-8)
endif ()
