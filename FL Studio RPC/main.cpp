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
// Made by cdn#0001 & melee#0001 

const char* APPLICATION_ID = "788422800238575659"; // DO NOT REPLACE THIS

bool check_process() {
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
        if (!wcscmp(pe32.szExeFile, L"FL64.exe")) {
            CloseHandle(hProcessSnap);
            return true;
        }
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
    MessageBox(NULL, L"FL Studio was not found!", L"Error", MB_ICONERROR | MB_OK);
    exit(0);
}


void update_presence(const std::wstring& process_name)
{
    auto current_time = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(current_time);

    while (true)
    {
        HWND hwnd = FindWindow(L"TFruityLoopsMainForm", NULL);
        if (hwnd != NULL)
        {
            wchar_t windowTitle[256];
            GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle));
            std::wstring title(windowTitle);

            size_t index = title.find(L" - FL Studio 20");
            if (index != std::wstring::npos)
            {
                title = title.substr(0, index);
            }

            int size = WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, NULL, 0, NULL, NULL);
            char* details = new char[size];
            WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, details, size, NULL, NULL);

            DiscordRichPresence presence;
            memset(&presence, 0, sizeof(presence));
            presence.details = details;
            presence.largeImageKey = "fl_logo";
            presence.largeImageText = "FL Studio 20";
            presence.instance = 1;
            presence.startTimestamp = start_time;
            Discord_UpdatePresence(&presence);
        }
        else
        {
            check_process();
            Discord_ClearPresence();
        }

        Sleep(1000);
    }
}

int main()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
    Discord_ClearPresence();

    const char* title = "FL Studio RPC";

    int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    LPCWSTR wTitle = new wchar_t[size];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, (LPWSTR)wTitle, size);

    SetConsoleTitle(wTitle);

    std::wstring process_name = L"FL64.exe";
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int console_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    int text_width = 18;
    int text_height = 1;
    int x = (console_width - text_width) / 2;
    int y = (console_height - text_height) / 2;

    COORD cursor_pos;
    cursor_pos.X = x;
    cursor_pos.Y = y;

    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursor_pos);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE);
    Sleep(1000);
    check_process();
    std::cout << "FL Studio was found!" << std::endl;
    Sleep(3000);
    system("cls");
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursor_pos);
    std::cout << "   Minimizing..  " << std::endl;
    Sleep(3000);
    FreeConsole();

    update_presence(process_name);

    while (true)
    {
        check_process();

        Discord_RunCallbacks();
        Sleep(1000);
    }
}