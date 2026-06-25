#include "Utils.h"
#include "AppxManager.h"
#include "ServiceManager.h"
#include "TelemetryManager.h"
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
        "  1) Remove bloatware apps (UWP/MSIX)\n"
        "  2) Remove OneDrive\n"
        "  3) Disable telemetry services\n"
        "  4) Delete telemetry services  (aggressive)\n"
        "  5) Apply telemetry registry tweaks\n"
        "  6) Create System Restore Point\n"
        "  7) List all targets (preview)\n"
        "  8) RUN ALL  (apps + OneDrive + disable services + registry)\n"
        "  0) Exit\n";
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
            if (Utils::AskYesNo("\n  Apply ALL telemetry registry tweaks?"))
                TelemetryManager::ApplyAll();
        } else if (choice == "6") {
            Utils::PrintInfo("Creating system restore point...");
            Utils::CreateRestorePoint();
        } else if (choice == "7") {
            AppxManager::List();
            ServiceManager::List();
            TelemetryManager::List();
        } else if (choice == "8") {
            Utils::PrintWarning("This will remove apps, OneDrive, disable services, and apply registry tweaks.");
            if (Utils::AskYesNo("\n  Proceed with FULL debloat?")) {
                AppxManager::RemoveAll();
                AppxManager::RemoveOneDrive();
                ServiceManager::DisableAll();
                TelemetryManager::ApplyAll();
                Utils::PrintSuccess("\n=== Full debloat complete. Reboot recommended. ===");
            }
        } else {
            Utils::PrintError("Invalid option.");
        }
    }
    return 0;
}
