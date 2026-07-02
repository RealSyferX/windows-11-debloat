// tests/test_datatables.cpp
//
// Unit tests for the data-driven design (bloatware list, telemetry services,
// scheduled tasks, blocked domains, registry tweaks).
//
// Uses a minimal hand-rolled assert framework - no external test dependency.
// Built only when the CMake option BUILD_TESTING=ON is set.
//
// The tests verify three properties of each data table:
//   1. Count  - the number of entries matches the expected value (which must
//               be kept in sync with the source and the README).
//   2. No duplicates - no two entries share the same identifying key.
//   3. Data validity  - every field that should be non-empty is non-empty,
//                       and domains contain at least one dot.
//
// If a test fails, investigate the underlying data - do NOT weaken the test.

#include "AppxManager.h"
#include "ServiceManager.h"
#include "ScheduledTaskManager.h"
#include "HostsManager.h"
#include "TelemetryManager.h"
#include "PerformanceManager.h"
#include "Utils.h"

#include <iostream>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::cerr << "FAIL: " #cond " (line " << __LINE__ << ")\n"; \
        ++g_failures; \
    } \
} while(0)

// -- Bloatware apps ----------------------------------------------------------

static void TestBloatwareList() {
    const auto& apps = AppxManager::GetBloatwareList();

    // Count - must match the README and the source table.
    CHECK(apps.size() == 39);

    // No duplicate name patterns (the key used for Appx removal).
    std::set<std::wstring> seenNames;
    for (const auto& a : apps)
        seenNames.insert(a.namePattern);
    CHECK(seenNames.size() == apps.size());

    // Data validity: every field must be non-empty.
    for (const auto& a : apps) {
        CHECK(!a.namePattern.empty());
        CHECK(!a.displayName.empty());
    }
}

// -- Telemetry services ------------------------------------------------------

static void TestTelemetryServices() {
    const auto& svcs = ServiceManager::GetTelemetryServices();

    // Count
    CHECK(svcs.size() == 19);

    // No duplicate service names
    std::set<std::wstring> seenNames;
    for (const auto& s : svcs)
        seenNames.insert(s.name);
    CHECK(seenNames.size() == svcs.size());

    // Data validity
    for (const auto& s : svcs) {
        CHECK(!s.name.empty());
        CHECK(!s.displayName.empty());
        CHECK(!s.description.empty());
    }
}

// -- Scheduled tasks ---------------------------------------------------------

static void TestScheduledTasks() {
    const auto& tasks = ScheduledTaskManager::GetTasks();

    // Count
    CHECK(tasks.size() == 17);

    // No duplicate (name, path) pairs - task identity is (TaskPath, TaskName),
    // so the same name in different paths is valid.
    std::set<std::pair<std::wstring, std::wstring>> seenPairs;
    for (const auto& t : tasks)
        seenPairs.insert({ t.name, t.path });
    CHECK(seenPairs.size() == tasks.size());

    // Data validity
    for (const auto& t : tasks) {
        CHECK(!t.name.empty());
        CHECK(!t.path.empty());
        CHECK(!t.description.empty());
    }
}

// -- Blocked domains ---------------------------------------------------------

static void TestBlockedDomains() {
    const auto& domains = HostsManager::GetBlockedDomains();

    // Count
    CHECK(domains.size() == 29);

    // No duplicate domains
    std::set<std::wstring> seenDomains(domains.begin(), domains.end());
    CHECK(seenDomains.size() == domains.size());

    // Data validity: non-empty and contains at least one dot.
    for (const auto& d : domains) {
        CHECK(!d.empty());
        CHECK(d.find(L'.') != std::wstring::npos);
    }
}

// -- Registry tweaks ---------------------------------------------------------

static void TestRegistryTweaks() {
    const auto& tweaks = TelemetryManager::GetRegistryTweaks();

    // Count
    CHECK(tweaks.size() == 44);

    // No duplicate (subKey, valueName) pairs - the same value name in a
    // different sub-key is a legitimate separate tweak.
    std::set<std::pair<std::wstring, std::wstring>> seenPairs;
    for (const auto& tw : tweaks)
        seenPairs.insert({ tw.subKey, tw.valueName });
    CHECK(seenPairs.size() == tweaks.size());

    // Data validity: subKey and valueName must be non-empty.
    for (const auto& tw : tweaks) {
        CHECK(!tw.subKey.empty());
        CHECK(!tw.valueName.empty());
        CHECK(!tw.description.empty());
    }
}

// -- PowerShell single-quote escaping --------------------------------------

