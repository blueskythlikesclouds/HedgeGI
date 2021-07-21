#pragma once

class FileDialog
{
public:
    static std::string openFile(LPCWSTR filter, LPCWSTR title);
    static std::string openFolder(LPCWSTR title);
    static std::string saveFile(LPCWSTR filter, LPCWSTR title);
};
