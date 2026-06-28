#pragma once
#include <string>
#include <vector>
#include <windows.h>

struct TelemetryService {
    std::wstring name;
    std::string  displayName;
    std::string  description;
};

// Parsed service backup entry.
struct ParsedServiceBackup {
    std::string name;       // service name
    DWORD startType;        // original start type (e.g., SERVICE_AUTO_START)
    bool valid;             // false if the line is malformed
};

class ServiceManager {
public:
    static const std::vector<TelemetryService>& GetTelemetryServices();
    static void List();
    static void DisableAll();
    static void DeleteAll();
    static void EnableAll();   // Revert: restore previously-disabled services

    // -- Backup format pure helper -------------------------------------------
    // Parses one line of the service backup file ("name|startType").
    // Handles CRLF, rejects empty lines, lines without a pipe, empty service
    // names, and non-numeric start types. Public static so unit tests can
    // exercise it directly (mirrors HostsManager::RemoveBlock/HasBlock,
    // TelemetryManager::SplitEscaped, and PerformanceManager::ParseBackup).
    // Returns {name, startType, true} on success, {_, 0, false} on failure.
    static ParsedServiceBackup ParseServiceLine(const std::string& line);
};