static void TestEscapePsSingleQuote() {
    // Apostrophe in the middle must be doubled.
    CHECK(Utils::EscapePsSingleQuote(L"foo'bar") == L"foo''bar");
    // Apostrophe-free string is unchanged.
    CHECK(Utils::EscapePsSingleQuote(L"noquotes") == L"noquotes");
    // Leading single quote.
    CHECK(Utils::EscapePsSingleQuote(L"'leading") == L"''leading");
    // Trailing single quote.
    CHECK(Utils::EscapePsSingleQuote(L"trailing'") == L"trailing''");
    // Two quotes -> each doubled -> four single quotes in output.
    CHECK(Utils::EscapePsSingleQuote(L"''") == L"''''");
    // Empty string is unchanged.
    CHECK(Utils::EscapePsSingleQuote(L"") == L"");
}

// -- HostsManager block removal ---------------------------------------------
// Tests the pure string-manipulation logic extracted from Revert/Apply.
// The marker constants are public static constexpr members of HostsManager,
// so we reference them directly (no duplication).

static void TestHostsRemoveBlock() {
    // Normal block: both markers present. The content simulates a real hosts
    // file where the original content ends with \n, and Apply appended \n\n
    // before the marker, giving \n\n\n before MARKER. RemoveBlock erases the
    // \n\n (from Apply) + the block, leaving the original \n intact.
    {
        std::string content = std::string("pre\n\n\n") +
            HostsManager::MARKER + "\n0.0.0.0 x\n" +
            HostsManager::END_MARKER + "\npost";
        auto result = HostsManager::RemoveBlock(content);
        CHECK(result.has_value());
        CHECK(*result == "pre\npost");
    }

    // Truncated block: start marker present, end marker missing.
    {
        std::string content = std::string("pre\n") +
            HostsManager::MARKER + "\n0.0.0.0 x\n";
        auto result = HostsManager::RemoveBlock(content);
        CHECK(result.has_value());
        CHECK(*result == "pre\n");
    }

    // No markers at all — nothing to do.
    {
        std::string content = "plain hosts file\n";
        auto result = HostsManager::RemoveBlock(content);
        CHECK(!result.has_value());
    }

    // Start marker only at EOF — content is just the marker.
    {
        std::string content = HostsManager::MARKER;
        auto result = HostsManager::RemoveBlock(content);
        CHECK(result.has_value());
        CHECK(*result == "");
    }

    // Block at very start (no preceding \n\n) — must not underflow.
    {
        std::string content = std::string(HostsManager::MARKER) +
            "\n0.0.0.0 x\n" + HostsManager::END_MARKER + "\npost";
        auto result = HostsManager::RemoveBlock(content);
        CHECK(result.has_value());
        CHECK(*result == "post");
    }

    // Idempotency: RemoveBlock(RemoveBlock(x)) — second call returns nullopt
    // because the block has already been removed.
    {
        std::string content = std::string("pre\n\n\n") +
            HostsManager::MARKER + "\n0.0.0.0 x\n" +
            HostsManager::END_MARKER + "\npost";
        auto first = HostsManager::RemoveBlock(content);
        CHECK(first.has_value());
        auto second = HostsManager::RemoveBlock(*first);
        CHECK(!second.has_value());
    }

    // End marker before start marker (corrupt file) — must not crash,
    // must erase from start marker to EOF.
    {
        std::string content = std::string("pre\n") +
            HostsManager::END_MARKER + "\nmiddle\n" +
            HostsManager::MARKER + "\n0.0.0.0 x\n";
        auto result = HostsManager::RemoveBlock(content);
        CHECK(result.has_value());
        CHECK(*result == "pre\n" + std::string(HostsManager::END_MARKER) + "\nmiddle\n");
    }
}

static void TestHostsHasBlock() {
    // MARKER present -> true.
    {
        std::string content = std::string("0.0.0.0 x\n") +
            HostsManager::MARKER + "\n0.0.0.0 y\n";
        CHECK(HostsManager::HasBlock(content));
    }

    // MARKER absent -> false.
    {
        std::string content = "plain hosts file\n0.0.0.0 localhost\n";
        CHECK(!HostsManager::HasBlock(content));
    }

    // Empty string -> false.
    {
        CHECK(!HostsManager::HasBlock(""));
    }
}

// -- Utils string conversion round-trip --------------------------------------

