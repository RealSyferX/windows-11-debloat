#include "HostsManager.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

const std::vector<std::wstring>& HostsManager::GetBlockedDomains() {
    static const std::vector<std::wstring> domains = {
        L"vortex.data.microsoft.com",
        L"vortex-win.data.microsoft.com",
        L"telecommand.telemetry.microsoft.com",
        L"telemetry.microsoft.com",
        L"telemetry.appex.bing.net",
        L"telemetry.appex.bing.com",
        L"telemetry.urs.microsoft.com",
        L"telemetry-uap.microsoft.com",
        L"settings-win.data.microsoft.com",
        L"settings.data.microsoft.com",
        L"watson.telemetry.microsoft.com",
        L"watson.ppe.telemetry.microsoft.com",
        L"oca.telemetry.microsoft.com",
        L"redirection.telemetry.microsoft.com",
        L"compatexchange.cloudapp.net",
        L"cs43.wpc.v0cdn.net",
        L"statsfe2.ws.microsoft.com",
        L"statsfe2.update.microsoft.com",
        L"survey.watson.microsoft.com",
        L"watson.live.com",
        L"diagnostics.support.microsoft.com",
        L"corp.sts.microsoft.com",
        L"cosmosiris.z1.test.core.windows.net",
        L"ceusrsrvkkus2e4prdzkdkgf5.blob.core.windows.net",
        L"db3e02e5-3207-4b9b-8c6e-2a3a4e5b6c7d.blob.core.windows.net",
        L"modern.watson.data.microsoft.com",
        L"browser.events.data.microsoft.com",
        L"prod.activity.windows.com",
        L"prod.tls.all-visible.devices",
    };
    return domains;
}

static const wchar_t* MARKER = L"# ===== RealSyferX Debloat — Telemetry Block =====";
static const wchar_t* HOSTS_PATH = L"C:\\Windows\\System32\\drivers\\etc\\hosts";

void HostsManager::List() {
    Utils::PrintHeader("Telemetry domains to block in hosts file:");
    const auto& domains = GetBlockedDomains();
    for (size_t i = 0; i < domains.size(); i++)
        std::cout << "  [" << (i < 9 ? " " : "") << (i + 1) << "] 0.0.0.0  "
                  << Utils::WideToString(domains[i]) << "\n";
    std::cout << "\n";
}

void HostsManager::Apply() {
    Utils::PrintHeader("Blocking telemetry domains in hosts file...");

    std::wifstream fin(HOSTS_PATH);
    std::wstring content((std::istreambuf_iterator<wchar_t>(fin)),
                          std::istreambuf_iterator<wchar_t>());
    fin.close();

    if (content.find(MARKER) != std::wstring::npos) {
        std::cout << "  [--] Telemetry block already applied. Skipping.\n";
        Utils::PrintWarning("Hosts block already present — no changes made.");
        return;
    }

    std::wostringstream block;
    block << L"\n\n" << MARKER << L"\n";
    block << L"# Blocking known Microsoft telemetry endpoints\n";
    block << L"# Remove these lines to restore\n";
    for (const auto& d : GetBlockedDomains())
        block << L"0.0.0.0 " << d << L"\n";
    block << L"# ===== End RealSyferX Debloat =====\n";

    std::wofstream fout(HOSTS_PATH, std::ios::app);
    if (!fout.is_open()) {
        Utils::PrintError("Failed to open hosts file for writing.");
        return;
    }
    fout << block.str();
    fout.close();

    std::cout << "  [OK] Blocked " << GetBlockedDomains().size()
              << " telemetry domains in hosts file.\n";
    Utils::PrintSuccess("Hosts file updated. DNS cache flushed.");

    Utils::RunPowerShell(L"ipconfig /flushdns | Out-Null; Write-Host '  [OK] DNS cache flushed'");
}
