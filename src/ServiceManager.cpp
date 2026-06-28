#include "ServiceManager.h"
#include "Utils.h"
#include <iostream>
#include <fstream>

const std::vector<TelemetryService>& ServiceManager::GetTelemetryServices() {
    static const std::vector<TelemetryService> svcs = {
        { L"DiagTrack",
          "Connected User Experiences and Telemetry",
          "Core telemetry — sends usage/diagnostic data to Microsoft" },
        { L"dmwappushservice",
          "Device Mgmt WAP Push Service",
          "WAP push routing used for MDM + telemetry" },
        { L"WerSvc",
          "Windows Error Reporting",
          "Uploads crash/error data to Microsoft" },
        { L"wercplsupport",
          "Problem Reports Control Panel Support",
          "Back-end for Error Reporting UI" },
        { L"MapsBroker",
          "Downloaded Maps Manager",
          "Background map downloads (skip if no offline maps)" },
        { L"RetailDemo",
          "Retail Demo Service",
          "Store demo mode — useless on a personal PC" },
        { L"wisvc",
          "Windows Insider Service",
          "Insider preview telemetry (skip if on Insider)" },
        { L"PcaSvc",
          "Program Compatibility Assistant",
          "Collects app-compatibility telemetry" },
        { L"DiagSvc",
          "Diagnostic Execution Service",
          "Runs diagnostic scripts that collect data" },
        { L"fhsvc",
          "File History Service",
          "File History backup (skip if you use it)" },
        { L"XblAuthManager",
          "Xbox Live Auth Manager",
          "Xbox Live authentication (disable if not using Xbox)" },
        { L"XblGameSave",
          "Xbox Live Game Save",
          "Cloud save sync for Xbox games (disable if no Xbox)" },
        { L"XboxGipSvc",
          "Xbox Accessory Management",
          "Xbox accessory driver (disable if no Xbox controllers)" },
        { L"XboxNetApiSvc",
          "Xbox Live Networking",
          "Xbox Live network services (disable if not using Xbox)" },
        { L"WMPNetworkSvc",
          "WMP Network Sharing Service",
          "Media sharing via Windows Media Player" },
        { L"WpcMonSvc",
          "Parental Controls",
          "Parental controls monitoring (disable if not used)" },
        { L"RemoteRegistry",
          "Remote Registry",
          "Allows remote registry edits — security risk" },
        { L"PhoneSvc",
          "Phone Service",
          "Phone connection service (disable if no phone integration)" },
        { L"WwanSvc",
          "WWAN AutoConfig",
          "Cellular data auto-config (disable if no SIM)" },
    };
    return svcs;
}

void ServiceManager::List() {
    Utils::PrintHeader("Telemetry / spyware services targeted:");
    const auto& svcs = GetTelemetryServices();
    for (size_t i = 0; i < svcs.size(); i++)
        std::cout << "  [" << (i < 9 ? " " : "") << (i + 1) << "] "
                  << svcs[i].displayName << "  ("
                  << Utils::WideToString(svcs[i].name) << ")\n      "
                  << svcs[i].description << "\n";
    std::cout << "\n";
}

static bool OpenSvc(const TelemetryService& svc, DWORD access, SC_HANDLE& scm, SC_HANDLE& svc_h) {
    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        Utils::PrintError("Failed to open Service Control Manager.");
        return false;
    }
    svc_h = OpenServiceW(scm, svc.name.c_str(), access);
    if (!svc_h) {
        CloseServiceHandle(scm);
        return false;
    }
    return true;
}

static void StopAndWait(SC_HANDLE svc_h) {
    SERVICE_STATUS st;
    if (!QueryServiceStatus(svc_h, &st)) return;
    if (st.dwCurrentState == SERVICE_STOPPED) return;
    BOOL stopOk = ControlService(svc_h, SERVICE_CONTROL_STOP, &st);
    if (!stopOk) {
        // The service may not accept STOP controls, or the request was denied.
        // ControlService still updates st on some failures (e.g. a race where
        // the service stopped between our query and the control call), so if it
        // is already stopped there is nothing to wait for. Either way we bail
        // out: polling a state that won't change wastes up to 15 seconds per
        // service (up to ~285 s across the 19-service list).
        return;
    }
    for (int i = 0; i < 30 && st.dwCurrentState != SERVICE_STOPPED; i++) {
        Sleep(500);
        if (!QueryServiceStatus(svc_h, &st)) break;
    }
}

