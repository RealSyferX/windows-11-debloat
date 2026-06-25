#pragma once
#include <string>
#include <vector>
#include <windows.h>

enum RegTweakType { REG_TWEAK_DWORD, REG_TWEAK_SZ };

struct RegistryTweak {
    HKEY         rootKey;
    std::wstring subKey;
    std::wstring valueName;
    RegTweakType type;
    DWORD        dwordData;
    std::wstring stringData;
    std::string  description;
};

class TelemetryManager {
public:
    static const std::vector<RegistryTweak>& GetRegistryTweaks();
    static void List();
    static void ApplyAll();
};
