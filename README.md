# Windows 11 Debloat & Anti-Spyware

**Strip out Microsoft's bloatware, kill telemetry services, and lock down your privacy — in one click.**

A standalone C++ tool. No background processes. No setup. No telemetry of its own. Just a clean executable that does its job and exits.

&rarr; **[win.syferx.net](https://win.syferx.net)**

---

## What It Does

| | Count | Details |
|---|---|---|
| **Apps Removed** | 31 | Bing, Copilot, Solitaire, Clipchamp, Teams, Outlook, Widgets, Cortana, Sticky Notes, Phone Link, and more |
| **Services Disabled** | 10 | DiagTrack (telemetry), Error Reporting, WAP Push, Insider telemetry, Compatibility Assistant, etc. |
| **Registry Tweaks** | 33+ | AllowTelemetry=0, Advertising ID off, Cortana off, Bing search off, Wi-Fi Sense off, activity history off |
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
2. Press **6** to create a System Restore Point
3. Press **8** to RUN ALL — or pick individual options 1–5
4. Reboot

```
  1) Remove bloatware apps (UWP/MSIX)
  2) Remove OneDrive
  3) Disable telemetry services
  4) Delete telemetry services  (aggressive)
  5) Apply telemetry registry tweaks
  6) Create System Restore Point
  7) List all targets (preview)
  8) RUN ALL  (apps + OneDrive + disable services + registry)
  0) Exit
```

## What Stays Untouched

This tool is surgical. It does **not** touch:

- Microsoft Edge
- Windows Defender / Security
- Windows Update
- NVIDIA / AMD / Realtek drivers
- Bluetooth, WiFi, networking
- Your personal files
- Notepad, Paint, Calculator, Snipping Tool, Photos
- Print Spooler, codecs, Windows Terminal

## Before vs After

| Without Debloat | With Debloat |
|---|---|
| 30+ bloatware apps pre-installed | Apps removed — won't reinstall |
| DiagTrack sending telemetry to Microsoft | Service stopped & disabled |
| Advertising ID tracking you | Turned off system-wide |
| Bing results cluttering Start Menu | Clean local search only |
| Cortana & cloud search active | Disabled |
| OneDrive syncing in background | Uninstalled & blocked |
| Wi-Fi Sense sharing your network | Disabled |
| Input personalization collecting typing | Disabled |
| ~10 services wasting CPU & RAM | Zero overhead |

## Tech

- C++17, Win32 API
- CMake build system
- UAC manifest (requireAdministrator)
- Services managed via SCM API
- Apps removed via PowerShell `Remove-AppxPackage` + `Remove-AppxProvisionedPackage`
- Registry tweaks via `RegCreateKeyExW` / `RegSetValueExW`

## Disclaimer

This tool modifies system services, registry, and installed applications. **Always create a System Restore Point before running.** Use at your own risk. Not affiliated with Microsoft.

---

Built by **[RealSyferX](https://win.syferx.net)** &middot; **[win.syferx.net](https://win.syferx.net)**
