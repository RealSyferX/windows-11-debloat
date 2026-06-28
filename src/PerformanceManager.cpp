#include "PerformanceManager.h"
#include "Utils.h"
#include <iostream>

void PerformanceManager::List() {
    Utils::PrintHeader("Performance & cleanup tasks:");
    std::cout <<
        "  [ 1] Disable hibernation (saves several GB of disk space)\n"
        "  [ 2] Disable fast startup (fixes dual-boot/driver issues)\n"
        "  [ 3] Set High Performance power plan\n"
        "  [ 4] Clean temp files (%TEMP%, Windows Temp, prefetch)\n"
        "       NOTE: Clearing Prefetch briefly slows boot until rebuilt.\n"
        "  [ 5] Clean Windows Update cache (SoftwareDistribution)\n"
        "  [ 6] Clean WinSxS component store (DISM /ResetBase)\n"
        "       NOTE: /ResetBase makes installed updates un-uninstallable.\n"
        "  -- All applied together via option 8 --\n\n";
}

void PerformanceManager::ApplyAll() {
    Utils::PrintHeader("Applying performance tweaks & disk cleanup...");

    std::wstring script =
        L"$ErrorActionPreference = 'SilentlyContinue'\n"

        // Disable hibernation
        L"Write-Host '  Disabling hibernation...'\n"
        L"powercfg /h off\n"
        L"Write-Host '  [OK] Hibernation disabled'\n\n"

        // Disable fast startup
        L"Write-Host '  Disabling fast startup...'\n"
        L"Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power' "
        L"  -Name 'HiberbootEnabled' -Value 0 -Type DWord\n"
        L"Write-Host '  [OK] Fast startup disabled'\n\n"

        // High performance power plan
        L"Write-Host '  Setting High Performance power plan...'\n"
        L"$hp = powercfg -l | Where-Object { $_ -match 'High performance' }\n"
        L"if ($hp -match '([a-fA-F0-9-]{36})') { powercfg -setactive $Matches[1] }\n"
        L"Write-Host '  [OK] High Performance power plan set'\n\n"

        // Clean temp files
        L"Write-Host '  Cleaning temp files...'\n"
        L"Remove-Item -Path \"$env:TEMP\\*\" -Recurse -Force\n"
        L"Remove-Item -Path \"$env:SystemRoot\\Temp\\*\" -Recurse -Force\n"
        L"Remove-Item -Path \"$env:SystemRoot\\Prefetch\\*\" -Recurse -Force\n"
        L"Write-Host '  [OK] Temp files cleaned'\n\n"

        // Clean Windows Update cache
        L"Write-Host '  Cleaning Windows Update cache...'\n"
        L"Stop-Service -Name wuauserv -Force\n"
        L"Remove-Item -Path \"$env:SystemRoot\\SoftwareDistribution\\Download\\*\" -Recurse -Force\n"
        L"Start-Service -Name wuauserv\n"
        L"Write-Host '  [OK] Windows Update cache cleaned'\n\n"

        // DISM WinSxS cleanup
        L"Write-Host '  Cleaning WinSxS component store (this may take a while)...'\n"
        L"DISM /Online /Cleanup-Image /StartComponentCleanup /ResetBase\n"
        L"Write-Host '  [OK] WinSxS cleanup complete'\n\n"

        L"Write-Host 'All performance tweaks applied.'\n";

    auto r = Utils::RunPowerShell(script);
    if (!r.out.empty()) std::cout << r.out;
    if (r.ok) Utils::PrintSuccess("Performance tweaks & disk cleanup complete.");
    else      Utils::PrintError("PowerShell failed to execute — changes may not have applied.");
}
