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

static void PrintMenu() {
    std::cout <<
        "\n"
        "   1) Remove bloatware apps (UWP/MSIX)\n"
        "   2) Remove OneDrive\n"
        "   3) Disable telemetry services\n"
        "   4) Delete telemetry services  (aggressive)\n"
        "   5) Apply telemetry & privacy registry tweaks\n"
        "   6) Disable scheduled telemetry tasks\n"
        "   7) Block telemetry domains (hosts file)\n"
        "   8) Performance tweaks (power, cleanup)\n"
        "   9) Create System Restore Point\n"
        "  10) List all targets (preview)\n"
        "  11) Revert: unblock telemetry domains (hosts)\n"
        "  12) Revert: re-enable scheduled tasks\n"
        "  13) RUN ALL  (everything)\n"
        "  14) Revert: re-enable telemetry services\n"
        "  15) Revert: undo registry tweaks\n"
        "  16) Revert: undo performance tweaks (hibernation, fast startup, power plan)\n"
        "   0) Exit\n";
}

// Maps a menu choice string to a short human-readable description, used for
// the audit log so each selection is recorded before it is executed.
static const char* MenuDescription(const std::string& choice) {
    if (choice == "1")  return "Remove bloatware apps";
    if (choice == "2")  return "Remove OneDrive";
    if (choice == "3")  return "Disable telemetry services";
    if (choice == "4")  return "Delete telemetry services (aggressive)";
    if (choice == "5")  return "Apply telemetry registry tweaks";
    if (choice == "6")  return "Disable scheduled telemetry tasks";
    if (choice == "7")  return "Block telemetry domains (hosts)";
    if (choice == "8")  return "Performance tweaks";
    if (choice == "9")  return "Create System Restore Point";
    if (choice == "10") return "List all targets (preview)";
    if (choice == "11") return "Revert: unblock telemetry domains";
    if (choice == "12") return "Revert: re-enable scheduled tasks";
    if (choice == "13") return "RUN ALL";
    if (choice == "14") return "Revert: re-enable telemetry services";
    if (choice == "15") return "Revert: undo registry tweaks";
    if (choice == "16") return "Revert: undo performance tweaks";
    if (choice == "0")  return "Exit";
    return "Invalid option";
}

