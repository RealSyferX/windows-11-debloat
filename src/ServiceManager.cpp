#include "ServiceManager.h"
#include "Utils.h"
#include <iostream>

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
    ControlService(svc_h, SERVICE_CONTROL_STOP, &st);
    for (int i = 0; i < 30 && st.dwCurrentState != SERVICE_STOPPED; i++) {
        Sleep(500);
        QueryServiceStatus(svc_h, &st);
    }
}

void ServiceManager::DisableAll() {
    Utils::PrintHeader("Disabling telemetry services...");
    for (const auto& s : GetTelemetryServices()) {
        SC_HANDLE scm, svc_h;
        if (!OpenSvc(s, SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG, scm, svc_h)) {
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
}

void ServiceManager::DeleteAll() {
    Utils::PrintHeader("Deleting telemetry services...");
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
}
