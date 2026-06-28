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

#include <iostream>
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
    CHECK(tweaks.size() == 45);

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

// -- main --------------------------------------------------------------------

int main() {
    TestBloatwareList();
    TestTelemetryServices();
    TestScheduledTasks();
    TestBlockedDomains();
    TestRegistryTweaks();

    if (g_failures == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    } else {
        std::cout << g_failures << " test(s) failed.\n";
        return 1;
    }
}