static void TestStringRoundTrip() {
    // Empty string round-trips to empty.
    CHECK(Utils::WideToString(Utils::StringToWide("")) == "");

    // ASCII string round-trips correctly.
    CHECK(Utils::WideToString(Utils::StringToWide("hello world")) == "hello world");

    // Multi-byte UTF-8 (em dash, U+2014, encoded as E2 80 94) round-trips.
    {
        std::string emDash = "\xE2\x80\x94";
        std::wstring wide = Utils::StringToWide(emDash);
        CHECK(wide.size() == 1);
        CHECK(wide[0] == L'\x2014');
        CHECK(Utils::WideToString(wide) == emDash);
    }
}

// -- TelemetryManager backup/restore pure functions --------------------------
// Tests the four helpers (EscapeField, SplitEscaped, HexEncode, HexDecode)
// that parse and serialize the registry backup file written by ApplyAll() and
// read by Revert(). A bug in any of them would silently corrupt the backup or
// restore the wrong registry values. These were extracted to public static
// methods (mirroring HostsManager::RemoveBlock/HasBlock) so they can be tested
// in isolation without touching the registry or the filesystem.

static void TestEscapeFieldRoundTrip() {
    // Single-field round-trip: SplitEscaped(EscapeField(s))[0] must recover s.
    // Backslash is the escape character, pipe is the field separator, so both
    // must survive a full encode -> decode cycle.
    const std::string cases[] = {
        "",                                               // empty
        "hello",                                          // plain ASCII
        "a\\b",                                           // backslash
        "a|b",                                            // pipe
        "a\\|b",                                          // backslash + pipe
        "a\\\\|b",                                        // two backslashes then pipe
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",   // registry path
        "|",                                              // lone pipe
        "\\",                                             // lone backslash
        "no special chars here 12345"                    // boring text
    };
    for (const auto& s : cases) {
        auto parsed = TelemetryManager::SplitEscaped(TelemetryManager::EscapeField(s));
        CHECK(parsed.size() == 1);
        CHECK(parsed[0] == s);
    }

    // Multi-field record round-trip. Build three escaped fields joined by '|'
    // and verify SplitEscaped recovers each one exactly.
    {
        const std::string f0 = "first\\field";
        const std::string f1 = "second|field";
        const std::string f2 = "third\\|field";
        std::string record = TelemetryManager::EscapeField(f0) + "|" +
                             TelemetryManager::EscapeField(f1) + "|" +
                             TelemetryManager::EscapeField(f2);
        auto parsed = TelemetryManager::SplitEscaped(record);
        CHECK(parsed.size() == 3);
        CHECK(parsed[0] == f0);
        CHECK(parsed[1] == f1);
        CHECK(parsed[2] == f2);
    }
}

static void TestHexEncodeDecodeRoundTrip() {
    // Empty input -> empty hex, and decodes back to empty.
    {
        std::vector<BYTE> data;
        CHECK(TelemetryManager::HexEncode(data) == "");
        CHECK(TelemetryManager::HexDecode(TelemetryManager::HexEncode(data)) == data);
    }
    // Single byte 0x00.
    {
        std::vector<BYTE> data = { 0x00 };
        CHECK(TelemetryManager::HexEncode(data) == "00");
        CHECK(TelemetryManager::HexDecode(TelemetryManager::HexEncode(data)) == data);
    }
    // Single byte 0xFF.
    {
        std::vector<BYTE> data = { 0xFF };
        CHECK(TelemetryManager::HexEncode(data) == "FF");
        CHECK(TelemetryManager::HexDecode(TelemetryManager::HexEncode(data)) == data);
    }
    // All 256 byte values 0x00..0xFF.
    {
        std::vector<BYTE> data;
        for (int i = 0; i < 256; ++i)
            data.push_back(static_cast<BYTE>(i));
        CHECK(TelemetryManager::HexDecode(TelemetryManager::HexEncode(data)) == data);
    }
    // A long pseudo-random-ish sequence.
    {
        std::vector<BYTE> data;
        for (int i = 0; i < 1000; ++i)
            data.push_back(static_cast<BYTE>(i * 31 + 7));
        CHECK(TelemetryManager::HexDecode(TelemetryManager::HexEncode(data)) == data);
    }
    // HexDecode also accepts lowercase a-f (HexDecode is case-insensitive).
    {
        auto out = TelemetryManager::HexDecode("deadbeef");
        CHECK(out.size() == 4);
        CHECK(out[0] == 0xDE);
        CHECK(out[1] == 0xAD);
        CHECK(out[2] == 0xBE);
        CHECK(out[3] == 0xEF);
    }
}

