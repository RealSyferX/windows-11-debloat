#include "ScheduledTaskManager.h"
#include "Utils.h"
#include <iostream>

const std::vector<TelemetryTask>& ScheduledTaskManager::GetTasks() {
    static const std::vector<TelemetryTask> tasks = {
        { L"Microsoft Compatibility Appraiser", L"\\Microsoft\\Windows\\Application Experience\\",
          "Collects program telemetry info for upgrades" },
        { L"ProgramDataUpdater", L"\\Microsoft\\Windows\\Application Experience\\",
          "Uploads program inventory data to Microsoft" },
        { L"StartupAppTask", L"\\Microsoft\\Windows\\Application Experience\\",
          "Monitors startup app changes" },
        { L"PcaPatchDbTask", L"\\Microsoft\\Windows\\Application Experience\\",
          "Compatibility patch database task" },
        { L"MareBackup", L"\\Microsoft\\Windows\\Application Experience\\",
          "Backup of Microsoft Account Recovery info" },
        { L"DsCleanup", L"\\Microsoft\\Windows\\Application Experience\\",
          "Cleanup of distributed sync data" },
        { L"Consolidator", L"\\Microsoft\\Windows\\Customer Experience Improvement Program\\",
          "Uploads CEIP telemetry to Microsoft" },
        { L"UsbCeip", L"\\Microsoft\\Windows\\Customer Experience Improvement Program\\",
          "USB-specific CEIP telemetry" },
        { L"KernelCeipTask", L"\\Microsoft\\Windows\\Customer Experience Improvement Program\\",
          "Kernel-mode CEIP telemetry" },
        { L"Microsoft-Windows-DiskDiagnosticDataCollector", L"\\Microsoft\\Windows\\DiskDiagnostic\\",
          "Disk diagnostic data collection" },
        { L"DmClient", L"\\Microsoft\\Windows\\Feedback\\Siuf\\",
          "Sends feedback telemetry to Microsoft" },
        { L"DmClientOnScenarioDownload", L"\\Microsoft\\Windows\\Feedback\\Siuf\\",
          "Feedback scenario download telemetry" },
        { L"MapsUpdateTask", L"\\Microsoft\\Windows\\Maps\\",
          "Offline maps auto-update" },
        { L"MapsToastTask", L"\\Microsoft\\Windows\\Maps\\",
          "Maps notification task" },
        { L"QueueReporting", L"\\Microsoft\\Windows\\Windows Error Reporting\\",
          "WER queue upload to Microsoft" },
        { L"AnalyzeSystem", L"\\Microsoft\\Windows\\Power Efficiency Diagnostics\\",
          "Power efficiency diagnostics telemetry" },
        { L"UploadCachedReports", L"\\Microsoft\\Windows\\Customer Experience Improvement Program Broker\\",
          "Uploads cached CEIP reports" },
    };
    return tasks;
}

void ScheduledTaskManager::List() {
    Utils::PrintHeader("Scheduled telemetry tasks targeted:");
    const auto& tasks = GetTasks();
    for (size_t i = 0; i < tasks.size(); i++)
        std::cout << "  [" << (i < 9 ? " " : "") << (i + 1) << "] "
                  << tasks[i].description << "\n      "
                  << Utils::WideToString(tasks[i].name) << "\n";
    std::cout << "\n";
}

void ScheduledTaskManager::DisableAll() {
    Utils::PrintHeader("Disabling scheduled telemetry tasks...");
    const auto& tasks = GetTasks();

    std::wstring script = L"$ErrorActionPreference = 'SilentlyContinue'\n$tasks = @(\n";
    for (size_t i = 0; i < tasks.size(); i++) {
        script += L"    @{Name='" + tasks[i].name + L"';Path='" + tasks[i].path + L"';Desc='"
            + Utils::StringToWide(tasks[i].description) + L"'}";
        if (i + 1 < tasks.size()) script += L",";
        script += L"\n";
    }
    script +=
        L")\n"
        L"foreach ($t in $tasks) {\n"
        L"    $r = Disable-ScheduledTask -TaskName $t.Name -TaskPath $t.Path\n"
        L"    if ($r) { Write-Host (\"  [OK] \" + $t.Desc) }\n"
        L"    else    { Write-Host (\"  [--] \" + $t.Name + ' - not found') }\n"
        L"}\n"
        L"Write-Host \"`nScheduled task cleanup complete.\"\n";

    std::string out = Utils::RunPowerShell(script);
    if (!out.empty()) std::cout << out;
    Utils::PrintSuccess("Scheduled telemetry tasks disabled.");
}

void ScheduledTaskManager::EnableAll() {
    Utils::PrintHeader("Re-enabling scheduled telemetry tasks...");
    const auto& tasks = GetTasks();

    std::wstring script = L"$ErrorActionPreference = 'SilentlyContinue'\n$tasks = @(\n";
    for (size_t i = 0; i < tasks.size(); i++) {
        script += L"    @{Name='" + tasks[i].name + L"';Path='" + tasks[i].path + L"';Desc='"
            + Utils::StringToWide(tasks[i].description) + L"'}";
        if (i + 1 < tasks.size()) script += L",";
        script += L"\n";
    }
    script +=
        L")\n"
        L"foreach ($t in $tasks) {\n"
        L"    $r = Enable-ScheduledTask -TaskName $t.Name -TaskPath $t.Path\n"
        L"    if ($r) { Write-Host (\"  [OK] \" + $t.Desc) }\n"
        L"    else    { Write-Host (\"  [--] \" + $t.Name + ' - not found') }\n"
        L"}\n"
        L"Write-Host \"`nScheduled task restore complete.\"\n";

    std::string out = Utils::RunPowerShell(script);
    if (!out.empty()) std::cout << out;
    Utils::PrintSuccess("Scheduled telemetry tasks re-enabled.");
}
