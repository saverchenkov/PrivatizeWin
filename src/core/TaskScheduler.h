#pragma once

#include <windows.h>
#include <string>
#include <string_view>

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]
// R.1: RAII resource management

struct ScheduledTaskConfig {
    std::wstring taskName{ L"PrivatizeWin Auto-Protect" };
    std::wstring templateName{ L"recommended" };
    std::wstring frequency{ L"daily" };
    std::wstring userMode{ L"all" };
};

class TaskScheduler {
public:
    [[nodiscard]] static bool IsTaskInstalled(std::wstring_view taskName = L"PrivatizeWin Auto-Protect");
    static bool InstallTask(const ScheduledTaskConfig& config);
    static bool UninstallTask(std::wstring_view taskName = L"PrivatizeWin Auto-Protect");
    [[nodiscard]] static std::wstring GetExecutablePath();
};

} // namespace PrivatizeWin
