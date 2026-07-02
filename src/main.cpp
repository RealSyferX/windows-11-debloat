#include "Utils.h"
#include "AppxManager.h"
#include "ServiceManager.h"
#include "TelemetryManager.h"
#include "ScheduledTaskManager.h"
#include "HostsManager.h"
#include "PerformanceManager.h"
#include <iostream>
#include <string>
#include <limits>
#include <functional>
#include <vector>

static void PrintBanner() {
    static const char* banner[][7] = {
        {"#####","#   #","#   #","#####","# #  ","#  # ","#   #"}, // R
        {"#####","#    ","#    ","#####","#    ","#    ","#####"}, // E
        {" ### ","#   #","#   #","#####","#   #","#   #","#   #"}, // A
        {"#    ","#    ","#    ","#    ","#    ","#    ","#####"}, // L
        {" ####","#    ","#    "," ### ","    #","    #","#### "}, // S
        {"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "}, // Y
        {"#####","#    ","#    ","#####","#    ","#    ","#    "}, // F
        {"#####","#    ","#    ","#####","#    ","#    ","#####"}, // E
        {"#####","#   #","#   #","#####","# #  ","#  # ","#   #"}, // R
        {"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"}, // X
    };

    Utils::SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "\n";
    for (int row = 0; row < 7; row++) {
        std::cout << "  ";
        for (int i = 0; i < 10; i++)
            std::cout << banner[i][row] << " ";
        std::cout << "\n" << std::flush;
        Sleep(70);
    }
    std::cout << "\n  ----------------------------------------------\n";
    std::cout << "       Windows 11 Debloat & Anti-Spyware\n";
    std::cout << "              C++ Edition\n";
    std::cout << "             by RealSyferX\n";
    std::cout << "               v" << DEBLOAT_VERSION << "\n";
    std::cout << "  ----------------------------------------------\n\n";
    Utils::SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
}

// ---------------------------------------------------------------------------
// Data-driven menu
//
// PrintMenu(), MenuDescription(), and the dispatch loop in main() all iterate
// over a single MenuItem table. Adding or reordering an option now requires
// editing one location instead of three separately-synced places.
// ---------------------------------------------------------------------------
struct MenuItem {
    int number;
    std::string label;       // text shown in PrintMenu
    std::string description; // text shown in MenuDescription
    std::function<void()> action;
};

// Wraps a single RUN ALL step in the try/catch/catch pattern so a failure in
// one manager does not skip the remaining steps. `stepLabel` is used for the
// console error message; `logTag` is used for the audit-log entry. The two are
// kept separate to match the exact strings the original inline blocks produced
// (e.g. console "Service disable failed" vs. log "RUN_ALL Services failed").
static void RunStep(const std::string& stepLabel,
                    const std::string& logTag,
                    const std::function<void()>& action) {
    try {
        action();
    } catch (const std::exception& e) {
        Utils::PrintError(stepLabel + " failed: " + e.what());
        Utils::LogAction("ERROR", "RUN_ALL " + logTag + " failed: " + e.what());
    } catch (...) {
        Utils::PrintError(stepLabel + " failed: unknown error.");
        Utils::LogAction("ERROR", "RUN_ALL " + logTag + " failed: unknown exception");
    }
}

