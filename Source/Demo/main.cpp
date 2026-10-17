#include <WinApp/WinApp.hpp>

#include <chrono>
#include <thread>

using namespace WinApp;

APP_ENTRY(HINSTANCE, HINSTANCE, PWSTR, int) {
    App App({.Title = L"WinApp Demo", .Width = 480, .Height = 440, .MinWidth = 480, .MinHeight = 360});

    App.Add<Label>(L"Name:", Rect {16, 19, 56, 20});
    auto& Name  = App.Add<TextBox>(Rect {72, 16, 280, 23}, TextBoxOptions {.Placeholder = L"Type your name"});
    auto& Greet = App.Add<Button>(L"Greet", Rect {364, 15, 100, 25});

    App.Add<GroupBox>(L"Options", Rect {16, 52, 216, 104});
    auto& Notify = App.Add<CheckBox>(L"Enable notifications", Rect {28, 74, 190, 20}, true);
    auto& Light  = App.Add<RadioButton>(L"Light theme", Rect {28, 100, 190, 20}, true);
    auto& Dark   = App.Add<RadioButton>(L"Dark theme", Rect {28, 124, 190, 20});
    Light.SetChecked(true);

    App.Add<Label>(L"Favorite color:", Rect {248, 58, 216, 20});
    auto& Color = App.Add<ComboBox>(Rect {248, 80, 216, 23});
    Color.AddItems({L"Red", L"Green", L"Blue", L"Purple"});
    Color.SetSelectedIndex(2);

    auto& Fruits = App.Add<ListBox>(Rect {248, 112, 216, 80});
    Fruits.AddItems({L"Apple", L"Banana", L"Cherry", L"Grape", L"Mango", L"Orange"});

    auto& VolumeLabel = App.Add<Label>(L"Volume: 40", Rect {16, 168, 216, 20});
    auto& Volume      = App.Add<Slider>(Rect {12, 188, 224, 30}, 0, 100, 40);

    auto& Progress = App.Add<ProgressBar>(Rect {16, 228, 448, 16});
    Progress.SetValue(40);

    // Long paths lose their middle rather than their file name.
    auto& Current = App.Add<Label>(L"No file opened", Rect {16, 252, 448, 20}, TextAlign::Left, TextOverflow::Path);

    auto& Log  = App.Add<TextBox>(Rect {16, 276, 448, 148}, TextBoxOptions {.Multiline = true, .ReadOnly = true});
    auto Print = [&Log](const std::wstring& Line) { Log.AppendText(Line + L"\r\n"); };

    Greet.OnClick([&] {
        const auto Who = Name.GetText();
        App.ShowMessage(Who.empty() ? L"Hello, stranger!" : L"Hello, " + Who + L"!", L"Greetings from WinApp.");
    });
    Name.OnChange([&] { Greet.SetEnabled(!Name.GetText().empty()); });
    Greet.SetEnabled(false);

    Notify.OnToggle([&](bool On) { Print(On ? L"Notifications enabled" : L"Notifications disabled"); });
    Light.OnSelect([&] { Print(L"Light theme selected"); });
    Dark.OnSelect([&] { Print(L"Dark theme selected"); });
    Color.OnSelectionChange([&](int Index) { Print(L"Color: " + Color.GetItem(Index)); });
    Fruits.OnSelectionChange([&](int Index) { Print(L"Fruit: " + Fruits.GetItem(Index)); });
    Fruits.OnActivate([&](int Index) { App.ShowMessage(L"You picked " + Fruits.GetItem(Index)); });
    Volume.OnValueChange([&](int Value) {
        VolumeLabel.SetText(L"Volume: " + std::to_wstring(Value));
        Progress.SetValue(Value);
        Progress.SetState(Value > 90 ? ProgressState::Error : ProgressState::Normal);
    });

    // Menu bar --------------------------------------------------------------------------------------------------------

    const std::vector<FileFilter> TextFilters {{L"Text files", L"*.txt;*.log"}, {L"All files", L"*.*"}};
    std::vector<std::wstring> RecentFiles;

    auto& File   = App.GetMenuBar().AddSubmenu(L"&File");
    auto AddFile = [&](const std::filesystem::path& Path) {
        Print(L"Opened " + Path.wstring());
        Current.SetText(Path.wstring());
        RecentFiles.push_back(Path.wstring());
    };

    File.AddItem(L"&Open...", {Modifiers::Ctrl, 'O'}, [&] {
        if (auto Path = FileDialog::OpenFile(App, {.Filters = TextFilters})) { AddFile(*Path); }
    });
    File.AddItem(L"Open &Multiple...", [&] {
        for (const auto& Path : FileDialog::OpenFiles(App, {.Title = L"Open several files"})) {
            AddFile(Path);
        }
    });
    File.AddItem(L"Save &As...", {Modifiers::Ctrl | Modifiers::Shift, 'S'}, [&] {
        if (auto Path = FileDialog::SaveFile(App, {.Filters = TextFilters, .FileName = L"Untitled"})) {
            Print(L"Save to " + Path->wstring());
        }
    });
    File.AddItem(L"Choose &Folder...", [&] {
        if (auto Folder = FileDialog::PickFolder(App)) { Print(L"Folder " + Folder->wstring()); }
    });
    File.AddSeparator();

    // Rebuilt every time it opens, to show dynamic menus.
    auto& Recent = File.AddSubmenu(L"&Recent Files");
    Recent.OnOpen([&] {
        Recent.Clear();
        if (RecentFiles.empty()) { Recent.AddItem(L"(none)", nullptr).SetEnabled(false); }
        for (const auto& Path : RecentFiles) {
            Recent.AddItem(Path, [&Print, Path] { Print(L"Reopen " + Path); });
        }
    });
    File.AddSeparator();
    File.AddItem(L"E&xit", [&] { App.Close(); });

    auto& View = App.GetMenuBar().AddSubmenu(L"&View");
    View.AddCheckItem(L"Show &Log", {Modifiers::Ctrl, 'L'}, true, [&](bool Show) { Log.SetVisible(Show); });
    View.AddItem(L"&Clear Log", {Modifiers::Ctrl | Modifiers::Shift, VK_DELETE}, [&] { Log.SetText(L""); });

    // Declared after App so it is joined before the App (and the controls it touches) go away.
    std::jthread Worker;

    auto& Tools = App.GetMenuBar().AddSubmenu(L"&Tools");
    Tools.AddItem(L"Run &Background Task", {Modifiers::Ctrl, 'B'}, [&] {
        Print(L"Background task started");
        // Controls may only be touched on the UI thread, so the worker hands every update to App.Post().
        Worker = std::jthread([&](const std::stop_token& Stop) {
            for (int Step = 0; Step <= 100 && !Stop.stop_requested(); Step += 5) {
                App.Post([&Progress, Step] { Progress.SetValue(Step); });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            App.Post([Print] { Print(L"Background task finished"); });
        });
    });

    auto& Help = App.GetMenuBar().AddSubmenu(L"&Help");
    Help.AddItem(L"&About", {Modifiers::None, VK_F1}, [&] {
        App.ShowMessage(L"WinApp Demo", L"Native Win32 with Common Controls v6.");
    });

    // Context menus
    // -----------------------------------------------------------------------------------------------------

    Menu FruitMenu;
    auto& RemoveFruit = FruitMenu.AddItem(L"&Remove", [&] { Fruits.RemoveItem(Fruits.GetSelectedIndex()); });
    FruitMenu.AddItem(L"&Add Kiwi", [&] { Fruits.AddItem(L"Kiwi"); });
    FruitMenu.AddSeparator();
    FruitMenu.AddItem(L"&Clear", [&] { Fruits.Clear(); });
    FruitMenu.OnOpen([&] { RemoveFruit.SetEnabled(Fruits.GetSelectedIndex() >= 0); });
    Fruits.SetContextMenu(&FruitMenu);

    Menu WindowMenu;
    WindowMenu.AddItem(L"Reset &volume", [&] {
        Volume.SetValue(40);
        VolumeLabel.SetText(L"Volume: 40");
        Progress.SetValue(40);
    });
    App.SetContextMenu(&WindowMenu);

    // Keep the log filling the bottom of the window as it resizes.
    App.OnResize([&](Size Client) {
        const auto& Bounds = Log.GetBounds();
        Log.SetBounds({Bounds.X, Bounds.Y, Client.Width - 32, Client.Height - Bounds.Y - 16});
    });
    App.OnClose([&] { return App.Confirm(L"Quit WinApp Demo?"); });

    return App.Run();
}