static void TestHexDecodeMalformed() {
    // Empty input -> empty output.
    CHECK(TelemetryManager::HexDecode("").empty());

    // Odd length ("A"): the loop condition (i+1 < size) never holds for the
    // dangling char, so nothing is decoded.
    CHECK(TelemetryManager::HexDecode("A").empty());

    // Invalid hex chars ("XYZ"): 'X' is invalid on the very first pair, so the
    // decoder bails out immediately and returns empty.
    CHECK(TelemetryManager::HexDecode("XYZ").empty());

    // Partial decode: the first pair "AB" is valid, the second pair "XY" is
    // invalid, so the decoder returns the one byte it managed to decode.
    {
        auto out = TelemetryManager::HexDecode("ABXY");
        CHECK(out.size() == 1);
        CHECK(out[0] == 0xAB);
    }

    // Odd length after a valid pair ("AB1"): "AB" decodes, then the lone '1'
    // is ignored by the loop bound.
    {
        auto out = TelemetryManager::HexDecode("AB1");
        CHECK(out.size() == 1);
        CHECK(out[0] == 0xAB);
    }
}

static void TestSplitEscapedEdgeCases() {
    // Empty string -> one empty field (the final push_back always runs).
    {
        auto out = TelemetryManager::SplitEscaped("");
        CHECK(out.size() == 1);
        CHECK(out[0] == "");
    }

    // Single field, no pipe -> one field equal to the input.
    {
        auto out = TelemetryManager::SplitEscaped("hello");
        CHECK(out.size() == 1);
        CHECK(out[0] == "hello");
    }

    // Plain pipe-delimited fields, no escapes.
    {
        auto out = TelemetryManager::SplitEscaped("a|b|c");
        CHECK(out.size() == 3);
        CHECK(out[0] == "a");
        CHECK(out[1] == "b");
        CHECK(out[2] == "c");
    }

    // Escaped pipe "\|" -> literal '|' inside the field, not a separator.
    {
        auto out = TelemetryManager::SplitEscaped("a\\|b");
        CHECK(out.size() == 1);
        CHECK(out[0] == "a|b");
    }

    // Escaped backslash "\\" -> literal '\' inside the field.
    {
        auto out = TelemetryManager::SplitEscaped("a\\\\b");
        CHECK(out.size() == 1);
        CHECK(out[0] == "a\\b");
    }

    // Unknown escape "\x" -> keeps the backslash literally, then 'x'.
    {
        auto out = TelemetryManager::SplitEscaped("a\\xb");
        CHECK(out.size() == 1);
        CHECK(out[0] == "a\\xb");
    }

    // Trailing backslash at end of line -> consumed as an incomplete escape;
    // the backslash is dropped from the final field.
    {
        auto out = TelemetryManager::SplitEscaped("abc\\");
        CHECK(out.size() == 1);
        CHECK(out[0] == "abc");
    }

    // Trailing backslash right after a pipe -> two fields, second empty.
    {
        auto out = TelemetryManager::SplitEscaped("abc|\\");
        CHECK(out.size() == 2);
        CHECK(out[0] == "abc");
        CHECK(out[1] == "");
    }

    // Trailing pipe -> two fields, second empty.
    {
        auto out = TelemetryManager::SplitEscaped("a|");
        CHECK(out.size() == 2);
        CHECK(out[0] == "a");
        CHECK(out[1] == "");
    }

    // Lone pipe -> two empty fields.
    {
        auto out = TelemetryManager::SplitEscaped("|");
        CHECK(out.size() == 2);
        CHECK(out[0] == "");
        CHECK(out[1] == "");
    }
}

