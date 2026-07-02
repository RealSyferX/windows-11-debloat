#include "Utils.h"
#include <cstdio>
#include <iostream>
#include <cctype>
#include <fstream>
#include <shellapi.h>
#include <sddl.h>
#include <aclapi.h>

namespace Utils {

// When true, AskYesNo() auto-confirms (returns true) without reading stdin.
// Set by SetAutoYes(true) from the --yes/-y CLI flag for non-interactive mode.
static bool g_autoYes = false;

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

PowerShellResult RunPowerShell(const std::wstring& script,
                                DWORD timeoutMs,
                                bool showProgress) {
    wchar_t tempDir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0)
        return { false, "" };

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
        return { false, "" };
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
        return { false, "" };

    DWORD wr = 0;
    const BYTE bom[2] = { 0xFF, 0xFE };
    // WriteFile can fail partway (disk full, antivirus lock, memory pressure),
    // leaving the temp .ps1 truncated or empty. If we launched PowerShell
    // against such a file we'd capture a parse error in `out` but still return
    // {true, out} — falsely reporting success when the script never ran. Guard
    // both writes by checking the BOOL return AND the byte count actually
    // written. GetLastError() is captured before CloseHandle so the diagnostic
    // reflects the WriteFile failure, not the handle teardown.
    if (!WriteFile(hFile, bom, 2, &wr, NULL) || wr != 2) {
        PrintError("RunPowerShell: WriteFile (BOM) failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        CloseHandle(hFile);
        return { false, "" };
    }
    DWORD expected = static_cast<DWORD>(script.size() * sizeof(wchar_t));
    if (!WriteFile(hFile, script.c_str(), expected, &wr, NULL) || wr != expected) {
        PrintError("RunPowerShell: WriteFile (script) failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        CloseHandle(hFile);
        return { false, "" };
    }
    CloseHandle(hFile);

    // --- Create anonymous pipe for child stdout+stderr ----------------------
    // The pipe write end must be inheritable so the child can write to it; the
    // read end must NOT be inherited (otherwise it would keep the pipe open in
    // the parent and ReadFile would never see EOF).
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE pipeRead = NULL, pipeWrite = NULL;
    if (!CreatePipe(&pipeRead, &pipeWrite, &sa, 65536)) {
        PrintError("RunPowerShell: CreatePipe failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        return { false, "" };
    }
    SetHandleInformation(pipeWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(pipeRead,  HANDLE_FLAG_INHERIT, 0);

    // --- Launch cmd.exe wrapping powershell.exe -----------------------------
    // The cmd.exe /c wrapper lets us merge stderr into stdout via 2>&1 so a
    // single pipe captures both streams. CREATE_NO_WINDOW prevents a console
    // popup. hStdInput is left NULL so that any PowerShell prompt for input
    // fails immediately instead of hanging forever.
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = pipeWrite;
    si.hStdError  = pipeWrite;
    si.hStdInput  = NULL;

    std::wstring cmd = L"cmd.exe /c powershell.exe -NoProfile -ExecutionPolicy "
        L"Bypass -File \"" + std::wstring(ps1Path) + L"\" 2>&1";

    // CreateProcessW requires a writable command-line buffer. std::wstring
    // data is contiguous and null-terminated; &cmd[0] yields a writable
    // wchar_t* (guaranteed since C++11, and MSVC has always done this).
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE,
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        PrintError("RunPowerShell: CreateProcessW failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        CloseHandle(pipeRead);
        CloseHandle(pipeWrite);
        return { false, "" };
    }

    // Close the write end in the parent so EOF is signalled when the child
    // exits (otherwise ReadFile would block forever waiting for more data).
    CloseHandle(pipeWrite);

    // --- Wait for the child with a timeout, draining the pipe ---------------
    // We poll in short intervals instead of a single blocking
    // WaitForSingleObject(fullTimeout) so that we can drain the pipe between
    // polls. If the child produces more output than the pipe buffer holds, a
    // single blocking wait would deadlock: the child blocks on write, the
    // parent blocks on wait, and neither progresses. The drain loop prevents
    // this while still respecting the overall timeout.
    std::string rawBytes;
    bool timedOut = false;
    DWORD elapsed = 0;
    const DWORD pollMs = 200;

    // Drains all currently-available bytes from the pipe into rawBytes.
    auto drainPipe = [&]() {
        DWORD avail = 0;
        while (PeekNamedPipe(pipeRead, NULL, 0, NULL, &avail, NULL) && avail > 0) {
            char buf[4096];
            DWORD nRead = 0;
            DWORD toRead = avail;
            if (toRead > sizeof(buf)) toRead = sizeof(buf);
            if (!ReadFile(pipeRead, buf, toRead, &nRead, NULL) || nRead == 0)
                break;
            rawBytes.append(buf, nRead);
        }
    };

    // --- Optional console spinner for long-running operations ----------------
    // When showProgress is true, a rotating spinner is printed to std::cout
    // (the console) after 3 seconds of no new pipe output, so the user can
    // see the tool is still alive during DISM /ResetBase or appx removal.
    // The spinner writes ONLY to std::cout — never to rawBytes — so the
    // captured output (r.out) is never corrupted.
    static const char spinner[] = "|/-\\";
    int spinnerIdx = 0;
    DWORD msSinceLastOutput = 0;
    const DWORD spinnerThresholdMs = 3000;
    bool spinnerActive = false;

    while (true) {
        size_t prevSize = rawBytes.size();
        drainPipe();

        DWORD waitResult = WaitForSingleObject(pi.hProcess, pollMs);
        elapsed += pollMs;

        // Spinner: only after spinnerThresholdMs of no new pipe output.
        if (showProgress) {
            bool newOutput = (rawBytes.size() > prevSize);
            if (newOutput) {
                msSinceLastOutput = 0;
                if (spinnerActive) {
                    std::cout << "   \r" << std::flush;
                    spinnerActive = false;
                }
            } else {
                msSinceLastOutput += pollMs;
            }
            if (msSinceLastOutput >= spinnerThresholdMs) {
                std::cout << spinner[spinnerIdx] << "\r" << std::flush;
                spinnerIdx = (spinnerIdx + 1) % 4;
                spinnerActive = true;
            }
        }

        if (waitResult == WAIT_OBJECT_0) {
            // Process exited — final drain to capture any trailing output
            // that was written between the last poll and exit.
            drainPipe();
            break;
        }

        if (elapsed >= timeoutMs) {
            timedOut = true;
            TerminateProcess(pi.hProcess, 1);
            // Reap the terminated process so the handle is fully released
            // before we close it below.
            WaitForSingleObject(pi.hProcess, 5000);
            drainPipe();   // grab whatever was buffered before the kill
            break;
        }
    }

    // Clear the spinner line so it doesn't leave a stray character on the
    // console before subsequent output (e.g. PrintPsResult).
    if (spinnerActive) {
        std::cout << "   \r" << std::flush;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pipeRead);

    // --- Convert raw pipe bytes to a narrow string --------------------------
    // The script was written as UTF-16LE with BOM, so PowerShell *may* emit
    // UTF-16LE output (it depends on $OutputEncoding). Detect the BOM and
    // convert accordingly; if no BOM, treat the bytes as ASCII/UTF-8.
    std::string out;
    if (rawBytes.size() >= 2 &&
        static_cast<unsigned char>(rawBytes[0]) == 0xFF &&
        static_cast<unsigned char>(rawBytes[1]) == 0xFE) {
        // UTF-16LE with BOM — skip the 2-byte BOM and convert to UTF-8.
        const wchar_t* wsrc =
            reinterpret_cast<const wchar_t*>(rawBytes.data() + 2);
        int wlen = static_cast<int>((rawBytes.size() - 2) / sizeof(wchar_t));
        int len = WideCharToMultiByte(CP_UTF8, 0, wsrc, wlen, NULL, 0, NULL, NULL);
        if (len > 0) {
            out.resize(len);
            WideCharToMultiByte(CP_UTF8, 0, wsrc, wlen, &out[0], len, NULL, NULL);
        }
    } else {
        // No BOM — treat as ASCII/UTF-8 directly.
        out = rawBytes;
    }

    if (timedOut) {
        out += "[!!] PowerShell timed out after " +
            std::to_string(timeoutMs / 1000) +
            "s \xE2\x80\x94 operation may be incomplete.";
        return { false, out };
    }

    return { true, out };
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

void PrintPsResult(const PowerShellResult& r,
                   const std::string& successMsg,
                   const std::string& errorMsg) {
    if (!r.out.empty()) std::cout << r.out;
    if (r.ok) PrintSuccess(successMsg);
    else      PrintError(errorMsg);
}

void SetAutoYes(bool enabled) {
    g_autoYes = enabled;
}

bool AskYesNo(const std::string& prompt) {
    if (g_autoYes) {
        std::cout << prompt << " (y/N): y\n";
        return true;
    }
    std::cout << prompt << " (y/N): ";
    std::string in;
    std::getline(std::cin, in);
    if (!std::cin) return false;
    if (in.empty()) return false;
    return std::tolower(static_cast<unsigned char>(in[0])) == 'y';
}

bool CreateRestorePoint() {
    // Enable-ComputerRestore may error if System Restore is already enabled on
    // the drive; that is harmless, so silence it. The drive letter is derived
    // from %SystemRoot% (e.g. "C:" or "D:") rather than hardcoded, so SR is
    // enabled on the correct drive on systems where Windows is not on C:.
    // Checkpoint-Computer, however, must surface real failures (disabled SR,
    // disk full, group-policy block, etc.) so the user is not falsely told a
    // safety net exists. We use -ErrorAction Stop so the error becomes
    // terminating and the try/catch can capture it, then emit a honest
    // [OK]/[!!] marker the C++ side keys off.
    // The restore point description includes the tool version so that, when
    // reviewing System Restore points, it is clear which version of Debloat
    // created the point.
    std::wstring desc = EscapePsSingleQuote(
        L"Before Debloat v" + StringToWide(GetVersion()));
    auto r = RunPowerShell(
        L"Enable-ComputerRestore -Drive $env:SystemRoot.Substring(0,2) -ErrorAction SilentlyContinue; "
        L"$ok = $true; $errMsg = ''; "
        L"try { Checkpoint-Computer -Description '" + desc + L"' "
        L"-RestorePointType 'MODIFY_SETTINGS' -ErrorAction Stop } "
        L"catch { $ok = $false; $errMsg = $_.Exception.Message }; "
        L"if ($ok) { Write-Host '[OK] Restore point created.' } "
        L"else { Write-Host ('[!!] Failed: ' + $errMsg) }"
    );
    // If PowerShell itself could not be launched, there is no honest result to
    // parse — report failure immediately rather than treating empty output as
    // a parseable payload.
    if (!r.ok) {
        PrintError("Failed to create restore point: PowerShell could not be launched.");
        return false;
    }
    std::string out = r.out;
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
    LogAction("RESTORE_POINT", "Created ok=" + std::to_string(success ? 1 : 0));
    return success;
}

std::string GetVersion() {
    return DEBLOAT_VERSION;
}

// -- Windows version detection ----------------------------------------------
//
// Reads values from HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion to
// determine the OS version. Windows 11 is identified by a build number >=
// 22000 (the first Windows 11 build was 22000.1; Windows 10 builds top out
// around 1904x). The registry approach requires no extra linkage beyond
// advapi32 (already linked) and avoids the deprecated GetVersionExW API,
// which is manifest-bound to return compatibility-shim values on modern
// Windows.

// Reads a REG_SZ value from the Windows NT CurrentVersion key. Returns the
// value as a UTF-8 string, or an empty string if the value is missing or
// cannot be read. File-local helper (not part of the Utils public API).
static std::string ReadCurrentVersionString(const wchar_t* valueName) {
    wchar_t buf[256];
    DWORD sz = sizeof(buf);   // bytes, including space for the null terminator
    LONG r = RegGetValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        valueName, RRF_RT_REG_SZ, NULL, buf, &sz);
    if (r != ERROR_SUCCESS)
        return "";
    // RegGetValueW includes the terminating null in sz. Exclude it so
    // WideToString produces a clean string with no embedded null.
    DWORD wlen = static_cast<DWORD>(sz / sizeof(wchar_t));
    if (wlen > 0 && buf[wlen - 1] == L'\0')
        --wlen;
    return WideToString(std::wstring(buf, wlen));
}

// Reads a REG_DWORD value from the Windows NT CurrentVersion key. Returns
// false if the value is missing or cannot be read. File-local helper.
static bool ReadCurrentVersionDword(const wchar_t* valueName, DWORD& out) {
    DWORD data = 0;
    DWORD sz = sizeof(data);
    LONG r = RegGetValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        valueName, RRF_RT_DWORD, NULL, &data, &sz);
    if (r != ERROR_SUCCESS)
        return false;
    out = data;
    return true;
}

int ParseBuildNumber(const std::string& buildStr) {
    if (buildStr.empty())
        return 0;
    int result = 0;
    for (char c : buildStr) {
        if (c < '0' || c > '9')
            break;   // stop at the first non-digit (e.g. '.' in "22631.1")
        result = result * 10 + (c - '0');
    }
    return result;
}

bool IsWindows11() {
    std::string buildStr = ReadCurrentVersionString(L"CurrentBuild");
    if (buildStr.empty())
        return true;   // fail-open: don't block users on a registry edge case
    int build = ParseBuildNumber(buildStr);
    if (build == 0)
        return true;   // unparseable build -- fail-open
    return build >= 22000;
}

std::string GetWindowsVersionString() {
    std::string buildStr = ReadCurrentVersionString(L"CurrentBuild");
    int build = ParseBuildNumber(buildStr);

    // UBR (Update Build Revision) is a DWORD appended after a dot, e.g.
    // "22631.4317". It is optional -- not all systems expose it -- so only
    // append it when the read succeeds.
    DWORD ubr = 0;
    bool hasUbr = ReadCurrentVersionDword(L"UBR", ubr);

    // Determine the OS display name. The ProductName registry value is
    // unreliable on Windows 11 (it often still reads "Windows 10"), so we
    // derive the major name from the build number when possible and fall
    // back to ProductName only when the build number is unavailable.
    std::string name;
    if (build >= 22000) {
        name = "Windows 11";
    } else if (build > 0) {
        name = "Windows 10";
    } else {
        name = ReadCurrentVersionString(L"ProductName");
        if (name.empty())
            name = "Windows";
    }

    std::string result = name;
    if (!buildStr.empty()) {
        result += " (build " + buildStr;
        if (hasUbr)
            result += "." + std::to_string(ubr);
        result += ")";
    }
    return result;
}

bool CreateSecureDirectory(const std::wstring& path) {
    // SDDL: D: = DACL, P = Protected (do NOT inherit from parent). This is the
    // key part that prevents %ProgramData%'s loose permissions (which grant
    // "Users" create/modify on subdirectories) from applying to our backup
    // folder. Only Built-in Administrators (BA) and SYSTEM (SY) get Full
    // Access (FA). A non-admin user therefore cannot tamper with the backup
    // files (service_backup.txt, reg_backup.txt, perf_backup.txt) that this
    // elevated tool reads back during Revert() — closing a privilege
    // escalation vector where a standard user could inject a crafted backup
    // file causing the next elevated run to apply attacker-controlled config.
    LPCWSTR sddl = L"D:P(A;;FA;;BA)(A;;FA;;SY)";
    PSECURITY_DESCRIPTOR pSD = NULL;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl, SDDL_REVISION_1, &pSD, NULL)) {
        PrintError("CreateSecureDirectory: ConvertStringSecurityDescriptorToSecurityDescriptorW failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        return false;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    bool ok = false;
    if (CreateDirectoryW(path.c_str(), &sa)) {
        ok = true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            // The directory already exists (possibly created by a prior,
            // non-hardened run, or by a different caller). Apply the
            // restrictive DACL to it now so existing loose permissions are
            // replaced. PROTECTED_DACL_SECURITY_INFORMATION makes the new DACL
            // protected (no inherit) — matching the "P" flag above.
            BOOL daclPresent = FALSE;
            PACL dacl = NULL;
            BOOL daclDefaulted = FALSE;
            if (GetSecurityDescriptorDacl(pSD, &daclPresent, &dacl, &daclDefaulted) &&
                daclPresent && dacl != NULL) {
                DWORD si = SetNamedSecurityInfoW(
                    const_cast<LPWSTR>(path.c_str()),
                    SE_FILE_OBJECT,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL, NULL, dacl, NULL);
                if (si == ERROR_SUCCESS) {
                    ok = true;
                } else {
                    PrintError("CreateSecureDirectory: SetNamedSecurityInfoW failed (GLE=" +
                        std::to_string(si) + ")");
                }
            } else {
                PrintError("CreateSecureDirectory: GetSecurityDescriptorDacl failed (GLE=" +
                    std::to_string(GetLastError()) + ")");
            }
        } else {
            PrintError("CreateSecureDirectory: CreateDirectoryW failed (GLE=" +
                std::to_string(err) + ")");
        }
    }

    LocalFree(pSD);
    return ok;
}

std::wstring GetDebloatDataDir() {
    wchar_t buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"ProgramData", buf, MAX_PATH);
    std::wstring dir;
    if (len == 0 || len >= MAX_PATH)
        dir = L"C:\\ProgramData\\Debloat\\";   // safe fallback
    else
        dir = std::wstring(buf) + L"\\Debloat\\";
    CreateSecureDirectory(dir);
    return dir;
}

void LogAction(const std::string& action, const std::string& detail) {
    // GetDebloatDataDir() already creates the directory if missing.
    std::wstring logPath = GetDebloatDataDir() + L"debloat.log";
    // MSVC's std::ofstream accepts a wchar_t* path (non-standard but required
    // for paths that contain non-ASCII characters from the system locale).
    std::ofstream log(logPath, std::ios::out | std::ios::app);
    if (!log.is_open())
        return;   // silent failure — don't crash the tool over logging

    SYSTEMTIME st;
    GetLocalTime(&st);
    char ts[32];
    std::snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    log << "[" << ts << "] " << action;
    if (!detail.empty())
        log << ": " << detail;
    log << "\n" << std::flush;   // flush so entries survive a crash mid-operation
}

std::wstring EscapePsSingleQuote(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size() + 4);
    for (wchar_t c : s) {
        if (c == L'\'') out += L"''";
        else            out += c;
    }
    return out;
}

