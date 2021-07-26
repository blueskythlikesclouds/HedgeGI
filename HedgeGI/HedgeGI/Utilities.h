#pragma once

static std::string getDirectoryPath(const std::string& path)
{
    const size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        return path.substr(0, index);

    return std::string();
}

static std::string getFileName(const std::string& path)
{
    const size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        return path.substr(index + 1, path.size() - index - 1);

    return path;
}

static std::string getFileNameWithoutExtension(const std::string& path)
{
    std::string fileName;

    size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        fileName = path.substr(index + 1, path.size() - index - 1);
    else
        fileName = path;

    index = fileName.find_last_of('.');
    if (index != std::string::npos)
        fileName = fileName.substr(0, index);

    return fileName;
}

static std::string getExecutableDirectoryPath()
{
    WCHAR moduleFilePathWideChar[1024];
    GetModuleFileNameW(NULL, moduleFilePathWideChar, 1024);

    CHAR moduleFilePathMultiByte[1024];
    WideCharToMultiByte(CP_UTF8, 0, moduleFilePathWideChar, -1, moduleFilePathMultiByte, 1024, 0, 0);

    return getDirectoryPath(moduleFilePathMultiByte);
}

constexpr uint64_t strHash(const char* const value)
{
    uint64_t hash = 5381;

    for (size_t i = 0; ; i++)
    {
        const char c = value[i];
        if (!c)
            break;

        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

inline std::string wideCharToMultiByte(LPCWSTR value)
{
    char multiByte[1024];
    WideCharToMultiByte(CP_UTF8, 0, value, -1, multiByte, _countof(multiByte), 0, 0);
    return std::string(multiByte);
}

inline void alert()
{
    FLASHWINFO flashInfo;
    flashInfo.cbSize = sizeof(FLASHWINFO);
    flashInfo.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    flashInfo.uCount = 5;
    flashInfo.dwTimeout = 0;
    flashInfo.hwnd = GetConsoleWindow();
    FlashWindowEx(&flashInfo);
    MessageBeep(MB_OK);
}