static void TestFullBackupRecordRoundTrip() {
    // Simulate exactly what ApplyAll writes for one registry value, including
    // a sub-key path full of backslashes and a value name that also contains a
    // backslash (registry paths use them). Then parse it back like Revert does
    // and verify every field — including the hex-encoded original bytes.
    const std::string rootKeyNum = "2147483649";  // 0x80000001 == HKEY_CURRENT_USER
    const std::string subKey     = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    const std::string valueName  = "Some\\Value\\With\\Backslash";
    const std::string existed    = "1";
    const std::string type       = "4";   // REG_DWORD
    const std::vector<BYTE> origData = { 0x01, 0x00, 0x00, 0x00 };  // DWORD 1, little-endian
    const std::string dataLen    = "4";
    const std::string dataHex    = TelemetryManager::HexEncode(origData);

    // Record format: rootKeyNum|subKey|valueName|existed|type|dataLen|dataHex
    std::string record =
        TelemetryManager::EscapeField(rootKeyNum) + "|" +
        TelemetryManager::EscapeField(subKey) + "|" +
        TelemetryManager::EscapeField(valueName) + "|" +
        TelemetryManager::EscapeField(existed) + "|" +
        TelemetryManager::EscapeField(type) + "|" +
        TelemetryManager::EscapeField(dataLen) + "|" +
        TelemetryManager::EscapeField(dataHex);

    auto f = TelemetryManager::SplitEscaped(record);
    CHECK(f.size() == 7);
    CHECK(f[0] == rootKeyNum);
    CHECK(f[1] == subKey);
    CHECK(f[2] == valueName);
    CHECK(f[3] == existed);
    CHECK(f[4] == type);
    CHECK(f[5] == dataLen);
    CHECK(f[6] == dataHex);

    // The dataHex field must round-trip through HexDecode back to the bytes.
    CHECK(TelemetryManager::HexDecode(f[6]) == origData);

    // Revert() skips any line whose SplitEscaped field count is not 7. Verify
    // a short record is rejected by that guard.
    auto bad = TelemetryManager::SplitEscaped("a|b|c");
    CHECK(bad.size() != 7);
}

// -- PerformanceManager backup parsing ----------------------------------------
// Tests the pure parsing logic extracted from Revert(). The backup file is
// pipe-delimited (key|value lines); ParseBackup turns the full content into a
// PerfBackup struct. FormatBackupLine serializes one line for round-trip
// testing. These were extracted to public static methods (mirroring
// HostsManager::RemoveBlock/HasBlock and TelemetryManager::SplitEscaped/...)
// so they can be tested without touching the filesystem or registry.

static void TestPerfBackupParse() {
    // Normal three-line backup: all three keys present and recovered.
    {
        std::string content =
            PerformanceManager::FormatBackupLine("hibernation", "on") +
            PerformanceManager::FormatBackupLine("hiberbootEnabled", "1") +
            PerformanceManager::FormatBackupLine("powerPlan", "381b4222-f694-41f0-9685-ff5bb260df2e");
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hibernation == "on");
        CHECK(p.hiberbootEnabled == "1");
        CHECK(p.powerPlan == "381b4222-f694-41f0-9685-ff5bb260df2e");
    }

    // CRLF line endings: same content with \r\n -> still parses correctly.
    {
        std::string content =
            "hibernation|on\r\n"
            "hiberbootEnabled|1\r\n"
            "powerPlan|381b4222-f694-41f0-9685-ff5bb260df2e\r\n";
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hibernation == "on");
        CHECK(p.hiberbootEnabled == "1");
        CHECK(p.powerPlan == "381b4222-f694-41f0-9685-ff5bb260df2e");
    }

    // Empty/malformed lines: blank lines and lines without a pipe are skipped
    // without crashing.
    {
        std::string content =
            "\n"
            "hibernation|on\n"
            "\n"
            "this line has no pipe\n"
            "hiberbootEnabled|0\n"
            "\n"
            "powerPlan|balanced\n";
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hibernation == "on");
        CHECK(p.hiberbootEnabled == "0");
        CHECK(p.powerPlan == "balanced");
    }

    // Missing keys: only hibernation present -> other fields are empty.
    {
        std::string content = PerformanceManager::FormatBackupLine("hibernation", "on");
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hibernation == "on");
        CHECK(p.hiberbootEnabled.empty());
        CHECK(p.powerPlan.empty());
    }

    // GUID round-trip: a real power plan GUID survives parsing exactly.
    {
        const std::string guid = "381b4222-f694-41f0-9685-ff5bb260df2e";
        std::string content = PerformanceManager::FormatBackupLine("powerPlan", guid);
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.powerPlan == guid);
    }

    // "missing" value preserved literally -- Revert uses this sentinel to
    // delete the registry value rather than restore it.
    {
        std::string content = PerformanceManager::FormatBackupLine("hiberbootEnabled", "missing");
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hiberbootEnabled == "missing");
    }

    // Trailing garbage in hiberbootEnabled value: "1abc" must not be silently
    // parsed as 1. ParseBackup stores the raw string; Revert() validates it
    // with std::stoul + idx check and skips the write on trailing garbage.
    // Verify the raw value is preserved and that the stoul+idx pattern
    // (mirroring Revert) rejects it.
    {
        std::string content = PerformanceManager::FormatBackupLine("hiberbootEnabled", "1abc");
        auto p = PerformanceManager::ParseBackup(content);
        CHECK(p.hiberbootEnabled == "1abc");
        bool wouldAccept = false;
        try {
            size_t idx = 0;
            (void)std::stoul(p.hiberbootEnabled, &idx);
            wouldAccept = (idx == p.hiberbootEnabled.size());
        } catch (...) { wouldAccept = false; }
        CHECK(!wouldAccept);
    }

    // Empty content -> all fields empty.
    {
        auto p = PerformanceManager::ParseBackup("");
        CHECK(p.hibernation.empty());
        CHECK(p.hiberbootEnabled.empty());
        CHECK(p.powerPlan.empty());
    }
}

