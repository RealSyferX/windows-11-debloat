#include "TelemetryManager.h"
#include "Utils.h"
#include <iostream>

const std::vector<RegistryTweak>& TelemetryManager::GetRegistryTweaks() {
    static const std::vector<RegistryTweak> t = {
        /* ---- core telemetry ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection",
          L"AllowTelemetry", REG_TWEAK_DWORD, 0, L"",
          "Disable Windows telemetry (AllowTelemetry = 0)" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection",
          L"AllowTelemetry", REG_TWEAK_DWORD, 0, L"",
          "Disable telemetry (secondary policy key)" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection",
          L"DoNotShowFeedbackNotifications", REG_TWEAK_DWORD, 1, L"",
          "Hide feedback/telemetry notifications" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection",
          L"DisableOneSettingsClient", REG_TWEAK_DWORD, 1, L"",
          "Disable OneSettings sync (cloud config pull)" },

        /* ---- advertising ID ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo",
          L"Enabled", REG_TWEAK_DWORD, 0, L"",
          "Disable advertising ID (per-user)" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AdvertisingInfo",
          L"DisabledByGroupPolicy", REG_TWEAK_DWORD, 1, L"",
          "Disable advertising ID (system-wide)" },

        /* ---- Cortana / search ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
          L"AllowCortana", REG_TWEAK_DWORD, 0, L"",
          "Disable Cortana" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
          L"AllowCloudSearch", REG_TWEAK_DWORD, 0, L"",
          "Disable cloud search" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
          L"AllowSearchToUseLocation", REG_TWEAK_DWORD, 0, L"",
          "Stop search from using location" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
          L"BingSearchEnabled", REG_TWEAK_DWORD, 0, L"",
          "Disable Bing web search in Start Menu" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
          L"CortanaConsent", REG_TWEAK_DWORD, 0, L"",
          "Revoke Cortana consent" },

        /* ---- compatibility telemetry ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat",
          L"AITEnable", REG_TWEAK_DWORD, 0, L"",
          "Disable Application Impact Telemetry" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat",
          L"DisableInventory", REG_TWEAK_DWORD, 1, L"",
          "Disable compatibility inventory collector" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat",
          L"DisableUAR", REG_TWEAK_DWORD, 1, L"",
          "Disable steps recorder upload" },

        /* ---- cloud content / consumer ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent",
          L"DisableWindowsConsumerFeatures", REG_TWEAK_DWORD, 1, L"",
          "Disable consumer features (suggested apps)" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent",
          L"DisableSoftLanding", REG_TWEAK_DWORD, 1, L"",
          "Disable Windows tips" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent",
          L"DisableConsumerTools", REG_TWEAK_DWORD, 1, L"",
          "Disable consumer tools in Settings" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager",
          L"SubscribedContent-310093Enabled", REG_TWEAK_DWORD, 0, L"",
          "Disable suggested apps" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager",
          L"SubscribedContent-338388Enabled", REG_TWEAK_DWORD, 0, L"",
          "Disable Spotlight tips" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager",
          L"SubscribedContent-338389Enabled", REG_TWEAK_DWORD, 0, L"",
          "Disable suggested apps on Start" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager",
          L"SystemPaneSuggestionsEnabled", REG_TWEAK_DWORD, 0, L"",
          "Disable system pane suggestions" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager",
          L"SilentInstalledAppsEnabled", REG_TWEAK_DWORD, 0, L"",
          "Disable silent app installation" },

        /* ---- input personalization ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\InputPersonalization",
          L"RestrictImplicitInkCollection", REG_TWEAK_DWORD, 1, L"",
          "Disable implicit ink collection" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\InputPersonalization",
          L"RestrictImplicitTextCollection", REG_TWEAK_DWORD, 1, L"",
          "Disable implicit text collection" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore",
          L"HarvestContacts", REG_TWEAK_DWORD, 0, L"",
          "Stop harvesting contacts for typing personalization" },

        /* ---- activity history ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System",
          L"EnableActivityFlighting", REG_TWEAK_DWORD, 0, L"",
          "Disable activity flighting" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System",
          L"PublishUserActivities", REG_TWEAK_DWORD, 0, L"",
          "Stop publishing user activities to cloud" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System",
          L"UploadUserActivities", REG_TWEAK_DWORD, 0, L"",
          "Stop uploading user activities" },

        /* ---- Start tracking ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
          L"Start_TrackProgs", REG_TWEAK_DWORD, 0, L"",
          "Disable Start menu tracking of most-used programs" },

        /* ---- feedback frequency ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Siuf\\Client",
          L"PeriodInNanoSeconds", REG_TWEAK_DWORD, 0, L"",
          "Set feedback prompt frequency to never" },

        /* ---- handwriting ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\TabletPC",
          L"PreventHandwritingData", REG_TWEAK_DWORD, 1, L"",
          "Prevent handwriting data sharing" },

        /* ---- Wi-Fi Sense ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\config",
          L"AutoConnectAllowedOEM", REG_TWEAK_DWORD, 0, L"",
          "Disable Wi-Fi Sense auto-connect" },

        /* ---- OneDrive block ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive",
          L"DisableFileSyncNGSC", REG_TWEAK_DWORD, 1, L"",
          "Disable OneDrive sync (prevent reinstall/re-enable)" },
    };
    return t;
}

void TelemetryManager::List() {
    Utils::PrintHeader("Registry tweaks (telemetry / privacy):");
    const auto& t = GetRegistryTweaks();
    for (size_t i = 0; i < t.size(); i++)
        std::cout << "  [" << (i < 9 ? " " : "") << (i + 1) << "] "
                  << t[i].description << "\n";
    std::cout << "\n";
}

void TelemetryManager::ApplyAll() {
    Utils::PrintHeader("Applying telemetry registry tweaks...");
    const auto& t = GetRegistryTweaks();
    int ok = 0;
    for (const auto& tw : t) {
        HKEY hKey = NULL;
        DWORD disp = 0;
        LONG r = RegCreateKeyExW(tw.rootKey, tw.subKey.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
            NULL, &hKey, &disp);
        if (r != ERROR_SUCCESS) {
            std::cout << "  [!!] " << tw.description << " — open key failed (err " << r << ")\n";
            continue;
        }
        if (tw.type == REG_TWEAK_DWORD) {
            r = RegSetValueExW(hKey, tw.valueName.c_str(), 0, REG_DWORD,
                reinterpret_cast<const BYTE*>(&tw.dwordData), sizeof(tw.dwordData));
        } else {
            r = RegSetValueExW(hKey, tw.valueName.c_str(), 0, REG_SZ,
                reinterpret_cast<const BYTE*>(tw.stringData.c_str()),
                static_cast<DWORD>((tw.stringData.size() + 1) * sizeof(wchar_t)));
        }
        RegCloseKey(hKey);
        if (r == ERROR_SUCCESS) { std::cout << "  [OK] " << tw.description << "\n"; ok++; }
        else                     std::cout << "  [!!] " << tw.description << " — set failed (err " << r << ")\n";
    }
    Utils::PrintSuccess("Applied " + std::to_string(ok) + "/" +
        std::to_string(t.size()) + " registry tweaks.");
}
