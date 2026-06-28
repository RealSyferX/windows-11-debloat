#pragma once

class PerformanceManager {
public:
    static void List();
    static void ApplyAll();
    static void Revert();   // Revert: re-enable hibernation, fast startup, restore power plan
};
