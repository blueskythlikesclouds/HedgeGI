#include "FileDialog.h"
#include "Utilities.h"

#include <ShObjIdl.h>
#include <atlbase.h>

std::string FileDialog::openFile(LPCWSTR filter, LPCWSTR title)
{
    WCHAR fileName[1024] {};

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = 1024;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER
        | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
        return wideCharToMultiByte(fileName);

    return "";
}

std::string FileDialog::openFolder(const LPCWSTR title)
{
    CComPtr<IFileOpenDialog> folderDialog;
    folderDialog.CoCreateInstance(CLSID_FileOpenDialog);

    FILEOPENDIALOGOPTIONS options {};
    folderDialog->GetOptions(&options);
    folderDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);

    folderDialog->SetTitle(title);

    if (FAILED(folderDialog->Show(nullptr)))
        return {};

    CComPtr<IShellItem> result;
    if (FAILED(folderDialog->GetResult(&result)))
        return {};

    CComHeapPtr<wchar_t> path;
    if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        return {};

    return wideCharToMultiByte(path);
}

std::string FileDialog::saveFile(LPCWSTR filter, LPCWSTR title)
{
    WCHAR fileName[1024] {};

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = 1024;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER
        | OFN_HIDEREADONLY | OFN_ENABLESIZING;

    if (GetSaveFileName(&ofn))
        return wideCharToMultiByte(fileName);

    return "";
}
