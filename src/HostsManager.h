#pragma once
#include <optional>
#include <string>
#include <vector>

class HostsManager {
public:
    // Marker strings that delimit the managed telemetry block in the hosts
    // file. Public so unit tests can reference them directly (no duplication).
    // The hosts file is ANSI/UTF-8 narrow bytes, never UTF-16; /utf-8 is set
    // in CMake so the em dash encodes correctly.
    static constexpr const char* MARKER =
        "# ===== RealSyferX Debloat \xE2\x80\x94 Telemetry Block =====";
    static constexpr const char* END_MARKER =
        "# ===== End RealSyferX Debloat =====";

    static const std::vector<std::wstring>& GetBlockedDomains();
    static void List();
    static void Apply();
    static void Revert();

    // Returns the content with the managed telemetry block removed.
    // Returns std::nullopt if neither marker is found (nothing to do).
    // Handles truncated blocks (start present, end missing) by erasing
    // from start marker to EOF.
    static std::optional<std::string> RemoveBlock(std::string content);

    // Returns true if the managed telemetry block is present in content.
    static bool HasBlock(const std::string& content);
};
