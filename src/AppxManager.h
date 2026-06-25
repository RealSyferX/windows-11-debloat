#pragma once
#include <string>
#include <vector>

struct BloatwareApp {
    std::wstring namePattern;
    std::string  displayName;
};

class AppxManager {
public:
    static const std::vector<BloatwareApp>& GetBloatwareList();
    static void List();
    static void RemoveAll();
    static void RemoveOneDrive();
};
