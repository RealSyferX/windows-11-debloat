#include "TelemetryManager.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

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

        /* ---- Explorer & UI ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
          L"HideFileExt", REG_TWEAK_DWORD, 0, L"",
          "Show file extensions in Explorer" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
          L"ShowCopilotButton", REG_TWEAK_DWORD, 0, L"",
          "Hide Copilot button from taskbar" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
          L"TaskbarDa", REG_TWEAK_DWORD, 0, L"",
          "Disable widgets panel on taskbar" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
          L"TaskbarMn", REG_TWEAK_DWORD, 0, L"",
          "Disable Meet Now on taskbar" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search",
          L"SearchboxTaskbarMode", REG_TWEAK_DWORD, 0, L"",
          "Hide search box on taskbar" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds",
          L"ShellFeedsTaskbarViewMode", REG_TWEAK_DWORD, 2, L"",
          "Disable news/feeds on taskbar" },

        /* ---- Copilot policy ---- */
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot",
          L"TurnOffWindowsCopilot", REG_TWEAK_DWORD, 1, L"",
          "Disable Windows Copilot (system-wide)" },

        /* ---- Dark mode ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          L"AppsUseLightTheme", REG_TWEAK_DWORD, 0, L"",
          "Dark mode for apps" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          L"SystemUsesLightTheme", REG_TWEAK_DWORD, 0, L"",
          "Dark mode for system" },

        /* ---- Background apps ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications",
          L"GlobalUserDisabled", REG_TWEAK_DWORD, 1, L"",
          "Disable background apps globally" },

        /* ---- Desktop icons ---- */
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel",
          L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", REG_TWEAK_DWORD, 0, L"",
          "Show This PC on desktop" },

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

// -- Backup helpers ----------------------------------------------------------
//
// ApplyAll() snapshots every registry value it is about to overwrite into a
// pipe-delimited text file so Revert() can restore each one exactly — writing
// the original bytes back, or deleting the value entirely if it did not exist
// before ApplyAll created it. This is the only correct way to undo user-setting
// overrides (HideFileExt, AppsUseLightTheme, TaskbarDa, ...) where the "default"
// is subjective and a blanket delete would discard the user's prior
// customization.

// Escape backslash and pipe so the field is safe inside the pipe-delimited
// record format. Registry paths/value-names do not normally contain pipes, but
// escaping makes the format unambiguous regardless.
static std::string EscapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '|') out += '\\';
        out += c;
    }
    return out;
}

// Splits a line on unescaped '|'. '\|' -> '|', '\\' -> '\' in output. Used to
// parse the backup records written by ApplyAll().
static std::vector<std::string> SplitEscaped(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    bool esc = false;
    for (char c : s) {
        if (esc) {
            if (c == '|')      cur += '|';
            else if (c == '\\') cur += '\\';
            else { cur += '\\'; cur += c; }   // unknown escape: keep literal
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == '|') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

static std::string HexEncode(const std::vector<BYTE>& data) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(data.size() * 2);
    for (BYTE b : data) {
        out += hex[b >> 4];
        out += hex[b & 0x0F];
    }
    return out;
}

static std::vector<BYTE> HexDecode(const std::string& hex) {
    std::vector<BYTE> out;
    out.reserve(hex.size() / 2);
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int hi = val(hex[i]);
        int lo = val(hex[i + 1]);
        if (hi < 0 || lo < 0) return out;   // malformed — return what we have
        out.push_back(static_cast<BYTE>((hi << 4) | lo));
    }
    return out;
}

