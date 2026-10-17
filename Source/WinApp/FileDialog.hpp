#pragma once

#include "Common.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace WinApp {
    class App;

    struct FileFilter {
        /// Shown in the file-type drop-down, e.g. L"Images".
        std::wstring Name;
        /// Semicolon-separated wildcard patterns, e.g. L"*.png;*.jpg".
        std::wstring Pattern;
    };

    struct FileDialogOptions {
        /// Window title (defaults to the system's "Open" / "Save As" / "Select Folder").
        std::wstring Title;
        /// File-type filters; the first one is selected initially. Ignored when picking folders.
        std::vector<FileFilter> Filters;
        /// Folder the dialog opens in. Defaults to the folder the user last used in this app.
        std::filesystem::path InitialFolder;
        /// Pre-filled file name.
        std::wstring FileName;
        /// Save only: extension (without the dot) appended when the user doesn't type one. Defaults to the extension
        /// of the first filter.
        std::wstring DefaultExtension;
    };

    /// Modern file and folder pickers (Common Item Dialog). Every function blocks until the user closes the dialog and
    /// returns an empty result if they cancel.
    class FileDialog {
    public:
        FileDialog() = delete;

        [[nodiscard]] static std::optional<std::filesystem::path> OpenFile(const App& Owner,
                                                                           const FileDialogOptions& Options = {});
        [[nodiscard]] static std::vector<std::filesystem::path> OpenFiles(const App& Owner,
                                                                          const FileDialogOptions& Options = {});
        [[nodiscard]] static std::optional<std::filesystem::path> SaveFile(const App& Owner,
                                                                           const FileDialogOptions& Options = {});
        [[nodiscard]] static std::optional<std::filesystem::path> PickFolder(const App& Owner,
                                                                             const FileDialogOptions& Options = {});
        [[nodiscard]] static std::vector<std::filesystem::path> PickFolders(const App& Owner,
                                                                            const FileDialogOptions& Options = {});
    };
}  // namespace WinApp