bool WriteBackupAtomic(const std::wstring& path,
                       const std::function<bool(std::ofstream&)>& writer) {
    // Construct a temp path in the same directory so the rename is atomic
    // (same volume). If the process crashes mid-write, the temp file is
    // left behind but the original backup is untouched.
    std::wstring tmpPath = path + L".tmp";

    std::ofstream tmp(tmpPath, std::ios::binary | std::ios::trunc);
    if (!tmp.is_open()) {
        PrintWarning("WriteBackupAtomic: could not create temp file: " +
            WideToString(tmpPath));
        return false;
    }

    // Let the caller populate the temp file. If the writer returns false,
    // the write is considered failed — clean up and bail.
    if (!writer(tmp)) {
        tmp.close();
        DeleteFileW(tmpPath.c_str());
        return false;
    }

    tmp.flush();
    tmp.close();

    // Atomically replace the target with the fully-written temp file.
    // MOVEFILE_REPLACE_EXISTING : overwrite the target atomically.
    // MOVEFILE_WRITE_THROUGH   : the rename does not return until the
    //   change is flushed to disk. Backup files are read back during
    //   Revert operations, so their durability is critical for the
    //   revert safety net — a power loss right after the rename returns
    //   must not lose the backup (otherwise Revert has nothing to
    //   restore from).
    if (!MoveFileExW(tmpPath.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        PrintError("WriteBackupAtomic: MoveFileExW failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        DeleteFileW(tmpPath.c_str());
        return false;
    }

    return true;
}

bool WriteFileAtomic(const std::wstring& path, const std::string& content) {
    // Construct a temp path in the same directory so the rename is atomic
    // (same volume). If the process crashes mid-write, the temp file is
    // left behind but the original file is untouched. This is the
    // crash-safe write path used for the hosts file — arguably the most
    // critical system file the tool modifies — so it must never leave the
    // target truncated/corrupt on a partial write.
    std::wstring tmpPath = path + L".tmp";

    std::ofstream tmp(tmpPath, std::ios::binary | std::ios::trunc);
    if (!tmp.is_open()) {
        PrintError("Failed to create temporary file for atomic write");
        return false;
    }

    tmp << content;
    tmp.flush();
    tmp.close();

    // MOVEFILE_REPLACE_EXISTING : overwrite the target atomically.
    // MOVEFILE_WRITE_THROUGH   : the rename does not return until the
    //   change is flushed to disk, so once we report success the new
    //   content is durable (as durable as the filesystem allows).
    if (!MoveFileExW(tmpPath.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        PrintError("WriteFileAtomic: MoveFileExW failed (GLE=" +
            std::to_string(GetLastError()) + ")");
        DeleteFileW(tmpPath.c_str());
        return false;
    }

    return true;
}

} // namespace Utils
