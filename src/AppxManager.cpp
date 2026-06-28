#include "AppxManager.h"
#include "Utils.h"
#include <iostream>

const std::vector<BloatwareApp>& AppxManager::GetBloatwareList() {
    static const std::vector<BloatwareApp> apps = {
        { L"Microsoft.BingNews",                     "Microsoft News" },
        { L"Microsoft.BingSearch",                   "Microsoft Bing" },
        { L"Microsoft.BingWeather",                  "MSN Weather" },
        { L"Microsoft.BingFinance",                  "MSN Money" },
        { L"Microsoft.BingSports",                   "MSN Sports" },
        { L"Microsoft.Copilot",                      "Copilot" },
        { L"Microsoft.GetHelp",                      "Get Help" },
        { L"Microsoft.Getstarted",                   "Microsoft Tips" },
        { L"Microsoft.Microsoft3DViewer",            "3D Viewer" },
        { L"Microsoft.MicrosoftOfficeHub",           "Office Hub" },
        { L"Microsoft.MicrosoftSolitaireCollection",  "Solitaire & Casual Games" },
        { L"Microsoft.MicrosoftStickyNotes",         "Sticky Notes" },
        { L"Microsoft.MixedReality.Portal",          "Mixed Reality Portal" },
        { L"Microsoft.OneConnect",                   "Paid Wi-Fi & Cellular" },
        { L"Microsoft.OutlookForWindows",            "Outlook for Windows" },
        { L"Microsoft.PowerAutomateDesktop",         "Power Automate" },
        { L"Microsoft.Skype",                        "Skype" },
        { L"Microsoft.Todos",                        "Microsoft To Do" },
        { L"Microsoft.Windows.DevHome",              "Dev Home" },
        { L"Microsoft.WindowsFeedbackHub",           "Feedback Hub" },
        { L"Microsoft.WindowsMaps",                  "Windows Maps" },
        { L"Microsoft.YourPhone",                    "Phone Link" },
        { L"Microsoft.ZuneMusic",                    "Windows Media Player (old)" },
        { L"Microsoft.ZuneVideo",                    "Movies & TV" },
        { L"Microsoft.549981C3F5F10",                "Cortana" },
        { L"MicrosoftCorporationII.MicrosoftFamily", "Microsoft Family Safety" },
        { L"MicrosoftCorporationII.QuickAssist",     "Quick Assist" },
        { L"MicrosoftWindows.Client.WebExperience",  "Web Experience (Widgets)" },
        { L"MicrosoftWindows.CrossDevice",           "Cross Device Experience" },
        { L"Clipchamp.Clipchamp",                    "Clipchamp" },
        { L"Microsoft.Teams",                        "Microsoft Teams" },
        { L"Microsoft.WindowsCommunicationsApps",    "Mail & Calendar" },
        { L"Microsoft.People",                       "People" },
        { L"Microsoft.WindowsSoundRecorder",         "Sound Recorder" },
        { L"Microsoft.BingFoodAndDrink",             "Bing Food & Drink" },
        { L"Microsoft.BingHealthAndFitness",         "Bing Health & Fitness" },
        { L"Microsoft.BingTravel",                   "Bing Travel" },
        { L"Microsoft.WindowsReadingList",           "Reading List" },
        { L"Microsoft.NetworkSpeedTest",             "Network Speed Test" },
    };
    return apps;
}

void AppxManager::List() {
    Utils::PrintHeader("Bloatware apps targeted for removal:");
    const auto& apps = GetBloatwareList();
    for (size_t i = 0; i < apps.size(); i++)
        std::cout << "  [" << (i < 9 ? " " : "") << (i + 1) << "] "
                  << apps[i].displayName << "  ("
                  << Utils::WideToString(apps[i].namePattern) << ")\n";
    std::cout << "\n";
}