int main(int argc, char* argv[]) {
    // Parse all command-line flags in a single pass before any other logic.
    // --version and --help exit BEFORE the UAC elevation check so non-admin
    // users can query the binary without being prompted for admin rights.
    // --no-banner skips the animated banner but continues to the menu flow,
    // which still requires elevation.
    bool noBanner = false;
    bool showHelp = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--version" || arg == "-V") {
            showVersion = true;
        } else if (arg == "--help" || arg == "-h") {
            showHelp = true;
        } else if (arg == "--no-banner" || arg == "-q") {
            noBanner = true;
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
        std::cout << "Usage: Debloat.exe [--version] [--help] [--no-banner]\n\n";
        std::cout << "Flags:\n";
        std::cout << "  --version, -V    Print version and exit\n";
        std::cout << "  --help, -h       Print this help and exit\n";
        std::cout << "  --no-banner, -q  Skip animated banner (for scripted use)\n\n";
        std::cout << "Menu options (interactive mode):\n";
        PrintMenu();
        return 0;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Windows 11 Debloat");

    if (!noBanner) {
        PrintBanner();
    }

    if (!Utils::IsElevated()) {
        Utils::PrintError("Administrator privileges required.");
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

    while (true) {
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

            if (choice == "0") {
                std::cout << "\n  Goodbye.\n";
                break;
            } else if (choice == "1") {
                if (Utils::AskYesNo("\n  Remove ALL bloatware apps?"))
                    AppxManager::RemoveAll();
            } else if (choice == "2") {
                if (Utils::AskYesNo("\n  Remove OneDrive?"))
                    AppxManager::RemoveOneDrive();
            } else if (choice == "3") {
                if (Utils::AskYesNo("\n  Disable ALL telemetry services?"))
                    ServiceManager::DisableAll();
            } else if (choice == "4") {
                Utils::PrintWarning("Deleting services is aggressive and harder to reverse.");
                if (Utils::AskYesNo("\n  Delete ALL telemetry services?"))
                    ServiceManager::DeleteAll();
            } else if (choice == "5") {
                if (Utils::AskYesNo("\n  Apply ALL telemetry & privacy registry tweaks?"))
                    TelemetryManager::ApplyAll();
            } else if (choice == "6") {
                if (Utils::AskYesNo("\n  Disable ALL scheduled telemetry tasks?"))
                    ScheduledTaskManager::DisableAll();
            } else if (choice == "7") {
                if (Utils::AskYesNo("\n  Block telemetry domains in hosts file?"))
                    HostsManager::Apply();
            } else if (choice == "8") {
                if (Utils::AskYesNo("\n  Apply performance tweaks & disk cleanup?"))
                    PerformanceManager::ApplyAll();
            } else if (choice == "9") {
                Utils::PrintInfo("Creating system restore point...");
                Utils::CreateRestorePoint();
            } else if (choice == "10") {
                AppxManager::List();
                ServiceManager::List();
                TelemetryManager::List();
                ScheduledTaskManager::List();
                HostsManager::List();
                PerformanceManager::List();
            } else if (choice == "11") {
                if (Utils::AskYesNo("\n  Revert telemetry block in hosts file?"))
                    HostsManager::Revert();
            } else if (choice == "12") {
                if (Utils::AskYesNo("\n  Re-enable ALL scheduled telemetry tasks?"))
                    ScheduledTaskManager::EnableAll();
            } else if (choice == "13") {
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
                        // manager does not skip the remaining steps.
                        try { AppxManager::RemoveAll(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Appx removal failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL Appx failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Appx removal failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL Appx failed: unknown exception");
                        }
                        try { AppxManager::RemoveOneDrive(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("OneDrive removal failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL OneDrive failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("OneDrive removal failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL OneDrive failed: unknown exception");
                        }
                        try { ServiceManager::DisableAll(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Service disable failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL Services failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Service disable failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL Services failed: unknown exception");
                        }
                        try { ScheduledTaskManager::DisableAll(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Scheduled task disable failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL ScheduledTasks failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Scheduled task disable failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL ScheduledTasks failed: unknown exception");
                        }
                        try { HostsManager::Apply(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Hosts block failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL Hosts failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Hosts block failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL Hosts failed: unknown exception");
                        }
                        try { TelemetryManager::ApplyAll(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Telemetry tweaks failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL Telemetry failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Telemetry tweaks failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL Telemetry failed: unknown exception");
                        }
                        try { PerformanceManager::ApplyAll(); } catch (const std::exception& e) {
                            Utils::PrintError(std::string("Performance tweaks failed: ") + e.what());
                            Utils::LogAction("ERROR", std::string("RUN_ALL Performance failed: ") + e.what());
                        } catch (...) {
                            Utils::PrintError("Performance tweaks failed: unknown error.");
                            Utils::LogAction("ERROR", "RUN_ALL Performance failed: unknown exception");
                        }
                        Utils::LogAction("RUN_ALL", "Full debloat complete");
                        Utils::PrintSuccess("\n=== Full debloat complete. Reboot recommended. ===");
                    }
                }
            } else if (choice == "14") {
                if (Utils::AskYesNo("\n  Re-enable ALL telemetry services?"))
                    ServiceManager::EnableAll();
            } else if (choice == "15") {
                if (Utils::AskYesNo("\n  Revert ALL registry tweaks from backup?"))
                    TelemetryManager::Revert();
            } else if (choice == "16") {
                if (Utils::AskYesNo("\n  Revert performance tweaks (hibernation, fast startup, power plan)?"))
                    PerformanceManager::Revert();
            } else {
                Utils::PrintError("Invalid option.");
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