// -- PerformanceManager power plan GUID parsing --------------------------------
// Tests the pure parsing logic extracted from GetActivePowerPlanGuid(). The
// helper parses "powercfg /getactivescheme" output, searching for "GUID:",
// skipping whitespace, extracting a 36-char GUID, and validating the
// 8-4-4-4-12 hex format (dashes at positions 8, 13, 18, 23). This was
// extracted to a public static method (mirroring ParseBackup/FormatBackupLine,
// HostsManager::RemoveBlock/HasBlock, and TelemetryManager::SplitEscaped/...)
// so it can be tested without invoking PowerShell.

static void TestParsePowerPlanGuid() {
    const std::string validGuid = "381b4222-f694-41f0-9685-ff5bb260df2e";

    // Normal powercfg output: "Power Scheme GUID: <guid>  (SchemeName)".
    {
        std::string out = "Power Scheme GUID: " + validGuid + "  (Balanced)";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out) == validGuid);
    }

    // No "GUID:" marker in the output -> empty string.
    {
        std::string out = "No power scheme information available.\n";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out).empty());
    }

    // Truncated output: GUID shorter than 36 chars after the marker.
    {
        std::string out = "Power Scheme GUID: 381b4222-f694-41f0";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out).empty());
    }

    // Invalid hex chars in a hex position -> empty string.
    {
        std::string out = "Power Scheme GUID: 381b4222-f694-g1f0-9685-ff5bb260df2e  (Balanced)";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out).empty());
    }

    // Missing dash at an expected position (hex char where dash belongs).
    {
        std::string out = "Power Scheme GUID: 381b4222af694-41f0-9685-ff5bb260df2e  (Balanced)";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out).empty());
    }

    // Uppercase hex (A-F) is accepted -> returns the uppercase GUID.
    {
        std::string out = "Power Scheme GUID: 381B4222-F694-41F0-9685-FF5BB260DF2E  (Balanced)";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out) == "381B4222-F694-41F0-9685-FF5BB260DF2E");
    }

    // Extra whitespace after "GUID:" is correctly skipped.
    {
        std::string out = "Power Scheme GUID:      \t  " + validGuid + "  (Balanced)";
        CHECK(PerformanceManager::ParsePowerPlanGuid(out) == validGuid);
    }

    // Empty string input -> empty string.
    {
        CHECK(PerformanceManager::ParsePowerPlanGuid("").empty());
    }
}

// -- Utils::ParseBuildNumber ---------------------------------------------------
// Tests the pure helper that converts a Windows build-number string to an
// integer. Windows 11 builds start at 22000; anything below is Windows 10
// or earlier. Extracted from IsWindows11() so the parsing logic can be unit
// tested without touching the registry.

static void TestParseBuildNumber() {
    // Typical Windows 11 builds (>= 22000).
    CHECK(Utils::ParseBuildNumber("22000") == 22000);
    CHECK(Utils::ParseBuildNumber("22631") == 22631);
    CHECK(Utils::ParseBuildNumber("26100") == 26100);

    // Typical Windows 10 builds (all < 22000).
    CHECK(Utils::ParseBuildNumber("19045") == 19045);
    CHECK(Utils::ParseBuildNumber("18363") == 18363);
    CHECK(Utils::ParseBuildNumber("10240") == 10240);

    // The boundary: 22000 is the first Windows 11 build.
    CHECK(Utils::ParseBuildNumber("21999") == 21999);   // Win10
    CHECK(Utils::ParseBuildNumber("22000") == 22000);   // Win11

    // Empty string -> 0.
    CHECK(Utils::ParseBuildNumber("") == 0);

    // Non-numeric -> 0.
    CHECK(Utils::ParseBuildNumber("abc") == 0);

    // Leading non-digit stops parsing immediately -> 0.
    CHECK(Utils::ParseBuildNumber("x22631") == 0);

    // Trailing non-digits are ignored (e.g. if UBR is appended as "22631.1").
    CHECK(Utils::ParseBuildNumber("22631.1") == 22631);

    // Leading zeros are handled correctly.
    CHECK(Utils::ParseBuildNumber("022631") == 22631);

    // Single digit.
    CHECK(Utils::ParseBuildNumber("5") == 5);

    // Zero.
    CHECK(Utils::ParseBuildNumber("0") == 0);
}

