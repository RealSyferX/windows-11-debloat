#include "Utils.h"
#include <cstdio>
#include <iostream>
#include <cctype>
#include <shellapi.h>

namespace Utils {

bool IsElevated() {
    BOOL ret = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elev;
        DWORD sz = sizeof(elev);
        if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &sz))
            ret = elev.TokenIsElevated;
        CloseHandle(hToken);
    }
    return ret != FALSE;
}

bool RelaunchAsAdmin() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0)
        return false;
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.nShow  = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei) != FALSE;
}

std::wstring StringToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}

std::string WideToString(const std::wstring& ws) {
    if (ws.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], len, NULL, NULL);
    return s;
}

std::string RunPowerShell(const std::wstring& script) {
    wchar_t tempDir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0)
        return "";

    std::wstring ps1 = std::wstring(tempDir) + L"debloat_" +
        std::to_wstring(GetTickCount64()) + L".ps1";

    HANDLE hFile = CreateFileW(ps1.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return "";

    DWORD wr = 0;
    const BYTE bom[2] = { 0xFF, 0xFE };
    WriteFile(hFile, bom, 2, &wr, NULL);
    WriteFile(hFile, script.c_str(),
        static_cast<DWORD>(script.size() * sizeof(wchar_t)), &wr, NULL);
    CloseHandle(hFile);

    std::wstring cmd = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \""
        + ps1 + L"\" 2>&1";

    FILE* pipe = _wpopen(cmd.c_str(), L"r");
    std::string result;
    if (pipe) {
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe))
            result += buf;
        _pclose(pipe);
    }

    DeleteFileW(ps1.c_str());
    return result;
}

void SetColor(WORD attr) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

static const WORD CLR_DEFAULT = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

void PrintInfo(const std::string& msg)    { SetColor(CLR_DEFAULT);                                std::cout << msg << "\n"; SetColor(CLR_DEFAULT); }
void PrintSuccess(const std::string& msg) { SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);    std::cout << msg << "\n"; SetColor(CLR_DEFAULT); }
void PrintError(const std::string& msg)   { SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);     std::cout << msg << "\n"; SetColor(CLR_DEFAULT); }
void PrintWarning(const std::string& msg) { SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY); std::cout << msg << "\n"; SetColor(CLR_DEFAULT); }
void PrintHeader(const std::string& msg)  { SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY); std::cout << "\n" << msg << "\n" << std::string(msg.size(), '-') << "\n"; SetColor(CLR_DEFAULT); }

bool AskYesNo(const std::string& prompt) {
    std::cout << prompt << " (Y/n): ";
    std::string in;
    std::getline(std::cin, in);
    if (in.empty()) return true;
    return std::tolower(static_cast<unsigned char>(in[0])) == 'y';
}

void CreateRestorePoint() {
    std::string out = RunPowerShell(
        L"Enable-ComputerRestore -Drive 'C:\\' -ErrorAction SilentlyContinue; "
        L"Checkpoint-Computer -Description 'Before Debloat' "
        L"-RestorePointType 'MODIFY_SETTINGS' -ErrorAction SilentlyContinue; "
        L"Write-Host '  Restore point created (or already exists).'"
    );
    if (!out.empty()) std::cout << out;
}

} // namespace Utils
