#include <Windows.h>
#include <chrono> 
#include "DiscordSDK/include/discord_rpc.h"
#include "DiscordSDK/include/discord_register.h"
#include <TlHelp32.h> 
#include <string>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <tchar.h>
#include <regex>

const char* APPLICATION_ID = "788422800238575659"; // Do not change this.

bool flRunning = false;

bool check_process(const std::wstring& process_name) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return false;
    }

    do {
        if (!wcscmp(pe32.szExeFile, process_name.c_str())) {
            CloseHandle(hProcessSnap);
            return true;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    return false;
}

bool flfocused() {
    return GetForegroundWindow() == FindWindow(L"TFruityLoopsMainForm", NULL);
}

void update_presence()
{
    auto current_time = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(current_time);

    std::wstring previous_project_title;

    while (true)
    {
        HWND hwnd = FindWindow(L"TFruityLoopsMainForm", NULL);
        if (hwnd != NULL)
        {
            wchar_t windowTitle[256];
            GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle));
            std::wstring title(windowTitle);

            std::wregex flStudioVersionRegex(L" - FL Studio (20|21)");
            title = std::regex_replace(title, flStudioVersionRegex, L"");

            int size = WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, NULL, 0, NULL, NULL);
            char* details = new char[size];
            WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, details, size, NULL, NULL);

            // Check if the project (flp) changed if so the time resets.
            if (title != previous_project_title) {
                start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                previous_project_title = title;
            }

            DiscordRichPresence presence;
            memset(&presence, 0, sizeof(presence));
            presence.details = details;
            presence.largeImageKey = "fl_logo";
            presence.largeImageText = "FL Studio";
            presence.smallImageKey = flfocused() ? nullptr : "idle";
            presence.smallImageText = "Idle";
            presence.instance = 1;

            if (flfocused()) {
                presence.startTimestamp = start_time;
            }

            Discord_UpdatePresence(&presence);
        }
        else
        {
            check_process(L"FL64.exe");
            check_process(L"FL32.exe");
            Discord_ClearPresence();

            start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            previous_project_title.clear();
        }

        Sleep(500);
    }
}

bool SetStartupRegistry(const std::wstring& appName, const std::wstring& exePath)
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        return false;
    }

    if (RegSetValueExW(hKey, appName.c_str(), 0, REG_SZ, (BYTE*)exePath.c_str(), (exePath.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

bool CheckStartupRegistry(const std::wstring& appName, std::wstring& exePath)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);

    if (RegQueryValueExW(hKey, appName.c_str(), NULL, NULL, (BYTE*)buffer, &bufferSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    exePath = buffer;

    RegCloseKey(hKey);
    return true;
}

int main()
{
    FreeConsole();
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
    Discord_ClearPresence();

    const char* title = "FLRPC";

    int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    LPWSTR wTitle = new wchar_t[size];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, (LPWSTR)wTitle, size);

    SetConsoleTitle(wTitle);
    delete[] wTitle;

    std::wstring FL64 = L"FL64.exe";
    std::wstring FL32 = L"FL32.exe";

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring appName = L"FLRPC";

    std::wstring startupExePath;
    if (!CheckStartupRegistry(appName, startupExePath))
    {
        if (!SetStartupRegistry(appName, exePath))
        {
            return 1;
        }
    }
    else if (startupExePath != exePath)
    {
        if (!SetStartupRegistry(appName, exePath))
        {
            return 1;
        }
    }

    std::time_t start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    while (true)
    {
        bool processRunning = check_process(FL64) || check_process(FL32);

        if (processRunning && !flRunning) {
            flRunning = true;
            update_presence();
        }
        else if (!processRunning && flRunning) {
            flRunning = false;
            Discord_ClearPresence();
            start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }

        Discord_RunCallbacks();
        Sleep(500);
    }

    return 0;
}
