#include "Utils.h"
#include "AppxManager.h"
#include "ServiceManager.h"
#include "TelemetryManager.h"
#include "ScheduledTaskManager.h"
#include "HostsManager.h"
#include "PerformanceManager.h"
#include <iostream>
#include <string>

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

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Windows 11 Debloat");

    PrintBanner();

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

    while (true) {
        PrintMenu();
        std::cout << "  > ";
        std::string choice;
        std::getline(std::cin, choice);

        if (!std::cin) {
            std::cout << "\n  EOF detected. Exiting.\n";
            break;
        }

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
                Utils::PrintInfo("Creating a System Restore Point before proceeding...");
                Utils::CreateRestorePoint();
                if (!Utils::AskYesNo("  Restore point attempted. Continue with full debloat?")) {
                    Utils::PrintInfo("Aborted. No changes were made.");
                } else {
                    AppxManager::RemoveAll();
                    AppxManager::RemoveOneDrive();
                    ServiceManager::DisableAll();
                    ScheduledTaskManager::DisableAll();
                    HostsManager::Apply();
                    TelemetryManager::ApplyAll();
                    PerformanceManager::ApplyAll();
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
    }
    return 0;
}
