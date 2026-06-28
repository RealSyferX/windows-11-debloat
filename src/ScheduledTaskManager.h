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
    static void EnableAll();

private:
    // Shared implementation for DisableAll/EnableAll. Builds the $tasks array,
    // runs the named cmdlet (Enable/Disable-ScheduledTask) over each entry, and
    // prints the result. `enable` selects the cmdlet and the user-facing
    // messages.
    static void SetTaskState(bool enable);
};
