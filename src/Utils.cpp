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

    // GetTempFileNameW with uUnique=0 atomically creates a zero-length file
    // with a unique, non-predictable name derived from the system time. This
    // replaces the predictable "debloat_<GetTickCount64>.ps1" scheme and
    // prevents an attacker who can write to %TEMP% from pre-placing a
    // symlink/hardlink to trick this elevated process into overwriting an
    // arbitrary file (privileged-write TOCTOU/symlink attack).
    wchar_t ps1Path[MAX_PATH];
    if (GetTempFileNameW(tempDir, L"dbl", 0, ps1Path) == 0) {
        PrintError("RunPowerShell: GetTempFileNameW failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        return "";
    }

    // RAII scope guard: delete the temp file on scope exit, even if an
    // exception is thrown mid-execution (e.g. bad_alloc during output capture).
    struct TempFileGuard {
        const wchar_t* path;
        ~TempFileGuard() { if (path) DeleteFileW(path); }
    } guard{ ps1Path };

    // GetTempFileNameW just created a brand-new, uniquely-named file; reopen it
    // with CREATE_ALWAYS (truncate) and write the UTF-16LE payload + BOM.
    HANDLE hFile = CreateFileW(ps1Path, GENERIC_WRITE, 0, NULL,
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
        + std::wstring(ps1Path) + L"\" 2>&1";

    FILE* pipe = _wpopen(cmd.c_str(), L"r");
    std::string result;
    if (pipe) {
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe))
            result += buf;
        _pclose(pipe);
    }

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
    if (!std::cin) return false;
    if (in.empty()) return true;
    return std::tolower(static_cast<unsigned char>(in[0])) == 'y';
}

bool CreateRestorePoint() {
    // Enable-ComputerRestore may error if System Restore is already enabled on
    // the drive; that is harmless, so silence it. Checkpoint-Computer, however,
    // must surface real failures (disabled SR, disk full, group-policy block,
    // etc.) so the user is not falsely told a safety net exists. We use
    // -ErrorAction Stop so the error becomes terminating and the try/catch can
    // capture it, then emit a honest [OK]/[!!] marker the C++ side keys off.
    std::string out = RunPowerShell(
        L"Enable-ComputerRestore -Drive 'C:\\' -ErrorAction SilentlyContinue; "
        L"$ok = $true; $errMsg = ''; "
        L"try { Checkpoint-Computer -Description 'Before Debloat' "
        L"-RestorePointType 'MODIFY_SETTINGS' -ErrorAction Stop } "
        L"catch { $ok = $false; $errMsg = $_.Exception.Message }; "
        L"if ($ok) { Write-Host '[OK] Restore point created.' } "
        L"else { Write-Host ('[!!] Failed: ' + $errMsg) }"
    );
    // Trim trailing whitespace/newlines so the colored printers don't double-up.
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' ||
                            out.back() == ' '  || out.back() == '\t'))
        out.pop_back();
    // Success is signalled by the [OK] marker; anything else (empty output,
    // [!!] failure line, or an unexpected payload) is treated as failure.
    bool success = out.find("[OK]") != std::string::npos;
    if (success)
        PrintSuccess(out);
    else
        PrintError(out.empty() ? "[!!] Failed: unable to create restore point." : out);
    return success;
}

} // namespace Utils