void AppxManager::RemoveAll() {
    Utils::PrintHeader("Removing bloatware apps...");

    const auto& apps = GetBloatwareList();
    Utils::LogAction("APPX", "Removing " + std::to_string(apps.size()) + " bloatware apps");
    std::wstring script = L"$ErrorActionPreference = 'SilentlyContinue'\n$apps = @(\n";
    for (size_t i = 0; i < apps.size(); i++) {
        script += L"    '" + Utils::EscapePsSingleQuote(apps[i].namePattern) + L"'";
        if (i + 1 < apps.size()) script += L",";
        script += L"\n";
    }
    script +=
        L")\n"
        L"foreach ($app in $apps) {\n"
        L"    Write-Host \"  Removing: $app\" -NoNewline\n"
        L"    $found = $false\n"
        L"    $pkg = Get-AppxPackage -Name \"*$app*\" -AllUsers\n"
        L"    if ($pkg) {\n"
        L"        $pkg | Remove-AppxPackage -AllUsers\n"
        L"        $found = $true\n"
        L"    }\n"
        L"    $prov = Get-AppxProvisionedPackage -Online | "
        L"        Where-Object { $_.DisplayName -match [regex]::Escape($app) -or "
        L"                       $_.PackageName   -match [regex]::Escape($app) }\n"
        L"    if ($prov) {\n"
        L"        $prov | ForEach-Object { "
        L"            Remove-AppxProvisionedPackage -Online -PackageName $_.PackageName }\n"
        L"        $found = $true\n"
        L"    }\n"
        L"    if ($found) { Write-Host \"  [OK] removed\" }\n"
        L"    else        { Write-Host \"  [--] not installed\" }\n"
        L"}\n"
        L"Write-Host \"`nApp removal complete.\"\n";

    auto r = Utils::RunPowerShell(script, 120000, true);
    Utils::PrintPsResult(r,
        "Bloatware app removal finished.",
        "PowerShell failed to execute — changes may not have applied.");
    Utils::LogAction("APPX", std::string("RemoveAll complete ok=") + (r.ok ? "1" : "0"));
}

void AppxManager::RemoveOneDrive() {
    Utils::PrintHeader("Removing OneDrive...");
    Utils::LogAction("APPX", "Removing OneDrive");
    std::wstring script =
        L"$ErrorActionPreference = 'SilentlyContinue'\n"
        L"Write-Host \"  Stopping OneDrive process...\"\n"
        L"Stop-Process -Name 'OneDrive' -Force\n"
        L"Start-Sleep -Seconds 2\n"
        L"$ok = $false\n"
        L"$setup = @(\"$env:SystemRoot\\System32\\OneDriveSetup.exe\", "
        L"           \"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\") "
        L"          | Where-Object { Test-Path $_ } | Select-Object -First 1\n"
        L"if ($setup) {\n"
        L"    Write-Host \"  Uninstalling via $setup...\"\n"
        L"    Start-Process -FilePath $setup -ArgumentList '/uninstall' -Wait -NoNewWindow\n"
        L"    $ok = $true\n"
        L"}\n"
        L"if (-not $ok) {\n"
        L"    Write-Host \"  Trying winget...\"\n"
        L"    winget uninstall Microsoft.OneDrive --silent --accept-source-agreements 2>&1 | Out-Null\n"
        L"    if ($LASTEXITCODE -eq 0) { $ok = $true }\n"
        L"}\n"
        L"Remove-Item -Recurse -Force \"$env:LOCALAPPDATA\\Microsoft\\OneDrive\"\n"
        L"Remove-Item -Recurse -Force \"$env:ProgramData\\Microsoft OneDrive\"\n"
        L"Remove-ItemProperty -Path 'HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Run' "
        L"  -Name 'OneDrive' -ErrorAction SilentlyContinue\n"
        L"if ($ok) { Write-Host '  [OK] OneDrive removed' }\n"
        L"else     { Write-Host '  [--] OneDrive not found' }\n";
    auto r = Utils::RunPowerShell(script);
    Utils::PrintPsResult(r,
        "OneDrive removal finished.",
        "PowerShell failed to execute — changes may not have applied.");
}
