# Windows 11 Debloat & Anti-Spyware

**Strip out Microsoft's bloatware, kill telemetry services, block telemetry domains, and lock down your privacy — in one click.**

A standalone C++ tool. No background processes. No setup. No telemetry of its own. Just a clean executable that does its job and exits.

&rarr; **[win.syferx.net](https://win.syferx.net)** &middot; **[github.com/RealSyferX](https://github.com/RealSyferX/)**

---

## What It Does

| | Count | Details |
|---|---|---|
| **Apps Removed** | 39 | Bing, Copilot, Solitaire, Clipchamp, Teams, Outlook, Widgets, Cortana, Mail & Calendar, People, and more |
| **Services Disabled** | 19 | DiagTrack (telemetry), Error Reporting, WAP Push, Xbox services, Remote Registry, Parental Controls, etc. |
| **Scheduled Tasks** | 17 | Compatibility Appraiser, CEIP, WER queue, Maps, Feedback, Disk Diagnostics, Power Efficiency |
| **Registry Tweaks** | 44 | AllowTelemetry=0, Advertising ID off, Cortana off, Bing search off, Wi-Fi Sense off, dark mode, background apps off, Copilot off |
| **Hosts Blocked** | 29 | Telemetry domains blocked at DNS level (vortex.data.microsoft.com, watson.telemetry.microsoft.com, etc.) |
| **Performance** | 6 | Disable hibernation, disable fast startup, High Performance power plan, clean temp/WU cache/WinSxS |
| **OneDrive** | Nuked | Uninstalled, folders cleaned, startup removed, sync blocked by policy |

Provisioned packages are also stripped — so removed apps **don't come back** after Windows updates.

## Quick Start

### Option A — Download (easiest)

Grab the pre-built `.exe` from **[win.syferx.net](https://win.syferx.net)** and run it as administrator.

### Option B — Compile from source

Requires Visual Studio 2022 with the C++ workload.

```bat
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Output: `build\Release\Debloat.exe`

## Usage

1. Run `Debloat.exe` as administrator (UAC auto-prompts)
2. Press **9** to create a System Restore Point
3. Press **13** to RUN ALL — or pick individual options 1–10
4. Use options 11, 12, or 14 to revert specific changes if needed
5. Reboot

```
   1) Remove bloatware apps (UWP/MSIX)
   2) Remove OneDrive
   3) Disable telemetry services
   4) Delete telemetry services  (aggressive)
   5) Apply telemetry & privacy registry tweaks
   6) Disable scheduled telemetry tasks
   7) Block telemetry domains (hosts file)
   8) Performance tweaks (power, cleanup)
   9) Create System Restore Point
  10) List all targets (preview)
   11) Revert: unblock telemetry domains (hosts)
   12) Revert: re-enable scheduled tasks
   13) RUN ALL  (everything)
   14) Revert: re-enable telemetry services
    0) Exit
```

> Options **11**, **12**, and **14** are non-destructive reverts — they undo the hosts-file block, re-enable the disabled scheduled tasks, and restore previously-disabled telemetry services respectively, giving you a quick escape hatch without restoring a whole-system snapshot. Option 14 reads a start-type backup written by option 3, so it restores the exact configuration that existed before disabling.

## What Stays Untouched

This tool is surgical. It does **not** touch:

- Microsoft Edge
- Windows Defender / Security
- Windows Update (the service — we only clean its download cache)
- NVIDIA / AMD / Realtek drivers
- Bluetooth, WiFi, networking
- Your personal files
- Notepad, Paint, Calculator, Snipping Tool, Photos
- Print Spooler, codecs, Windows Terminal

## Before vs After

| Without Debloat | With Debloat |
|---|---|
| 39 bloatware apps pre-installed | Apps removed — won't reinstall |
| DiagTrack sending telemetry to Microsoft | Service stopped & disabled |
| 17 scheduled tasks collecting telemetry | All disabled |
| 29 telemetry domains reachable | Blocked in hosts file |
| Advertising ID tracking you | Turned off system-wide |
| Bing results cluttering Start Menu | Clean local search only |
| Cortana & cloud search active | Disabled |
| Copilot button on taskbar | Hidden & disabled |
| OneDrive syncing in background | Uninstalled & blocked |
| Wi-Fi Sense sharing your network | Disabled |
| Input personalization collecting typing | Disabled |
| Background apps running | Disabled globally |
| Light mode default | Dark mode enabled |
| Hibernation wasting GBs of disk | Disabled |
| ~19 services wasting CPU & RAM | Zero overhead |
| WinSxS bloat accumulating | Component store cleaned |

## Tech

- C++17, Win32 API
- CMake build system
- UAC manifest (requireAdministrator)
- Services managed via SCM API (`OpenSCManager` / `ChangeServiceConfig`)
- Apps removed via PowerShell `Remove-AppxPackage` + `Remove-AppxProvisionedPackage`
- Scheduled tasks disabled via PowerShell `Disable-ScheduledTask`
- Hosts file edited directly (idempotent — marker comment prevents duplicates)
- Registry tweaks via `RegCreateKeyExW` / `RegSetValueExW`
- Performance via `powercfg`, `DISM /StartComponentCleanup /ResetBase`

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Disclaimer

This tool modifies system services, registry, hosts file, and installed applications. **Always create a System Restore Point before running.** Use at your own risk. Not affiliated with Microsoft.

---

Built by **[RealSyferX](https://github.com/RealSyferX/)** &middot; **[win.syferx.net](https://win.syferx.net)**
