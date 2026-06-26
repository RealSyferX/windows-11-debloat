#pragma once
#include <string>
#include <vector>

struct TelemetryTask {
    std::wstring name;
    std::wstring path;
    std::string  description;
};

class ScheduledTaskManager {
public:
    static const std::vector<TelemetryTask>& GetTasks();
    static void List();
    static void DisableAll();
};
