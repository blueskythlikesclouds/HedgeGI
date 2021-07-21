#include "FileDialog.h"

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

std::string FileDialog::openFolder(LPCWSTR title)
{
	WCHAR fileName[1024] {};
	wcscpy(fileName, L"Enter into a directory and press Save");

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER
		| OFN_HIDEREADONLY | OFN_ENABLESIZING;

	if (GetSaveFileName(&ofn))
		return getDirectoryPath(wideCharToMultiByte(fileName));

	return "";
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