// -- ServiceManager backup line parsing ----------------------------------------
// Tests the pure parsing logic extracted from EnableAll(). The backup file is
// line-delimited ("name|startType"); ParseServiceLine turns one line into a
// ParsedServiceBackup struct. This was extracted to a public static method
// (mirroring HostsManager::RemoveBlock/HasBlock, TelemetryManager::SplitEscaped,
// and PerformanceManager::ParseBackup) so it can be tested without touching the
// SCM or the filesystem.

static void TestServiceBackupParse() {
    // Normal valid line: name and numeric start type.
    {
        auto p = ServiceManager::ParseServiceLine("DiagTrack|2");
        CHECK(p.valid);
        CHECK(p.name == "DiagTrack");
        CHECK(p.startType == 2);
    }

    // CRLF line ending: trailing \r must be stripped before parsing.
    {
        auto p = ServiceManager::ParseServiceLine("WerSvc|3\r");
        CHECK(p.valid);
        CHECK(p.name == "WerSvc");
        CHECK(p.startType == 3);
    }

    // Empty line -> invalid.
    {
        auto p = ServiceManager::ParseServiceLine("");
        CHECK(!p.valid);
    }

    // No pipe separator -> invalid.
    {
        auto p = ServiceManager::ParseServiceLine("garbage");
        CHECK(!p.valid);
    }

    // Non-numeric start type -> invalid (std::stoul throws).
    {
        auto p = ServiceManager::ParseServiceLine("X|abc");
        CHECK(!p.valid);
    }

    // Trailing garbage after a valid number -> invalid. Without the idx
    // check, std::stoul("2abc") would silently parse as 2 and discard the
    // "abc". The parser must reject the entire line as malformed.
    {
        auto p = ServiceManager::ParseServiceLine("DiagTrack|2abc");
        CHECK(!p.valid);
    }

    // Trailing space after a valid number -> invalid. std::stoul("2 ")
    // parses 2 but leaves idx pointing at the space, so the idx != size()
    // check rejects it.
    {
        auto p = ServiceManager::ParseServiceLine("DiagTrack|2 ");
        CHECK(!p.valid);
    }

    // Trailing pipe with empty number -> invalid (stoul on "" throws).
    {
        auto p = ServiceManager::ParseServiceLine("X|");
        CHECK(!p.valid);
    }

    // Empty name (leading pipe) -> invalid: an empty name can never open a
    // real service, so the parser rejects it rather than passing an empty
    // string to OpenServiceW.
    {
        auto p = ServiceManager::ParseServiceLine("|4");
        CHECK(!p.valid);
    }

    // CRLF-only line ("\r") -> empty after strip -> invalid.
    {
        auto p = ServiceManager::ParseServiceLine("\r");
        CHECK(!p.valid);
    }

    // SERVICE_AUTO_START (2) round-trips correctly.
    {
        auto p = ServiceManager::ParseServiceLine("SomeSvc|2");
        CHECK(p.valid);
        CHECK(p.startType == SERVICE_AUTO_START);
    }

    // SERVICE_DISABLED (4) round-trips correctly.
    {
        auto p = ServiceManager::ParseServiceLine("SomeSvc|4");
        CHECK(p.valid);
        CHECK(p.startType == SERVICE_DISABLED);
    }

    // SERVICE_DEMAND_START (3) round-trips correctly.
    {
        auto p = ServiceManager::ParseServiceLine("PcaSvc|3");
        CHECK(p.valid);
        CHECK(p.startType == SERVICE_DEMAND_START);
    }

    // Leading/trailing whitespace is preserved in the name (the parser does
    // not trim — the backup writer never adds whitespace, so a name with
    // leading spaces is a corrupt line that should fail at OpenServiceW,
    // not be silently mangled by the parser).
    {
        auto p = ServiceManager::ParseServiceLine(" DiagTrack|2");
        CHECK(p.valid);
        CHECK(p.name == " DiagTrack");
        CHECK(p.startType == 2);
    }
}

// -- TelemetryManager root-key validation -------------------------------------
// Tests the HKEY allowlist used by Revert() to guard against corrupt or
// hand-edited backup files. Only the six predefined root keys are valid;
// every other integer is rejected so it is never passed to RegCreateKeyExW.