// The single source of truth for the menu. Captureless lambdas reference only
// free/static functions (managers, Utils, RunStep), so no captures are needed.
static const std::vector<MenuItem>& GetMenu() {
    static const std::vector<MenuItem> menu = {
        {1, "Remove bloatware apps (UWP/MSIX)", "Remove bloatware apps",
            []{ if (Utils::AskYesNo("\n  Remove ALL bloatware apps?"))
                    AppxManager::RemoveAll(); }},
        {2, "Remove OneDrive", "Remove OneDrive",
            []{ if (Utils::AskYesNo("\n  Remove OneDrive?"))
                    AppxManager::RemoveOneDrive(); }},
        {3, "Disable telemetry services", "Disable telemetry services",
            []{ if (Utils::AskYesNo("\n  Disable ALL telemetry services?"))
                    ServiceManager::DisableAll(); }},
        {4, "Delete telemetry services  (aggressive)", "Delete telemetry services (aggressive)",
            []{ Utils::PrintWarning("Deleting services is aggressive and harder to reverse.");
                if (Utils::AskYesNo("\n  Delete ALL telemetry services?"))
                    ServiceManager::DeleteAll(); }},
        {5, "Apply telemetry & privacy registry tweaks", "Apply telemetry registry tweaks",
            []{ if (Utils::AskYesNo("\n  Apply ALL telemetry & privacy registry tweaks?"))
                    TelemetryManager::ApplyAll(); }},
        {6, "Disable scheduled telemetry tasks", "Disable scheduled telemetry tasks",
            []{ if (Utils::AskYesNo("\n  Disable ALL scheduled telemetry tasks?"))
                    ScheduledTaskManager::DisableAll(); }},
        {7, "Block telemetry domains (hosts file)", "Block telemetry domains (hosts)",
            []{ if (Utils::AskYesNo("\n  Block telemetry domains in hosts file?"))
                    HostsManager::Apply(); }},
        {8, "Performance tweaks (power, cleanup)", "Performance tweaks",
            []{ if (Utils::AskYesNo("\n  Apply performance tweaks & disk cleanup?"))
                    PerformanceManager::ApplyAll(); }},
        {9, "Create System Restore Point", "Create System Restore Point",
            []{ Utils::PrintInfo("Creating system restore point...");
                Utils::CreateRestorePoint(); }},
        {10, "List all targets (preview)", "List all targets (preview)",
            []{ AppxManager::List();
                ServiceManager::List();
                TelemetryManager::List();
                ScheduledTaskManager::List();
                HostsManager::List();
                PerformanceManager::List(); }},
        {11, "Revert: unblock telemetry domains (hosts)", "Revert: unblock telemetry domains",
            []{ if (Utils::AskYesNo("\n  Revert telemetry block in hosts file?"))
                    HostsManager::Revert(); }},
        {12, "Revert: re-enable scheduled tasks", "Revert: re-enable scheduled tasks",
            []{ if (Utils::AskYesNo("\n  Re-enable ALL scheduled telemetry tasks?"))
                    ScheduledTaskManager::EnableAll(); }},
        {13, "RUN ALL  (everything)", "RUN ALL",
            []{
                Utils::PrintWarning("This will remove apps, OneDrive, disable services, apply registry tweaks,");
                Utils::PrintWarning("disable scheduled tasks, block telemetry domains, and run performance tweaks.");
                if (Utils::AskYesNo("\n  Proceed with FULL debloat?")) {
                    Utils::LogAction("RUN_ALL", "Starting full debloat");
                    Utils::PrintInfo("Creating a System Restore Point before proceeding...");
                    Utils::CreateRestorePoint();
                    if (!Utils::AskYesNo("  Restore point attempted. Continue with full debloat?")) {
                        Utils::PrintInfo("Aborted. No changes were made.");
                    } else {
                        // Each RUN ALL step is isolated so a failure in one
                        // manager does not skip the remaining steps. Between
                        // steps, check for Ctrl+C interrupt so a long-running
                        // operation (e.g. PerformanceManager::ApplyAll with
                        // DISM) does not force the user to wait for all
                        // remaining steps. The `return;` exits the RUN ALL
                        // action lambda; the menu loop's top-of-iteration
                        // check then logs SESSION_INTERRUPTED and breaks.
                        RunStep("Appx removal", "Appx", []{ AppxManager::RemoveAll(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after Appx (1/7 complete, 6 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after Appx (1/7 complete, 6 skipped)");
                            return;
                        }
                        RunStep("OneDrive removal", "OneDrive", []{ AppxManager::RemoveOneDrive(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after OneDrive (2/7 complete, 5 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after OneDrive (2/7 complete, 5 skipped)");
                            return;
                        }
                        RunStep("Service disable", "Services", []{ ServiceManager::DisableAll(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after Services (3/7 complete, 4 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after Services (3/7 complete, 4 skipped)");
                            return;
                        }
                        RunStep("Scheduled task disable", "ScheduledTasks", []{ ScheduledTaskManager::DisableAll(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after ScheduledTasks (4/7 complete, 3 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after ScheduledTasks (4/7 complete, 3 skipped)");
                            return;
                        }
                        RunStep("Hosts block", "Hosts", []{ HostsManager::Apply(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after Hosts (5/7 complete, 2 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after Hosts (5/7 complete, 2 skipped)");
                            return;
                        }
                        RunStep("Telemetry tweaks", "Telemetry", []{ TelemetryManager::ApplyAll(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after Telemetry (6/7 complete, 1 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after Telemetry (6/7 complete, 1 skipped)");
                            return;
                        }
                        RunStep("Performance tweaks", "Performance", []{ PerformanceManager::ApplyAll(); });
                        if (Utils::IsShutdownRequested()) {
                            Utils::PrintWarning("[!!] RUN ALL interrupted after Performance (7/7 complete, 0 skipped).");
                            Utils::LogAction("RUN_ALL_INTERRUPTED", "interrupted after Performance (7/7 complete, 0 skipped)");
                            return;
                        }
                        Utils::LogAction("RUN_ALL", "Full debloat complete");
                        Utils::PrintSuccess("\n=== Full debloat complete. Reboot recommended. ===");
                    }
                }
            }},
        {14, "Revert: re-enable telemetry services", "Revert: re-enable telemetry services",
            []{ if (Utils::AskYesNo("\n  Re-enable ALL telemetry services?"))
                    ServiceManager::EnableAll(); }},
        {15, "Revert: undo registry tweaks", "Revert: undo registry tweaks",
            []{ if (Utils::AskYesNo("\n  Revert ALL registry tweaks from backup?"))
                    TelemetryManager::Revert(); }},
        {16, "Revert: undo performance tweaks (hibernation, fast startup, power plan)", "Revert: undo performance tweaks",
            []{ if (Utils::AskYesNo("\n  Revert performance tweaks (hibernation, fast startup, power plan)?"))
                    PerformanceManager::Revert(); }},
        {0, "Exit", "Exit",
            []{ std::cout << "\n  Goodbye.\n"; }},
    };
    return menu;
}

static void PrintMenu() {
    std::cout << "\n";
    for (const auto& item : GetMenu()) {
        if (item.number < 10)
            std::cout << "   " << item.number << ") " << item.label << "\n";
        else
            std::cout << "  " << item.number << ") " << item.label << "\n";
    }
}

// Maps a menu choice string to a short human-readable description, used for
// the audit log so each selection is recorded before it is executed.
static const char* MenuDescription(const std::string& choice) {
    for (const auto& item : GetMenu()) {
        if (choice == std::to_string(item.number))
            return item.description.c_str();
    }
    return "Invalid option";
}

// Maps a --revert <target> name to its menu option number. The mapping is
// derived from the menu table in GetMenu(): revert operations are items
// 11 (hosts), 12 (tasks), 14 (services), 15 (registry), and 16 (perf).
// Returns -1 for an unrecognized target name.
static int RevertTargetToNumber(const std::string& target) {
    if (target == "hosts")    return 11;  // Revert: unblock telemetry domains
    if (target == "tasks")    return 12;  // Revert: re-enable scheduled tasks
    if (target == "services") return 14;  // Revert: re-enable telemetry services
    if (target == "registry") return 15;  // Revert: undo registry tweaks
    if (target == "perf")     return 16;  // Revert: undo performance tweaks
    return -1;
}

// Executes a single menu option non-interactively. Returns 0 on success,
// 1 on failure (invalid number or exception during execution). The caller
// is responsible for the elevation and version-guard checks; AskYesNo
// prompts inside the actions are auto-confirmed when SetAutoYes(true) was
// called beforehand.
static int RunNonInteractive(int menuNumber) {
    const MenuItem* item = nullptr;
    for (const auto& m : GetMenu()) {
        if (m.number == menuNumber) {
            item = &m;
            break;
        }
    }
    if (!item) {
        Utils::PrintError("Invalid menu option number: " + std::to_string(menuNumber));
        return 1;
    }

    Utils::LogAction("CLI", "option " + std::to_string(menuNumber) + " - " + item->description);

    try {
        item->action();
    } catch (const std::exception& e) {
        Utils::PrintError(std::string("Action failed: ") + e.what());
        Utils::LogAction("ERROR", std::string("CLI action failed: ") + e.what());
        return 1;
    } catch (...) {
        Utils::PrintError("Action failed: unknown error.");
        Utils::LogAction("ERROR", "CLI action failed: unknown exception");
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // Parse all command-line flags in a single pass before any other logic.
    // --version and --help exit BEFORE the UAC elevation check so non-admin
    // users can query the binary without being prompted for admin rights.
    // --no-banner skips the animated banner but continues to the menu flow,
    // which still requires elevation.
    // --apply <N> runs a single menu option non-interactively and exits.
    // --revert <target> runs a revert operation by name and exits.
    // --yes/-y auto-confirms all AskYesNo prompts (required for non-interactive
    // use when stdin is not a TTY).
    bool noBanner = false;
    bool showHelp = false;
    bool showVersion = false;
    bool autoYes = false;
    int applyOption = -1;          // -1 = not specified
    std::string revertTarget;      // empty = not specified

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--version" || arg == "-V") {
            showVersion = true;
        } else if (arg == "--help" || arg == "-h") {
            showHelp = true;
        } else if (arg == "--no-banner" || arg == "-q") {
            noBanner = true;
        } else if (arg == "--yes" || arg == "-y") {
            autoYes = true;
        } else if (arg == "--apply" || arg == "-a") {
            if (i + 1 >= argc) {
                SetConsoleOutputCP(CP_UTF8);
                Utils::PrintError("--apply requires a menu option number.");
                return 1;
            }
            std::string numStr(argv[++i]);
            size_t pos = 0;
            try {
                applyOption = std::stoi(numStr, &pos);
            } catch (...) {
                SetConsoleOutputCP(CP_UTF8);
                Utils::PrintError("Invalid menu option number: " + numStr);
                return 1;
            }
            if (pos != numStr.size()) {
                SetConsoleOutputCP(CP_UTF8);
                Utils::PrintError("Invalid menu option number: " + numStr);
                return 1;
            }
        } else if (arg == "--revert") {
            if (i + 1 >= argc) {
                SetConsoleOutputCP(CP_UTF8);
                Utils::PrintError("--revert requires a target name (hosts|tasks|services|registry|perf).");
                return 1;
            }
            revertTarget = argv[++i];
        }
    }

    if (showVersion) {
        std::cout << "Debloat v" << Utils::GetVersion() << "\n";
        return 0;
    }

    if (showHelp) {
        // Ensure the em-dash and any other non-ASCII characters render correctly
        // even though this runs before the main SetConsoleOutputCP(CP_UTF8) call.
        SetConsoleOutputCP(CP_UTF8);
        std::cout << "Debloat v" << Utils::GetVersion() << " — Windows 11 Debloat Tool\n";
        std::cout << "Usage: Debloat.exe [--version] [--help] [--no-banner] [--yes]\n";
        std::cout << "                   [--apply <N>] [--revert <target>]\n\n";
        std::cout << "Flags:\n";
        std::cout << "  --version, -V       Print version and exit\n";
        std::cout << "  --help, -h          Print this help and exit\n";
        std::cout << "  --no-banner, -q     Skip animated banner (for scripted use)\n";
        std::cout << "  --yes, -y           Auto-confirm all prompts (required for non-interactive use)\n";
        std::cout << "  --apply <N>         Run menu option N non-interactively and exit\n";
        std::cout << "                      (requires --yes if stdin is not a TTY)\n";
        std::cout << "  --revert <target>   Run revert by name and exit:\n";
        std::cout << "                        hosts | tasks | services | registry | perf\n\n";
        std::cout << "Menu options (interactive mode):\n";
        PrintMenu();
        return 0;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Windows 11 Debloat");

    // Resolve --revert <target> to a menu option number. --apply takes
    // precedence if both --apply and --revert were given on the same command
    // line (the user can always re-run with the other flag).
    bool nonInteractive = false;
    if (applyOption >= 0) {
        nonInteractive = true;
    } else if (!revertTarget.empty()) {
        applyOption = RevertTargetToNumber(revertTarget);
        if (applyOption < 0) {
            Utils::PrintError("Unknown revert target: " + revertTarget +
                " (valid: hosts|tasks|services|registry|perf)");
            return 1;
        }
        nonInteractive = true;
    }

    // Non-interactive safety: refuse --apply/--revert without --yes when
    // stdin is not a TTY (e.g. piped or redirected input), since destructive
    // actions cannot be confirmed interactively in that context.
    if (nonInteractive && !autoYes) {
        DWORD ft = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        if (ft != FILE_TYPE_CHAR) {
            Utils::PrintError("Non-interactive mode requires --yes to confirm destructive actions.");
            return 1;
        }
    }

    // Banner: skip in non-interactive mode or when --no-banner is set.
    if (!nonInteractive && !noBanner) {
        PrintBanner();
    }

    if (!Utils::IsElevated()) {
        Utils::PrintError("Administrator privileges required.");
        if (nonInteractive) {
            // No relaunch prompt in non-interactive mode — the caller (script,
            // GPO, MDM) is responsible for invoking the tool elevated.
            Utils::LogAction("ERROR", "CLI mode without elevation");
            return 1;
        }
        if (Utils::AskYesNo("  Relaunch as administrator?")) {
            if (Utils::RelaunchAsAdmin())
                return 0;
            Utils::PrintError("Failed to relaunch. Right-click -> Run as administrator.");
        }
        std::cout << "\n  Press Enter to exit...";
        std::cin.get();
        return 1;
    }
    Utils::PrintSuccess("Running with administrator privileges.");
    Utils::LogAction("SESSION_START", "Debloat v" + Utils::GetVersion());

    // Install the Ctrl+C console control handler so interrupts are caught
    // gracefully instead of hard-terminating the process mid-operation.
    Utils::InstallShutdownHandler();

    // Enable auto-yes for all AskYesNo calls (managers, version guard, etc.)
    // so that --yes propagates to confirmation prompts deep in the action
    // lambdas without changing their call signatures.
    if (autoYes) {
        Utils::SetAutoYes(true);
    }

    // -- Windows 11 version guard -------------------------------------------
    // The tool is designed for Windows 11. If it detects a non-Windows-11 OS
    // (build < 22000), it warns the user and asks for confirmation before
    // proceeding, since tweaks may not apply or may have unintended effects.
    // With --yes, AskYesNo auto-confirms so the guard never blocks scripting.
    if (!Utils::IsWindows11()) {
        std::string ver = Utils::GetWindowsVersionString();
        Utils::PrintWarning("========================================================");
        Utils::PrintWarning("  WARNING: This tool is designed for Windows 11.");
        Utils::PrintWarning("  Detected: " + ver);
        Utils::PrintWarning("  Some tweaks may not apply or may have unintended effects.");
        Utils::PrintWarning("========================================================");
        if (!Utils::AskYesNo("  Continue anyway?")) {
            Utils::LogAction("SESSION_ABORT", "Non-Windows-11 OS detected, user declined to continue");
            return 1;
        }
        Utils::LogAction("VERSION_OVERRIDE", "Running on non-Windows-11 OS, user confirmed");
    }

    // Non-interactive execution: --apply or --revert runs a single menu
    // option and exits, skipping the interactive menu loop entirely.
    if (nonInteractive) {
        int exitCode = RunNonInteractive(applyOption);
        if (Utils::IsShutdownRequested()) {
            Utils::LogAction("SESSION_INTERRUPTED", "Ctrl+C received (non-interactive)");
        }
        Utils::LogAction("CLI_EXIT", "exit code " + std::to_string(exitCode));
        Utils::LogAction("SESSION_END", "Debloat tool exited (non-interactive)");
        return exitCode;
    }

    while (true) {
        if (Utils::IsShutdownRequested()) {
            Utils::PrintWarning("[!!] Session interrupted. Exiting gracefully.");
            Utils::LogAction("SESSION_INTERRUPTED", "Ctrl+C received");
            break;
        }
        try {
            PrintMenu();
            std::cout << "  > ";
            std::string choice;
            std::getline(std::cin, choice);

            if (!std::cin) {
                std::cout << "\n  EOF detected. Exiting.\n";
                break;
            }

            // Log the menu selection for the audit trail before executing it.
            Utils::LogAction("MENU", std::string("option ") + choice + " - " + MenuDescription(choice));

            // Data-driven dispatch: iterate the single MenuItem table. The
            // Exit action (number 0) prints the goodbye message; the separate
            // choice == "0" check breaks out of the surrounding while loop.
            bool matched = false;
            for (const auto& item : GetMenu()) {
                if (choice == std::to_string(item.number)) {
                    item.action();
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                Utils::PrintError("Invalid option.");
            }
            if (choice == "0") {
                break;
            }
        } catch (const std::exception& e) {
            Utils::PrintError(std::string("Unexpected error: ") + e.what());
            Utils::LogAction("ERROR", std::string("Exception: ") + e.what());
            std::cout << "\n  Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        } catch (...) {
            Utils::PrintError("Unknown error occurred.");
            Utils::LogAction("ERROR", "Unknown exception");
            std::cout << "\n  Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
    }
    Utils::LogAction("SESSION_END", "Debloat tool exited");
    return 0;
}
