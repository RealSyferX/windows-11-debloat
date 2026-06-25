#pragma once
#include <string>
#include <windows.h>

namespace Utils {
    bool IsElevated();
    bool RelaunchAsAdmin();
    std::string RunPowerShell(const std::wstring& script);
    void SetColor(WORD attr);
    void PrintInfo(const std::string& msg);
    void PrintSuccess(const std::string& msg);
    void PrintError(const std::string& msg);
    void PrintWarning(const std::string& msg);
    void PrintHeader(const std::string& msg);
    bool AskYesNo(const std::string& prompt);
    std::wstring StringToWide(const std::string& s);
    std::string WideToString(const std::wstring& ws);
    void CreateRestorePoint();
}