static void TestRootKeyValidation() {
    // The six predefined root keys (winnt.h), in their documented 32-bit
    // form (0x8000000N). These are the values a hand-edited backup might
    // contain. All must be accepted.
    CHECK(TelemetryManager::IsValidRootKey(0x80000000) == true);  // HKEY_CLASSES_ROOT
    CHECK(TelemetryManager::IsValidRootKey(0x80000001) == true);  // HKEY_CURRENT_USER
    CHECK(TelemetryManager::IsValidRootKey(0x80000002) == true);  // HKEY_LOCAL_MACHINE
    CHECK(TelemetryManager::IsValidRootKey(0x80000003) == true);  // HKEY_USERS
    CHECK(TelemetryManager::IsValidRootKey(0x80000004) == true);  // HKEY_PERFORMANCE_DATA
    CHECK(TelemetryManager::IsValidRootKey(0x80000005) == true);  // HKEY_CURRENT_CONFIG

    // Sign-extended form (what ApplyAll writes on x64): also accepted.
    // On x64, winnt.h defines HKEY_* as ((HKEY)(ULONG_PTR)(LONG)0x8000000N),
    // which sign-extends to 0xFFFFFFFF8000000N.
    CHECK(TelemetryManager::IsValidRootKey(
        reinterpret_cast<uintptr_t>(HKEY_CLASSES_ROOT)) == true);
    CHECK(TelemetryManager::IsValidRootKey(
        reinterpret_cast<uintptr_t>(HKEY_CURRENT_USER)) == true);
    CHECK(TelemetryManager::IsValidRootKey(
        reinterpret_cast<uintptr_t>(HKEY_LOCAL_MACHINE)) == true);

    // Zero and small integers are not valid root keys.
    CHECK(TelemetryManager::IsValidRootKey(0) == false);
    CHECK(TelemetryManager::IsValidRootKey(1) == false);

    // Just past the last valid predefined key.
    CHECK(TelemetryManager::IsValidRootKey(0x80000006) == false);

    // Arbitrary garbage.
    CHECK(TelemetryManager::IsValidRootKey(0xDEADBEEF) == false);

    // The maximum 64-bit value.
    CHECK(TelemetryManager::IsValidRootKey(UINT64_MAX) == false);

    // RootKeyFromNum: valid 32-bit input sets 'out' to the canonical HKEY.
    {
        HKEY out = NULL;
        CHECK(TelemetryManager::RootKeyFromNum(0x80000002, out) == true);
        CHECK(out == HKEY_LOCAL_MACHINE);
    }
    {
        HKEY out = NULL;
        CHECK(TelemetryManager::RootKeyFromNum(0x80000001, out) == true);
        CHECK(out == HKEY_CURRENT_USER);
    }

    // RootKeyFromNum: sign-extended input also yields the canonical HKEY.
    {
        HKEY out = NULL;
        CHECK(TelemetryManager::RootKeyFromNum(
            reinterpret_cast<uintptr_t>(HKEY_CLASSES_ROOT), out) == true);
        CHECK(out == HKEY_CLASSES_ROOT);
    }

    // RootKeyFromNum: invalid input returns false and leaves 'out' untouched.
    {
        HKEY sentinel = reinterpret_cast<HKEY>(static_cast<uintptr_t>(0xDEADBEEF));
        HKEY out = sentinel;
        CHECK(TelemetryManager::RootKeyFromNum(0xDEADBEEF, out) == false);
        CHECK(out == sentinel);
    }
    {
        HKEY out = NULL;
        CHECK(TelemetryManager::RootKeyFromNum(0, out) == false);
        CHECK(out == NULL);
    }
}

// -- main --------------------------------------------------------------------

int main() {
    TestBloatwareList();
    TestTelemetryServices();
    TestScheduledTasks();
    TestBlockedDomains();
    TestRegistryTweaks();
    TestEscapePsSingleQuote();
    TestHostsRemoveBlock();
    TestHostsHasBlock();
    TestStringRoundTrip();
    TestEscapeFieldRoundTrip();
    TestHexEncodeDecodeRoundTrip();
    TestHexDecodeMalformed();
    TestSplitEscapedEdgeCases();
    TestFullBackupRecordRoundTrip();
    TestRootKeyValidation();
    TestPerfBackupParse();
    TestParsePowerPlanGuid();
    TestParseBuildNumber();
    TestServiceBackupParse();

    if (g_failures == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    } else {
        std::cout << g_failures << " test(s) failed.\n";
        return 1;
    }
}
