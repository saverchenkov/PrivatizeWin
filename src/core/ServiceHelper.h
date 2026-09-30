#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <optional>
#include "Types.h"
#include "SmartHandle.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view for read-only
// I.10: Use [[nodiscard]]
// R.1: RAII resource management (UniqueScHandle)

class ServiceHelper {
public:
    [[nodiscard]] static bool ServiceExists(std::wstring_view serviceName) noexcept;
    [[nodiscard]] static std::optional<uint32_t> GetServiceStartType(std::wstring_view serviceName);
    static bool SetServiceStartType(std::wstring_view serviceName, uint32_t startType);
    static bool StopService(std::wstring_view serviceName);
    static bool StartService(std::wstring_view serviceName);

    [[nodiscard]] static SettingStatus AuditAction(const ServiceAction& action);
    static bool ApplyAction(const ServiceAction& action, bool enableProtection);
};

} // namespace PrivatizeWin
