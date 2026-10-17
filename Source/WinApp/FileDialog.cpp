#include "FileDialog.hpp"
#include "App.hpp"

#include <ShObjIdl.h>
#include <wrl/client.h>

#include <system_error>

namespace WinApp {
    namespace {
        using Microsoft::WRL::ComPtr;
        using std::filesystem::path;

        void Check(HRESULT Result, const char* What) {
            if (FAILED(Result)) { throw std::system_error(Result, std::system_category(), What); }
        }

        path GetPath(IShellItem* Item) {
            PWSTR Raw = nullptr;
            Check(Item->GetDisplayName(SIGDN_FILESYSPATH, &Raw), "IShellItem::GetDisplayName");
            path Result(Raw);
            CoTaskMemFree(Raw);
            return Result;
        }

        /// "*.txt;*.log" -> "txt". Empty if the first pattern has no concrete extension (e.g. "*.*").
        std::wstring ExtensionFromPattern(const std::wstring& Pattern) {
            const auto First = Pattern.substr(0, Pattern.find(L';'));
            if (!First.starts_with(L"*.")) { return {}; }
            auto Extension = First.substr(2);
            if (Extension.find_first_of(L"*?") != std::wstring::npos) { return {}; }
            return Extension;
        }

        template<class T>
        ComPtr<T> CreateFileDialog(REFCLSID Class) {
            ComPtr<T> Dialog;
            Check(CoCreateInstance(Class, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog)), "CoCreateInstance");
            return Dialog;
        }

        void Configure(IFileDialog* Dialog,
                       const FileDialogOptions& Options,
                       FILEOPENDIALOGOPTIONS ExtraFlags,
                       bool UseFilters) {
            FILEOPENDIALOGOPTIONS Flags = 0;
            Check(Dialog->GetOptions(&Flags), "IFileDialog::GetOptions");
            Check(Dialog->SetOptions(Flags | FOS_FORCEFILESYSTEM | ExtraFlags), "IFileDialog::SetOptions");

            if (!Options.Title.empty()) { Check(Dialog->SetTitle(Options.Title.c_str()), "IFileDialog::SetTitle"); }

            if (UseFilters && !Options.Filters.empty()) {
                std::vector<COMDLG_FILTERSPEC> Specs;
                Specs.reserve(Options.Filters.size());
                for (const auto& Filter : Options.Filters) { Specs.push_back({Filter.Name.c_str(), Filter.Pattern.c_str()}); }
                Check(Dialog->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data()), "IFileDialog::SetFileTypes");
                Check(Dialog->SetFileTypeIndex(1), "IFileDialog::SetFileTypeIndex");
            }

            if (!Options.InitialFolder.empty()) {
                // A folder that doesn't exist is not worth failing over; the dialog just opens in its default spot.
                ComPtr<IShellItem> Folder;
                if (SUCCEEDED(SHCreateItemFromParsingName(Options.InitialFolder.c_str(), nullptr, IID_PPV_ARGS(&Folder)))) {
                    Dialog->SetFolder(Folder.Get());
                }
            }

            if (!Options.FileName.empty()) {
                Check(Dialog->SetFileName(Options.FileName.c_str()), "IFileDialog::SetFileName");
            }
        }

        /// Returns false if the user cancelled.
        bool Show(IFileDialog* Dialog, const App& Owner) {
            const HRESULT Result = Dialog->Show(Owner.GetHandle());
            if (Result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) { return false; }
            Check(Result, "IFileDialog::Show");
            return true;
        }

        std::optional<path> ShowOpenSingle(const App& Owner, const FileDialogOptions& Options, bool Folders) {
            auto Dialog = CreateFileDialog<IFileOpenDialog>(__uuidof(FileOpenDialog));
            Configure(Dialog.Get(), Options, Folders ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST, !Folders);
            if (!Show(Dialog.Get(), Owner)) { return std::nullopt; }

            ComPtr<IShellItem> Item;
            Check(Dialog->GetResult(&Item), "IFileDialog::GetResult");
            return GetPath(Item.Get());
        }

        std::vector<path> ShowOpenMultiple(const App& Owner, const FileDialogOptions& Options, bool Folders) {
            auto Dialog = CreateFileDialog<IFileOpenDialog>(__uuidof(FileOpenDialog));
            Configure(Dialog.Get(),
                      Options,
                      FOS_ALLOWMULTISELECT | (Folders ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST),
                      !Folders);
            if (!Show(Dialog.Get(), Owner)) { return {}; }

            ComPtr<IShellItemArray> Items;
            Check(Dialog->GetResults(&Items), "IFileOpenDialog::GetResults");
            DWORD Count = 0;
            Check(Items->GetCount(&Count), "IShellItemArray::GetCount");

            std::vector<path> Paths;
            Paths.reserve(Count);
            for (DWORD Index = 0; Index < Count; ++Index) {
                ComPtr<IShellItem> Item;
                Check(Items->GetItemAt(Index, &Item), "IShellItemArray::GetItemAt");
                Paths.push_back(GetPath(Item.Get()));
            }
            return Paths;
        }
    }  // namespace

    std::optional<path> FileDialog::OpenFile(const App& Owner, const FileDialogOptions& Options) {
        return ShowOpenSingle(Owner, Options, false);
    }

    std::vector<path> FileDialog::OpenFiles(const App& Owner, const FileDialogOptions& Options) {
        return ShowOpenMultiple(Owner, Options, false);
    }

    std::optional<path> FileDialog::PickFolder(const App& Owner, const FileDialogOptions& Options) {
        return ShowOpenSingle(Owner, Options, true);
    }

    std::vector<path> FileDialog::PickFolders(const App& Owner, const FileDialogOptions& Options) {
        return ShowOpenMultiple(Owner, Options, true);
    }

    std::optional<path> FileDialog::SaveFile(const App& Owner, const FileDialogOptions& Options) {
        auto Dialog = CreateFileDialog<IFileSaveDialog>(__uuidof(FileSaveDialog));
        Configure(Dialog.Get(), Options, FOS_OVERWRITEPROMPT, true);

        // Once a default extension is set, the dialog also switches it to match whichever filter the user picks.
        auto Extension = Options.DefaultExtension;
        if (Extension.empty() && !Options.Filters.empty()) { Extension = ExtensionFromPattern(Options.Filters.front().Pattern); }
        if (!Extension.empty()) { Check(Dialog->SetDefaultExtension(Extension.c_str()), "IFileDialog::SetDefaultExtension"); }

        if (!Show(Dialog.Get(), Owner)) { return std::nullopt; }

        ComPtr<IShellItem> Item;
        Check(Dialog->GetResult(&Item), "IFileDialog::GetResult");
        return GetPath(Item.Get());
    }
}  // namespace WinApp
