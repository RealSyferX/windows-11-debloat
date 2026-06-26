#pragma once
#include <string>
#include <vector>

class HostsManager {
public:
    static const std::vector<std::wstring>& GetBlockedDomains();
    static void List();
    static void Apply();
};
