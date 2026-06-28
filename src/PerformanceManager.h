#pragma once
#include <string>

// Parsed performance backup data.
struct PerfBackup {
    std::string hibernation;       // "on" or empty
    std::string hiberbootEnabled;  // original registry value or "missing"
    std::string powerPlan;         // original power plan GUID or empty
};

class PerformanceManager {
public:
    static void List();
    static void ApplyAll();
    static void Revert();   // Revert: re-enable hibernation, fast startup, restore power plan

    // -- Backup format pure helpers ------------------------------------------
    // These parse and serialize the pipe-delimited performance backup file
    // written by ApplyAll() and read by Revert(). Public static so unit tests
    // can exercise them directly (mirrors the HostsManager/TelemetryManager
    // pattern).
    //
    // Parses the full backup file content into a PerfBackup struct.
    // Handles CRLF, empty lines, and malformed lines gracefully.
    static PerfBackup ParseBackup(const std::string& content);

    // Serializes one key|value line for round-trip testing.
    static std::string FormatBackupLine(const std::string& key, const std::string& value);

    // Parses a GUID from powercfg /getactivescheme output. Extracted from
    // GetActivePowerPlanGuid() so the parsing logic can be unit tested.
    // Returns the 36-char GUID string, or empty string if not found/invalid.
    static std::string ParsePowerPlanGuid(const std::string& powercfgOutput);
};