void TelemetryManager::ApplyAll() {
    Utils::PrintHeader("Applying telemetry registry tweaks...");
    const auto& t = GetRegistryTweaks();

    // Open the backup file once (truncate) so each ApplyAll run produces a
    // fresh snapshot of the original values. Revert() reads this back.
    std::wstring backupPath = Utils::GetDebloatDataDir() + L"reg_backup.txt";
    std::ofstream backup(backupPath, std::ios::out | std::ios::trunc);
    if (!backup.is_open())
        Utils::PrintWarning("Could not open registry backup file — original values will not be saved.");

    int ok = 0;
    for (const auto& tw : t) {
        // --- Snapshot the current value BEFORE overwriting it ----------------
        // RegGetValueW opens the key read-only internally and returns the raw
        // type + bytes (RRF_NOEXPAND keeps REG_EXPAND_SZ unexpanded). A missing
        // value (ERROR_FILE_NOT_FOUND / ERROR_PATH_NOT_FOUND) is recorded as
        // "did not exist" so Revert() can delete the value we are about to
        // create, restoring Microsoft's default.
        if (backup.is_open()) {
            DWORD origType = 0;
            DWORD origSize = 0;
            LONG qr = RegGetValueW(tw.rootKey, tw.subKey.c_str(),
                tw.valueName.c_str(), RRF_RT_ANY | RRF_NOEXPAND,
                &origType, NULL, &origSize);
            bool existed = (qr == ERROR_SUCCESS || qr == ERROR_MORE_DATA);
            DWORD actualType = 0;
            std::vector<BYTE> origData;
            if (existed) {
                actualType = origType;
                if (origSize > 0) {
                    origData.resize(origSize);
                    DWORD sz = origSize;
                    LONG r2 = RegGetValueW(tw.rootKey, tw.subKey.c_str(),
                        tw.valueName.c_str(), RRF_RT_ANY | RRF_NOEXPAND,
                        &actualType, origData.data(), &sz);
                    if (r2 != ERROR_SUCCESS) {
                        existed = false;     // size query ok but data read failed
                        origData.clear();
                        actualType = 0;
                    } else {
                        origData.resize(sz);
                    }
                }
            }
            // Record format: rootKeyNum|subKey|valueName|existed|type|dataLen|dataHex
            backup << reinterpret_cast<uintptr_t>(tw.rootKey)
                   << "|" << EscapeField(Utils::WideToString(tw.subKey))
                   << "|" << EscapeField(Utils::WideToString(tw.valueName))
                   << "|" << (existed ? 1 : 0)
                   << "|" << (existed ? actualType : 0)
                   << "|" << origData.size()
                   << "|" << HexEncode(origData)
                   << "\n";
        }

        // --- Apply the tweak (existing logic) --------------------------------
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
    if (backup.is_open()) {
        backup.flush();
        backup.close();
    }
    Utils::PrintSuccess("Applied " + std::to_string(ok) + "/" +
        std::to_string(t.size()) + " registry tweaks.");
    Utils::LogAction("REGISTRY", "Applied " + std::to_string(ok) + "/" +
        std::to_string(t.size()) + " registry tweaks");
}

void TelemetryManager::Revert() {
    Utils::PrintHeader("Reverting registry tweaks from backup...");

    std::wstring backupPath = Utils::GetDebloatDataDir() + L"reg_backup.txt";
    std::ifstream fin(backupPath);
    if (!fin.is_open()) {
        std::cout << "  [--] No registry backup found — nothing to revert.\n";
        return;
    }

    int restored = 0, deleted = 0, failed = 0;
    std::string line;
    while (std::getline(fin, line)) {
        // Tolerate CRLF line endings.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::vector<std::string> fields = SplitEscaped(line);
        if (fields.size() != 7) continue;          // malformed record — skip

        // rootKeyNum -> HKEY. The predefined HKEY_* handles are stable numeric
        // constants (e.g. HKEY_CURRENT_USER == 0x80000001), so the round-trip
        // reinterpret_cast is exact for every key this tool touches.
        uintptr_t rootKeyNum = 0;
        try {
            rootKeyNum = static_cast<uintptr_t>(std::stoull(fields[0]));
        } catch (...) { continue; }
        HKEY rootKey = reinterpret_cast<HKEY>(rootKeyNum);

        std::wstring subKey    = Utils::StringToWide(fields[1]);
        std::wstring valueName = Utils::StringToWide(fields[2]);

        int existed = 0;
        DWORD type = 0, dataLen = 0;
        try {
            existed = std::stoi(fields[3]);
            type    = static_cast<DWORD>(std::stoul(fields[4]));
            dataLen = static_cast<DWORD>(std::stoul(fields[5]));
        } catch (...) { continue; }

        std::vector<BYTE> data = HexDecode(fields[6]);

        std::string nameNarrow = Utils::WideToString(valueName);

        // Open (creating the key if needed) so the value can be written or
        // deleted. ApplyAll uses RegCreateKeyExW for the same reason.
        HKEY hKey = NULL;
        LONG r = RegCreateKeyExW(rootKey, subKey.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
        if (r != ERROR_SUCCESS) {
            std::cout << "  [!!] " << nameNarrow << " — open key failed (err " << r << ")\n";
            ++failed;
            continue;
        }

        if (existed) {
            // Write the original bytes back with the original type.
            const BYTE* p = data.empty() ? nullptr : data.data();
            r = RegSetValueExW(hKey, valueName.c_str(), 0, type,
                p, static_cast<DWORD>(data.size()));
            if (r == ERROR_SUCCESS) {
                std::cout << "  [OK] " << nameNarrow << "\n";
                ++restored;
            } else {
                std::cout << "  [!!] " << nameNarrow << " — restore failed (err " << r << ")\n";
                ++failed;
            }
        } else {
            // The value did not exist before ApplyAll — delete what we created.
            r = RegDeleteValueW(hKey, valueName.c_str());
            if (r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND) {
                std::cout << "  [OK] " << nameNarrow << " (deleted)\n";
                ++deleted;
            } else {
                std::cout << "  [!!] " << nameNarrow << " — delete failed (err " << r << ")\n";
                ++failed;
            }
        }
        RegCloseKey(hKey);
    }
    fin.close();

    std::cout << "\n  Summary: " << restored << " restored, " << deleted
              << " deleted, " << failed << " failed.\n";
    Utils::LogAction("REGISTRY", "Reverted " + std::to_string(restored + deleted) +
        " registry tweaks from backup");
    if (failed == 0 && (restored + deleted) > 0)
        Utils::PrintSuccess("Registry tweaks reverted from backup.");
    else if (restored + deleted > 0)
        Utils::PrintWarning("Registry revert completed with some issues.");
    else
        Utils::PrintError("No registry values could be restored.");
}
