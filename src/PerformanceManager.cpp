#include "PerformanceManager.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <string>

// -- Backup helpers ----------------------------------------------------------
//
// ApplyAll() snapshots the original state of the three reversible performance
// tweaks (hibernation, fast startup, power plan) into a pipe-delimited text
// file at %ProgramData%\Debloat\perf_backup.txt so Revert() can restore them.
// The other three tweaks (temp cleanup, WU cache, DISM /ResetBase) are
// irreversible and intentionally not backed up.

// Returns the full path to the performance backup file at
// %ProgramData%\Debloat\perf_backup.txt. Creates the directory if it does not
// exist (idempotent — ERROR_ALREADY_EXISTS is ignored by CreateDirectoryW).
// Falls back to C:\ProgramData\Debloat\ if %ProgramData% is unset/unavailable.
// Mirrors the GetBackupFilePath() pattern in ServiceManager and
// TelemetryManager so all backups live side-by-side in the same directory.
static std::wstring GetBackupFilePath() {
    wchar_t buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"ProgramData", buf, MAX_PATH);
    std::wstring dir;
    if (len == 0 || len >= MAX_PATH)
        dir = L"C:\\ProgramData\\Debloat\\";   // safe fallback
    else
        dir = std::wstring(buf) + L"\\Debloat\\";
    CreateDirectoryW(dir.c_str(), NULL);
    return dir + L"perf_backup.txt";
}

