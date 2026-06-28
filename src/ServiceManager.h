#pragma once
#include <string>
#include <vector>

struct TelemetryService {
    std::wstring name;
    std::string  displayName;
    std::string  description;
};

class ServiceManager {
public:
    static const std::vector<TelemetryService>& GetTelemetryServices();
    static void List();
    static void DisableAll();
    static void DeleteAll();
    static void EnableAll();   // Revert: restore previously-disabled services
};
