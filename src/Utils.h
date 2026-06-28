#pragma once
#include <string>
#include <windows.h>

namespace Utils {

    // Result of a RunPowerShell call. `ok` is false only when PowerShell could
    // not be launched at all (temp-path/file creation failure, or _wpopen
    // returned NULL). `out` is the captured stdout/stderr; it may be empty
    // even when ok==true (e.g. a script that writes nothing).
    struct PowerShellResult {
        bool        ok;
        std::string out;
    };

    bool IsElevated();
    bool RelaunchAsAdmin();
    // Writes the script to a unique, non-predictable temp file (GetTempFileNameW)
    // and auto-deletes it via RAII when RunPowerShell returns.
    PowerShellResult RunPowerShell(const std::wstring& script);
    void SetColor(WORD attr);
    void PrintInfo(const std::string& msg);
    void PrintSuccess(const std::string& msg);
    void PrintError(const std::string& msg);
    void PrintWarning(const std::string& msg);
    void PrintHeader(const std::string& msg);
    bool AskYesNo(const std::string& prompt);
    std::wstring StringToWide(const std::string& s);
    std::string WideToString(const std::wstring& ws);
    bool CreateRestorePoint();
}
