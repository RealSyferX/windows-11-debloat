# Contributing to Windows 11 Debloat

First off, thanks for taking the time to contribute! This document covers
everything you need to get a local build running, understand the code
conventions, and submit a pull request that passes review.

This is a C++17 / Win32 project built with CMake and MSVC. It is a
**data-driven** tool: bloatware apps, telemetry services, scheduled tasks,
blocked domains, and registry tweaks all live in static data tables inside the
manager `.cpp` files. Most contributions touch those tables, so read the
[data-table update workflow](#data-table-update-workflow) section carefully.

---

## Prerequisites

- **Visual Studio 2022** with the *"Desktop development with C++"* workload
  (includes the MSVC compiler and the Windows SDK).
- **CMake 3.15 or newer** (bundled with VS 2022, or install separately).
- Git (for cloning and branching).

No external libraries are required — the project links only against `advapi32`
and `shell32` from the Windows SDK.

---

## Build Instructions

From the repository root:

```bat
cmake -B build -G "Visual Studio 17 2022" -DBUILD_TESTING=ON
cmake --build build --config Release
```

- The main executable is produced at `build\Release\Debloat.exe`.
- `-DBUILD_TESTING=ON` builds the `debloat_tests` target (see below). Omit it
  for a faster release-only build.

> **Note:** `Debloat.exe` has a `requireAdministrator` UAC manifest, so running
> it will prompt for elevation. `--version` and `--help` exit *before* the
> elevation check, so they work without admin rights.

---

## Running Tests

The test binary is a hand-rolled assert framework (no external test
dependency). Build with `-DBUILD_TESTING=ON` as above, then run:

```bat
.\build\Release\debloat_tests.exe
```

The executable prints `All tests passed.` and exits `0` on success, or
`N test(s) failed.` and exits `1` on failure. **All tests must pass before a
PR can be merged.**

The tests validate the data tables (counts, no duplicates, field validity) and
the pure helper functions extracted from the managers (PowerShell escaping,
hosts-block removal, registry backup serialization, performance backup
parsing). If a test fails, investigate the underlying data — **do NOT weaken
the test**.

---

## Code Style

The codebase enforces a strict, zero-warning build. Follow these conventions
in all new and modified code:

- **4-space indentation** (no tabs).
- **`#pragma once`** for header guards (not `#ifndef`/`#define`).
- **Compile cleanly under `/W4 /permissive- /WX`** — all code must build with
  **zero warnings**. Warnings are treated as errors; a warning is a build
  failure.
- **User-facing output** goes through the `Utils` print helpers:
  `PrintInfo`, `PrintSuccess` (a.k.a. `PrintOK`), `PrintError`,
  `PrintWarning`, and `PrintHeader`. Do not use raw `std::cout` for
  status/error messages intended for the end user.
- **Manager classes use static methods only** — they are never instantiated.
  See `AppxManager`, `ServiceManager`, `TelemetryManager`,
  `ScheduledTaskManager`, `HostsManager`, and `PerformanceManager`. Follow the
  same pattern for any new manager.
- **All PowerShell execution must go through `Utils::RunPowerShell()`** — never
  call raw `_wpopen` / `_popen` / `system()` directly. `RunPowerShell` writes
  the script to a non-predictable temp file, captures output, enforces a
  timeout, and auto-deletes the temp file.
- **Escape interpolated values with `Utils::EscapePsSingleQuote()`** when
  embedding any value into a PowerShell single-quoted string literal — even
  values from "trusted" static data tables. This prevents PowerShell injection.
- **Use `Utils::GetDebloatDataDir()`** to resolve the backup/data directory
  (`%ProgramData%\Debloat\`). Do not hardcode the path.
- **Use `Utils::WriteBackupAtomic()`** for all backup file writes. It writes to
  a temp file and renames over the target, so a crash mid-write cannot corrupt
  an existing backup.
- **Use `Utils::PrintPsResult()`** to handle the result of a `RunPowerShell`
  call — it prints captured output followed by a success or error message.
- **Use `Utils::LogAction()`** for every action that modifies the system. This
  appends a timestamped entry to the persistent audit log at
  `%ProgramData%\Debloat\debloat.log`.
- **Confirmation prompts default to No (`(y/N)`)** — destructive actions
  require explicit confirmation. Empty input (pressing Enter) or EOF is
  treated as No; the user must type an explicit `y` or `Y` to proceed. This
  is a deliberate safety default for a system-modifying tool.

---

## Data-Table Update Workflow

This is the most common contribution type: adding a new bloatware app, a
telemetry service, a scheduled task, a blocked domain, or a registry tweak.

When you add or remove an entry, you **MUST update all three of these places**
— they are intentionally kept in sync and the tests enforce the counts:

1. **The source `.cpp` data table.** For example, add an entry to the
   `static const std::vector` returned by `AppxManager::GetBloatwareList()` in
   `src/AppxManager.cpp`. The other tables live in:
   - `ServiceManager::GetTelemetryServices()` — `src/ServiceManager.cpp`
   - `ScheduledTaskManager::GetTasks()` — `src/ScheduledTaskManager.cpp`
   - `HostsManager::GetBlockedDomains()` — `src/HostsManager.cpp`
   - `TelemetryManager::GetRegistryTweaks()` — `src/TelemetryManager.cpp`

2. **The count assertion in `tests/test_datatables.cpp`.** For example:
   ```cpp
   CHECK(AppxManager::GetBloatwareList().size() == 39);
   ```
   Update `39` to the new count. The test checks counts for every table.

3. **The count in `README.md`.** For example, the "Apps Removed" row in the
   *What It Does* table shows `39`, and the *Before vs After* table references
   "39 bloatware apps". Update both occurrences.

A PR that updates only one or two of these places will fail review (and likely
fail the test build). The triple-sync is deliberate: it prevents drift between
the code, the tests, and the user-facing documentation.

---

## Revert Requirements

Any **new action that modifies the system** must be reversible. Concretely,
a new system-modifying feature must include a `Revert()` method (or extend an
existing one) that:

1. **Creates a backup before modifying** the system, using
   `Utils::WriteBackupAtomic()`. The backup must capture enough state to
   restore the original configuration (e.g., service start types, original
   registry values, original power-plan GUID).
2. **Reads the backup to restore original state** on revert — not reset to
   Microsoft defaults. If a value did not exist before the tweak, revert must
   delete it rather than set a default.
3. **Is added as a menu option** in `src/main.cpp` so users can invoke it
   from the interactive menu (following the existing `11)–16) Revert: …`
   pattern).
4. **Is documented in `README.md`** — both in the usage menu listing and, if
   the revert has non-trivial behaviour, in the explanatory note beneath it.

Irreversible actions (like temp-file cleanup or DISM `/ResetBase`) are the
exception, not the rule, and must be clearly documented as irreversible in the
README.

---

## Pull Request Checklist

Before opening a PR, confirm:

- [ ] **Tests pass** — `.\build\Release\debloat_tests.exe` exits `0`.
- [ ] **No new compiler warnings** under `/W4 /permissive- /WX`.
- [ ] **README counts updated** if you changed any data table (see
      [Data-table update workflow](#data-table-update-workflow)).
- [ ] **New system-modifying actions have a revert + backup** (see
      [Revert requirements](#revert-requirements)).
- [ ] **New PowerShell interpolation uses `EscapePsSingleQuote`** for any
      value embedded in a single-quoted PS string.
- [ ] **`Utils::LogAction` calls added** for any new system-modifying action.
- [ ] **No new raw `_wpopen`/`system` calls** — all PowerShell goes through
      `Utils::RunPowerShell`.

---

## Branch Naming

Use a short, descriptive branch name prefixed by its type:

| Prefix | Use for |
|---|---|
| `feat/` | New features or data-table entries |
| `fix/` | Bug fixes |
| `chore/` | Maintenance, deps, build config |
| `refactor/` | Code restructuring with no behavior change |
| `test/` | New or fixed tests |
| `docs/` | Documentation changes |

Example: `feat/add-weather-bloatware`, `fix/hosts-revert-edge-case`.

---

## Commit Message Format

This project uses [Conventional Commits](https://www.conventionalcommits.org/).
Format each commit message as:

```
<type>: <description>
```

Where `<type>` is one of: `feat`, `fix`, `chore`, `refactor`, `test`, `docs`,
`perf`, `security`.

Examples (matching the existing project history):

```
feat: add new bloatware app
fix: prevent crash on EOF input
refactor: deduplicate DisableAll/EnableAll into shared helper
test: add unit tests for backup parsing pure functions
docs: update README counts for new telemetry service
```

Keep the subject line under ~72 characters. Use the imperative mood
("add", "fix", "prevent"), not past tense ("added", "fixed").

---

## Questions?

If something in this guide is unclear, feel free to open a regular
(non-security) GitHub issue with the `question` label, and we'll help you get
unblocked.
