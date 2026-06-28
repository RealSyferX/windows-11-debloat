#include "HostsManager.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

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

// MARKER and END_MARKER are defined as public static constexpr members in
// HostsManager.h so that unit tests can reference them directly.

// Returns the real hosts-file path using the actual system directory, so the
// tool works even when Windows is installed on a drive other than C: (common
// in dual-boot, enterprise imaging, and some OEM setups). std::ifstream /
// std::ofstream do not expand %SystemRoot%, so the path must be resolved here.
// Falls back to the classic C:\Windows\... path if GetSystemDirectoryW fails.
static std::wstring GetHostsPath() {
    wchar_t sysDir[MAX_PATH];
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0)
        return L"C:\\Windows\\System32\\drivers\\etc\\hosts"; // safe fallback
    return std::wstring(sysDir) + L"\\drivers\\etc\\hosts";
}

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

    const std::wstring hostsPath = GetHostsPath();

    // Read the hosts file as raw bytes (binary) — no codecvt translation.
    // The real hosts file is ANSI/UTF-8 narrow bytes, never UTF-16.
    std::string content;
    std::ifstream fin(hostsPath.c_str(), std::ios::binary);
    if (!fin) {
        // ifstream could not open the file. Distinguish between "file does
        // not exist" (safe: create a new one with just the telemetry block)
        // and "file exists but is locked/unreadable" (dangerous: the atomic
        // write would replace it with ONLY the telemetry block, destroying
        // all existing user/custom hosts entries). GetFileAttributesW checks
        // existence without opening the file. This mirrors Revert()'s safe
        // abort-on-read-failure behavior and avoids silent data loss.
        DWORD attrs = GetFileAttributesW(hostsPath.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            DWORD err = GetLastError();
            if (err == ERROR_FILE_NOT_FOUND) {
                // File doesn't exist — safe to proceed; the atomic write
                // will create a new hosts file with the telemetry block.
                Utils::PrintInfo("Hosts file not found. Creating a new one with telemetry block.");
            } else {
                // GetFileAttributesW failed for another reason (e.g. path
                // not found, access denied). Abort to avoid data loss.
                Utils::PrintError("Could not determine hosts file state. Aborting to prevent data loss.");
                return;
            }
        } else {
            // The file exists but ifstream could not open it (likely locked
            // by antivirus or another process). Abort rather than overwrite.
            Utils::PrintError("Hosts file exists but cannot be read. Aborting to prevent data loss.");
            return;
        }
        // content remains empty (safe: file genuinely doesn't exist).
    } else {
        content.assign((std::istreambuf_iterator<char>(fin)),
                        std::istreambuf_iterator<char>());
    }
    fin.close();

    if (HasBlock(content)) {
        std::cout << "  [--] Telemetry block already applied. Skipping.\n";
        Utils::PrintWarning("Hosts block already present — no changes made.");
        return;
    }

    std::ostringstream block;
    block << "\n\n" << MARKER << "\n";
    block << "# Blocking known Microsoft telemetry endpoints\n";
    block << "# Remove these lines to restore\n";
    for (const auto& d : GetBlockedDomains())
        block << "0.0.0.0 " << Utils::WideToString(d) << "\n";
    block << END_MARKER << "\n";

    // Build the full new content in memory, then write it atomically: the
    // write goes to a .tmp file in the same directory and is renamed over the
    // target via MoveFileExW. If the process crashes mid-write, the original
    // hosts file is preserved intact rather than left truncated/corrupt.
    std::string newContent = content + block.str();
    if (!Utils::WriteFileAtomic(hostsPath, newContent)) {
        Utils::PrintError("Failed to write hosts file.");
        return;
    }

    std::cout << "  [OK] Blocked " << GetBlockedDomains().size()
              << " telemetry domains in hosts file.\n";
    Utils::LogAction("HOSTS", "Blocked " +
        std::to_string(GetBlockedDomains().size()) + " telemetry domains");

    auto r = Utils::RunPowerShell(L"ipconfig /flushdns | Out-Null; Write-Host '  [OK] DNS cache flushed'");
    Utils::PrintPsResult(r,
        "Hosts file updated. DNS cache flushed.",
        "Hosts file updated, but PowerShell failed to flush DNS cache.");
}

void HostsManager::Revert() {
    Utils::PrintHeader("Reverting telemetry hosts block...");

    const std::wstring hostsPath = GetHostsPath();

    // Read the hosts file as raw bytes (binary) — consistent with Apply().
    std::ifstream fin(hostsPath.c_str(), std::ios::binary);
    if (!fin) {
        Utils::PrintError("Could not open hosts file for reading.");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(fin)),
                         std::istreambuf_iterator<char>());
    fin.close();

    // The block-removal logic lives in the pure, testable RemoveBlock helper.
    auto result = RemoveBlock(std::move(content));
    if (!result) {
        std::cout << "  [--] No block found — nothing to revert.\n";
        return;
    }

    // Write the result atomically: the new content goes to a .tmp file and is
    // renamed over the hosts file via MoveFileExW. The previous Revert() opened
    // the hosts file with std::ios::trunc and rewrote it in place — if the
    // process crashed, lost power, or the write partially failed mid-stream,
    // the hosts file was left truncated/corrupt, breaking DNS resolution
    // system-wide. The atomic rename guarantees the original file is either
    // fully replaced or left completely untouched.
    if (!Utils::WriteFileAtomic(hostsPath, *result)) {
        Utils::PrintError("Failed to write hosts file.");
        return;
    }

    std::cout << "  [OK] Hosts block reverted.\n";
    Utils::LogAction("HOSTS", "Reverted telemetry block");

    auto r = Utils::RunPowerShell(L"ipconfig /flushdns | Out-Null; Write-Host '  [OK] DNS cache flushed'");
    Utils::PrintPsResult(r,
        "Hosts block removed. DNS cache flushed.",
        "Hosts block removed, but PowerShell failed to flush DNS cache.");
}

std::optional<std::string> HostsManager::RemoveBlock(std::string content) {
    size_t start = content.find(MARKER);
    size_t end   = content.find(END_MARKER);

    // No start marker — nothing to remove. Also covers the case where only
    // the end marker is present (malformed block); we cannot safely remove
    // anything without a start marker.
    if (start == std::string::npos)
        return std::nullopt;

    if (end == std::string::npos) {
        // Truncated block — no end marker. Erase from start marker to EOF.
        content.erase(start);
    } else {
        // If the end marker appears before the start marker (corrupt/malformed
        // file), treat it as a truncated block — erase from start marker to EOF.
        // Without this guard, endLineEnd - removeStart would underflow (both
        // are size_t) and std::string::erase would throw std::out_of_range.
        if (end < start) {
            content.erase(start);
            return content;
        }

        // Both markers found — erase from start marker through the end of
        // the end-marker line (including its trailing newline).
        size_t endLineEnd = end + std::string(END_MARKER).size();
        if (endLineEnd < content.size() && content[endLineEnd] == '\n')
            endLineEnd += 1;

        // Also remove the preceding "\n\n" that Apply() prepended to the
        // block, so no stray blank lines are left behind.
        size_t removeStart = start;
        if (removeStart >= 2 &&
            content[removeStart - 1] == '\n' &&
            content[removeStart - 2] == '\n')
            removeStart -= 2;

        content.erase(removeStart, endLineEnd - removeStart);
    }

    return content;
}

bool HostsManager::HasBlock(const std::string& content) {
    return content.find(MARKER) != std::string::npos;
}
