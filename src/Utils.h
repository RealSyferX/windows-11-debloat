#pragma once
#include <string>
#include <functional>
#include <fstream>
#include <windows.h>

#ifndef DEBLOAT_VERSION
#define DEBLOAT_VERSION "unknown"
#endif

namespace Utils {

    // Result of a RunPowerShell call. `ok` is false when PowerShell could not
    // be launched at all (temp-path/file creation failure, or CreateProcessW
    // returned FALSE) OR when the script timed out. `out` is the captured
    // stdout/stderr; it may be empty even when ok==true (e.g. a script that
    // writes nothing). On timeout, `out` contains whatever output was captured
    // before the kill, followed by a timeout notice.
    struct PowerShellResult {
        bool        ok;
        std::string out;
    };

    bool IsElevated();
    bool RelaunchAsAdmin();
    // Writes the script to a unique, non-predictable temp file (GetTempFileNameW)
    // and auto-deletes it via RAII when RunPowerShell returns.
    //
    // Executes a PowerShell script with a timeout. Returns {ok, out} where
    // ok=false if PowerShell could not be launched OR if it timed out. The out
    // string contains captured stdout/stderr, or an error message on
    // timeout/launch failure.
    // timeoutMs: max milliseconds to wait (default 120000 = 2 minutes).
    PowerShellResult RunPowerShell(const std::wstring& script,
                                   DWORD timeoutMs = 120000);
    void SetColor(WORD attr);
    void PrintInfo(const std::string& msg);
    void PrintSuccess(const std::string& msg);
    void PrintError(const std::string& msg);
    void PrintWarning(const std::string& msg);
    void PrintHeader(const std::string& msg);
    // Prints the PowerShell result: captured output, then success or error message.
    void PrintPsResult(const PowerShellResult& r,
                       const std::string& successMsg,
                       const std::string& errorMsg);
    bool AskYesNo(const std::string& prompt);
    std::wstring StringToWide(const std::string& s);
    std::string WideToString(const std::wstring& ws);
    bool CreateRestorePoint();

    // Returns the version string (from DEBLOAT_VERSION macro).
    std::string GetVersion();

    // Creates a directory with Full Control for Administrators and SYSTEM only.
    // If the directory already exists, applies the restrictive ACL to it.
    // This prevents non-admin users from tampering with backup files that
    // the elevated tool reads back during Revert operations.
    bool CreateSecureDirectory(const std::wstring& path);

    // Returns %ProgramData%\Debloat\ (creating it if missing).
    // Falls back to C:\ProgramData\Debloat\ if the env var is unavailable.
    std::wstring GetDebloatDataDir();

    // Appends a timestamped entry to the persistent log file
    // (%ProgramData%\Debloat\debloat.log). Creates the directory if needed.
    // Silent on failure — never crashes the tool because logging failed.
    void LogAction(const std::string& action, const std::string& detail = "");

    // Doubles every single quote so the value is safe to embed inside a
    // PowerShell single-quoted string literal. Call this on ANY value
    // interpolated into a PS single-quoted string, even "trusted" table data.
    std::wstring EscapePsSingleQuote(const std::wstring& s);

    // Writes to a backup file atomically: creates a .tmp file, calls the writer
    // callback to populate it, flushes/closes, then renames over the target.
    // If the process crashes mid-write, the original backup is preserved.
    // Returns true on success, false on failure (logs a warning).
    bool WriteBackupAtomic(const std::wstring& path,
                           const std::function<bool(std::ofstream&)>& writer);
}