// Check whether hibernation is currently enabled by looking for hiberfil.sys
// on the system drive. powercfg /h off deletes this file; powercfg /h on
// recreates it. GetFileAttributesW returns INVALID_FILE_ATTRIBUTES if the file
// does not exist (it does not care about hidden/system attributes).
static bool IsHibernationOn() {
    wchar_t sysDrive[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"SystemDrive", sysDrive, MAX_PATH);
    std::wstring hiberPath = (len > 0 && len < MAX_PATH)
        ? std::wstring(sysDrive) + L"\\hiberfil.sys"
        : L"C:\\hiberfil.sys";
    DWORD attr = GetFileAttributesW(hiberPath.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

// Read the current HiberbootEnabled DWORD from the registry. Returns false if
// the value does not exist or cannot be read (ERROR_FILE_NOT_FOUND, etc.).
static bool ReadHiberbootEnabled(DWORD& outValue) {
    DWORD data = 0;
    DWORD sz = sizeof(data);
    LONG r = RegGetValueW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power",
        L"HiberbootEnabled", RRF_RT_DWORD, NULL, &data, &sz);
    if (r == ERROR_SUCCESS) {
        outValue = data;
        return true;
    }
    return false;
}

// Run powercfg /getactivescheme and parse the GUID from the output. The output
// looks like: "Power Scheme GUID: 381b4222-f694-4f7c-978e-ea2cd5f4d93e  (Balanced)"
// Returns an empty string if the GUID could not be parsed or PowerShell failed.
static std::string GetActivePowerPlanGuid() {
    auto r = Utils::RunPowerShell(L"powercfg /getactivescheme\n", 15000);
    if (!r.ok) return "";
    const std::string& s = r.out;
    size_t pos = s.find("GUID:");
    if (pos == std::string::npos) return "";
    pos += 5;  // skip "GUID:"
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
    if (pos + 36 > s.size()) return "";
    std::string guid = s.substr(pos, 36);
    // Validate: 8hex-4hex-4hex-4hex-12hex
    for (int i = 0; i < 36; ++i) {
        char c = guid[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return "";
        } else {
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F')))
                return "";
        }
    }
    return guid;
}

void PerformanceManager::List() {
    Utils::PrintHeader("Performance & cleanup tasks:");
    std::cout <<
        "  [ 1] Disable hibernation (saves several GB of disk space)\n"
        "       NOTE: reversible -- re-enabled by option 16\n"
        "  [ 2] Disable fast startup (fixes dual-boot/driver issues)\n"
        "       NOTE: reversible -- restored by option 16\n"
        "  [ 3] Set High Performance power plan\n"
        "       NOTE: reversible -- restored by option 16\n"
        "  [ 4] Clean temp files (%TEMP%, Windows Temp, prefetch)\n"
        "       NOTE: irreversible -- cleared files cannot be restored\n"
        "       NOTE: Clearing Prefetch briefly slows boot until rebuilt.\n"
        "  [ 5] Clean Windows Update cache (SoftwareDistribution)\n"
        "       NOTE: irreversible -- cleared cache cannot be restored\n"
        "  [ 6] Clean WinSxS component store (DISM /ResetBase)\n"
        "       NOTE: irreversible -- /ResetBase makes installed updates un-uninstallable.\n"
        "  -- All applied together via option 8 --\n"
        "  -- Revert reversible tweaks via option 16 --\n\n";
}

void PerformanceManager::ApplyAll() {
    Utils::PrintHeader("Applying performance tweaks & disk cleanup...");

    // --- Backup reversible settings before modifying them -------------------
    // Record the original state of the three reversible tweaks so Revert()
    // can restore them exactly. The other three tweaks (temp cleanup, WU
    // cache, DISM /ResetBase) are irreversible and not backed up.
    {
        std::wstring backupPath = GetBackupFilePath();
        std::ofstream backup(backupPath, std::ios::out | std::ios::trunc);
        if (backup.is_open()) {
            backup << "hibernation|" << (IsHibernationOn() ? "on" : "off") << "\n";
            DWORD hbVal = 0;
            if (ReadHiberbootEnabled(hbVal))
                backup << "hiberbootEnabled|" << hbVal << "\n";
            else
                backup << "hiberbootEnabled|missing\n";
            std::string guid = GetActivePowerPlanGuid();
            backup << "powerPlan|" << (guid.empty() ? "unknown" : guid) << "\n";
            backup.flush();
            backup.close();
        } else {
            Utils::PrintWarning("Could not open performance backup file -- reversible tweaks will not be restorable.");
        }
    }

    // --- PowerShell script with honest per-step success/failure reporting ---
    // Each command's result is checked before printing [OK]. Native executables
    // (powercfg, DISM) are checked via $LASTEXITCODE. Cmdlets are wrapped in
    // try/catch with -ErrorAction Stop. Remove-Item operations iterate per-item
    // so locked files are counted as failures rather than silently skipped.
    std::wstring script =
        L"$ErrorActionPreference = 'Stop'\n"

        // Disable hibernation
        L"Write-Host '  Disabling hibernation...'\n"
        L"powercfg /h off\n"
        L"if ($LASTEXITCODE -eq 0) { Write-Host '  [OK] Hibernation disabled' }\n"
        L"else { Write-Host \"  [!!] Hibernation disable failed (exit $LASTEXITCODE)\" }\n\n"

        // Disable fast startup
        L"Write-Host '  Disabling fast startup...'\n"
        L"try {\n"
        L"  Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power' "
        L"-Name 'HiberbootEnabled' -Value 0 -Type DWord -ErrorAction Stop\n"
        L"  Write-Host '  [OK] Fast startup disabled'\n"
        L"} catch {\n"
        L"  Write-Host \"  [!!] Fast startup disable failed: $_\"\n"
        L"}\n\n"

        // High performance power plan
        L"Write-Host '  Setting High Performance power plan...'\n"
        L"$hp = powercfg -l | Where-Object { $_ -match 'High performance' }\n"
        L"if ($hp -match '([a-fA-F0-9-]{36})') {\n"
        L"  powercfg -setactive $Matches[1]\n"
        L"  if ($LASTEXITCODE -eq 0) { Write-Host '  [OK] High Performance power plan set' }\n"
        L"  else { Write-Host \"  [!!] Power plan setactive failed (exit $LASTEXITCODE)\" }\n"
        L"} else {\n"
        L"  Write-Host '  [!!] High Performance power plan not found'\n"
        L"}\n\n"

        // Clean temp files (per-item counting so locked files are reported)
        L"Write-Host '  Cleaning temp files...'\n"
        L"$cleanOk = 0; $cleanFail = 0\n"
        L"foreach ($base in @($env:TEMP, \"$env:SystemRoot\\Temp\", \"$env:SystemRoot\\Prefetch\")) {\n"
        L"  $items = Get-ChildItem -Path $base -Force -ErrorAction SilentlyContinue\n"
        L"  foreach ($item in $items) {\n"
        L"    try {\n"
        L"      Remove-Item -Path $item.FullName -Recurse -Force -ErrorAction Stop\n"
        L"      $cleanOk++\n"
        L"    } catch {\n"
        L"      $cleanFail++\n"
        L"    }\n"
        L"  }\n"
        L"}\n"
        L"if ($cleanFail -eq 0) { Write-Host \"  [OK] Temp files cleaned ($cleanOk items)\" }\n"
        L"else { Write-Host \"  [!!] Temp cleanup: $cleanOk cleaned, $cleanFail failed (locked files skipped)\" }\n\n"

        // Clean Windows Update cache
        L"Write-Host '  Cleaning Windows Update cache...'\n"
        L"$wuStopOk = $true\n"
        L"try {\n"
        L"  Stop-Service -Name wuauserv -Force -ErrorAction Stop\n"
        L"} catch {\n"
        L"  $wuStopOk = $false\n"
        L"  Write-Host \"  [!!] Stop wuauserv failed: $_\"\n"
        L"}\n"
        L"if ($wuStopOk) {\n"
        L"  $wuCleanOk = 0; $wuCleanFail = 0\n"
        L"  $wuItems = Get-ChildItem -Path \"$env:SystemRoot\\SoftwareDistribution\\Download\" -Force -ErrorAction SilentlyContinue\n"
        L"  foreach ($wuItem in $wuItems) {\n"
        L"    try {\n"
        L"      Remove-Item -Path $wuItem.FullName -Recurse -Force -ErrorAction Stop\n"
        L"      $wuCleanOk++\n"
        L"    } catch {\n"
        L"      $wuCleanFail++\n"
        L"    }\n"
        L"  }\n"
        L"  try {\n"
        L"    Start-Service -Name wuauserv -ErrorAction Stop\n"
        L"  } catch {\n"
        L"    Write-Host \"  [!!] Restart wuauserv failed: $_\"\n"
        L"  }\n"
        L"  if ($wuCleanFail -eq 0) { Write-Host \"  [OK] Windows Update cache cleaned ($wuCleanOk items)\" }\n"
        L"  else { Write-Host \"  [!!] WU cache: $wuCleanOk cleaned, $wuCleanFail failed\" }\n"
        L"} else {\n"
        L"  try { Start-Service -Name wuauserv -ErrorAction SilentlyContinue } catch {}\n"
        L"  Write-Host '  [!!] Windows Update cache cleanup incomplete'\n"
        L"}\n\n"

        // DISM WinSxS cleanup
        L"Write-Host '  Cleaning WinSxS component store (this may take a while)...'\n"
        L"DISM /Online /Cleanup-Image /StartComponentCleanup /ResetBase\n"
        L"if ($LASTEXITCODE -eq 0) { Write-Host '  [OK] WinSxS cleanup complete' }\n"
        L"else { Write-Host \"  [!!] DISM failed (exit $LASTEXITCODE)\" }\n\n"

        L"Write-Host 'All performance tweaks applied.'\n";

    auto r = Utils::RunPowerShell(script, 900000);  // 15 minutes for DISM
    if (!r.out.empty()) std::cout << r.out;
    if (r.ok) Utils::PrintSuccess("Performance tweaks & disk cleanup complete.");
    else      Utils::PrintError("PowerShell failed to execute -- changes may not have applied.");
}

void PerformanceManager::Revert() {
    Utils::PrintHeader("Reverting performance tweaks from backup...");

    std::wstring backupPath = GetBackupFilePath();
    std::ifstream fin(backupPath);
    if (!fin.is_open()) {
        std::cout << "  [--] No performance backup found -- nothing to revert.\n";
        return;
    }

    std::string hibernationState, hiberbootVal, powerPlanGuid;
    std::string line;
    while (std::getline(fin, line)) {
        // Tolerate CRLF line endings.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        size_t bar = line.find('|');
        if (bar == std::string::npos) continue;
        std::string key = line.substr(0, bar);
        std::string val = line.substr(bar + 1);
        if (key == "hibernation")            hibernationState = val;
        else if (key == "hiberbootEnabled")  hiberbootVal = val;
        else if (key == "powerPlan")         powerPlanGuid = val;
    }
    fin.close();

    int restored = 0, failed = 0;

    // 1. Re-enable hibernation (only if it was on before ApplyAll turned it off)
    if (hibernationState == "on") {
        std::cout << "  Re-enabling hibernation...\n";
        auto r = Utils::RunPowerShell(
            L"powercfg /h on\n"
            L"if ($LASTEXITCODE -eq 0) { Write-Host '__OK__' } else { Write-Host '__FAIL__' }\n",
            30000);
        if (r.ok && r.out.find("__OK__") != std::string::npos) {
            std::cout << "  [OK] Hibernation re-enabled\n";
            ++restored;
        } else {
            std::cout << "  [!!] Hibernation re-enable failed\n";
            if (!r.out.empty()) std::cout << r.out;
            ++failed;
        }
    } else if (hibernationState == "off") {
        std::cout << "  [--] Hibernation was already off -- skipping\n";
    }

    // 2. Restore HiberbootEnabled (fast startup) via registry API
    if (!hiberbootVal.empty()) {
        std::cout << "  Restoring fast startup setting...\n";
        HKEY hKey = NULL;
        LONG r = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
        if (r != ERROR_SUCCESS) {
            std::cout << "  [!!] Fast startup -- open key failed (err " << r << ")\n";
            ++failed;
        } else if (hiberbootVal == "missing") {
            // Value did not exist before ApplyAll -- delete what we created
            r = RegDeleteValueW(hKey, L"HiberbootEnabled");
            if (r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND) {
                std::cout << "  [OK] Fast startup value removed (restored to default)\n";
                ++restored;
            } else {
                std::cout << "  [!!] Fast startup value delete failed (err " << r << ")\n";
                ++failed;
            }
        } else {
            DWORD val = 1;  // default to enabled if parse fails
            try { val = static_cast<DWORD>(std::stoul(hiberbootVal)); }
            catch (...) { val = 1; }
            r = RegSetValueExW(hKey, L"HiberbootEnabled", 0, REG_DWORD,
                reinterpret_cast<const BYTE*>(&val), sizeof(val));
            if (r == ERROR_SUCCESS) {
                std::cout << "  [OK] Fast startup restored (HiberbootEnabled=" << val << ")\n";
                ++restored;
            } else {
                std::cout << "  [!!] Fast startup restore failed (err " << r << ")\n";
                ++failed;
            }
        }
        if (hKey) RegCloseKey(hKey);
    }

    // 3. Restore power plan to the original GUID
    if (!powerPlanGuid.empty() && powerPlanGuid != "unknown") {
        std::cout << "  Restoring power plan to " << powerPlanGuid << "...\n";
        std::wstring psGuid = Utils::StringToWide(powerPlanGuid);
        std::wstring script = L"powercfg -setactive " + psGuid + L"\n"
            L"if ($LASTEXITCODE -eq 0) { Write-Host '__OK__' } else { Write-Host '__FAIL__' }\n";
        auto r = Utils::RunPowerShell(script, 30000);
        if (r.ok && r.out.find("__OK__") != std::string::npos) {
            std::cout << "  [OK] Power plan restored\n";
            ++restored;
        } else {
            std::cout << "  [!!] Power plan restore failed\n";
            if (!r.out.empty()) std::cout << r.out;
            ++failed;
        }
    } else if (powerPlanGuid == "unknown") {
        std::cout << "  [--] Original power plan unknown -- skipping\n";
    }

    // Note about irreversible tweaks
    std::cout << "\n  NOTE: The following tweaks are irreversible and cannot be reverted:\n"
              << "    - Temp file cleanup (deleted files cannot be restored)\n"
              << "    - Windows Update cache cleanup (deleted downloads cannot be restored)\n"
              << "    - WinSxS /ResetBase (installed updates made permanent)\n";

    std::cout << "\n  Summary: " << restored << " restored, " << failed << " failed.\n";
    if (failed == 0 && restored > 0)
        Utils::PrintSuccess("Performance tweaks reverted from backup.");
    else if (restored > 0)
        Utils::PrintWarning("Performance revert completed with some issues.");
    else
        Utils::PrintError("No performance tweaks could be restored.");
}
