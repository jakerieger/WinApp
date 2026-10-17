# WinApp

WinApp is a C++20 library for writing modern native Windows applications with the Win32 (and associated) APIs. It
provides simple abstractions for creating windows, controls, dialogs, and more.

## Example

The code below can also be found in the [Demo](Source/Demo/main.cpp) project.

```c++
    // Main application instance
    App App({.Title = L"WinApp Demo", .Width = 480, .Height = 440, .MinWidth = 480, .MinHeight = 360});

    // Adding some controls to the application's window
    App.Add<Label>(L"Name:", Rect {16, 19, 56, 20});
    auto& Name  = App.Add<TextBox>(Rect {72, 16, 280, 23}, TextBoxOptions {.Placeholder = L"Type your name"});
    auto& Greet = App.Add<Button>(L"Greet", Rect {364, 15, 100, 25});
    
    // Handling events (button presses, changes in value, etc.)
    Greet.OnClick([&] {
        const auto Who = Name.GetText();
        App.ShowMessage(Who.empty() ? L"Hello, stranger!" : L"Hello, " + Who + L"!", L"Greetings from WinApp.");
    });
    Name.OnChange([&] { Greet.SetEnabled(!Name.GetText().empty()); });
```