// Queries the current start type (SERVICE_BOOT_START, SERVICE_SYSTEM_START,
// SERVICE_AUTO_START, SERVICE_DEMAND_START, SERVICE_DISABLED) of an open
// service handle. Returns false if the query fails.
static bool QueryStartType(SC_HANDLE svc_h, DWORD& outStartType) {
    DWORD needed = 0;
    // First call always fails with ERROR_INSUFFICIENT_BUFFER but sets 'needed'.
    QueryServiceConfigW(svc_h, NULL, 0, &needed);
    if (needed == 0) return false;
    std::vector<BYTE> buf(needed);
    auto cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buf.data());
    if (!QueryServiceConfigW(svc_h, cfg, needed, &needed))
        return false;
    outStartType = cfg->dwStartType;
    return true;
}

void ServiceManager::DisableAll() {
    Utils::PrintHeader("Disabling telemetry services...");

    // Snapshot all original start types to the backup file atomically: write
    // to a .tmp file, then rename over the target. If the process crashes
    // mid-write, the original backup is preserved. The backup is only
    // replaced if the full write succeeds. EnableAll() reads this back.
    std::wstring backupPath = Utils::GetDebloatDataDir() + L"service_backup.txt";
    bool backupOk = Utils::WriteBackupAtomic(backupPath, [&](std::ofstream& backup) -> bool {
        for (const auto& s : GetTelemetryServices()) {
            SC_HANDLE scm, svc_h;
            if (!OpenSvc(s, SERVICE_QUERY_CONFIG, scm, svc_h))
                continue;  // service not found — no backup line
            DWORD origStart = 0;
            if (QueryStartType(svc_h, origStart))
                backup << Utils::WideToString(s.name) << "|" << origStart << "\n";
            CloseServiceHandle(svc_h);
            CloseServiceHandle(scm);
        }
        return true;
    });
    if (!backupOk)
        Utils::PrintWarning("Could not write service backup file — start types will not be saved.");

    for (const auto& s : GetTelemetryServices()) {
        SC_HANDLE scm, svc_h;
        if (!OpenSvc(s, SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG, scm, svc_h)) {
            std::cout << "  [--] " << s.displayName << " — not found\n";
            continue;
        }
        StopAndWait(svc_h);
        BOOL ok = ChangeServiceConfigW(svc_h, SERVICE_NO_CHANGE, SERVICE_DISABLED,
            SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        CloseServiceHandle(svc_h);
        CloseServiceHandle(scm);
        if (ok) std::cout << "  [OK] " << s.displayName << " — stopped & disabled\n";
        else    std::cout << "  [!!] " << s.displayName << " — disable failed (err "
                          << GetLastError() << ")\n";
    }
    Utils::PrintSuccess("Telemetry services disabled.");
    Utils::LogAction("SERVICE", "Disabled " +
        std::to_string(GetTelemetryServices().size()) + " telemetry services");
}

void ServiceManager::DeleteAll() {
    Utils::PrintHeader("Deleting telemetry services...");
    Utils::LogAction("SERVICE", "Deleting " +
        std::to_string(GetTelemetryServices().size()) + " telemetry services (aggressive)");
    for (const auto& s : GetTelemetryServices()) {
        SC_HANDLE scm, svc_h;
        if (!OpenSvc(s, SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | DELETE, scm, svc_h)) {
            std::cout << "  [--] " << s.displayName << " — not found\n";
            continue;
        }
        StopAndWait(svc_h);
        ChangeServiceConfigW(svc_h, SERVICE_NO_CHANGE, SERVICE_DISABLED,
            SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        BOOL ok = DeleteService(svc_h);
        CloseServiceHandle(svc_h);
        CloseServiceHandle(scm);
        if (ok) std::cout << "  [OK] " << s.displayName << " — marked for deletion (reboot to finish)\n";
        else    std::cout << "  [!!] " << s.displayName << " — delete failed (err "
                          << GetLastError() << ")\n";
    }
    Utils::PrintSuccess("Telemetry service deletion requested.");
    Utils::LogAction("SERVICE", "Service deletion requested");
}

// -- Pure backup parsing helper -----------------------------------------------
// Extracted from EnableAll() so the "name|startType" parsing logic can be unit
// tested without touching the SCM or the filesystem. Mirrors the pattern used
// by HostsManager (RemoveBlock/HasBlock), TelemetryManager (SplitEscaped/...),
// and PerformanceManager (ParseBackup).

ParsedServiceBackup ServiceManager::ParseServiceLine(const std::string& line) {
    ParsedServiceBackup result{ "", 0, false };

    // Tolerate CRLF line endings (matches PerformanceManager::ParseBackup).
    std::string trimmed = line;
    if (!trimmed.empty() && trimmed.back() == '\r') trimmed.pop_back();

    // Reject empty lines.
    if (trimmed.empty()) return result;

    // Find the pipe separator.
    size_t bar = trimmed.find('|');
    if (bar == std::string::npos) return result;

    std::string name = trimmed.substr(0, bar);
    std::string typeStr = trimmed.substr(bar + 1);

    // Reject empty service names — an empty name can never open a real service.
    if (name.empty()) return result;

    // Parse the start type. std::stoul throws std::invalid_argument on empty
    // or non-numeric input, and std::out_of_range on overflow; both are
    // rejected as malformed lines.
    DWORD startType = 0;
    try {
        startType = static_cast<DWORD>(std::stoul(typeStr));
    } catch (...) {
        return result;
    }

    result.name = name;
    result.startType = startType;
    result.valid = true;
    return result;
}

void ServiceManager::EnableAll() {
    Utils::PrintHeader("Re-enabling telemetry services from backup...");

    std::wstring backupPath = Utils::GetDebloatDataDir() + L"service_backup.txt";
    std::ifstream fin(backupPath);
    if (!fin.is_open()) {
        std::cout << "  [--] No service backup found — nothing to revert.\n";
        return;
    }

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        Utils::PrintError("Failed to open Service Control Manager.");
        return;
    }

    int restored = 0, notFound = 0, failed = 0;
    std::string line;
    while (std::getline(fin, line)) {
        auto parsed = ParseServiceLine(line);
        if (!parsed.valid) continue;

        DWORD startType = parsed.startType;
        std::string nameStr = parsed.name;
        std::wstring svcName = Utils::StringToWide(nameStr);
        SC_HANDLE svc_h = OpenServiceW(scm, svcName.c_str(),
            SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG);
        if (!svc_h) {
            std::cout << "  [--] " << nameStr << " — not found / no backup\n";
            ++notFound;
            continue;
        }
        BOOL ok = ChangeServiceConfigW(svc_h, SERVICE_NO_CHANGE, startType,
            SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (ok) {
            std::cout << "  [OK] " << nameStr << " — restored (start=" << startType << ")\n";
            ++restored;
            // Start the service now only if it was auto-start or boot-start;
            // don't start services that were originally disabled/manual.
            if (startType == SERVICE_AUTO_START || startType == SERVICE_BOOT_START)
                StartServiceW(svc_h, 0, NULL);
        } else {
            std::cout << "  [!!] " << nameStr << " — restore failed (err "
                      << GetLastError() << ")\n";
            ++failed;
        }
        CloseServiceHandle(svc_h);
    }
    fin.close();
    CloseServiceHandle(scm);

    std::cout << "\n  Summary: " << restored << " restored, " << notFound
              << " not found, " << failed << " failed.\n";
    Utils::LogAction("SERVICE", "Reverted " + std::to_string(restored) + " services from backup");
    if (failed == 0 && notFound == 0 && restored > 0)
        Utils::PrintSuccess("Telemetry services re-enabled from backup.");
    else if (restored > 0)
        Utils::PrintWarning("Service restore completed with some issues.");
    else
        Utils::PrintError("No services could be restored.");
}
