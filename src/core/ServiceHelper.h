#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <optional>
#include <functional>
#include "Types.h"
#include "SmartHandle.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view for read-only
// I.10: Use [[nodiscard]]
// R.1: RAII resource management (UniqueScHandle)

enum class ServiceAvailability {
    Available,
    Inaccessible,
    Missing
};

class ServiceHelper {
public:
    [[nodiscard]] static ServiceAvailability GetServiceAvailability(std::wstring_view serviceName) noexcept;
    [[nodiscard]] static bool ServiceExists(std::wstring_view serviceName) noexcept;
    [[nodiscard]] static std::optional<uint32_t> GetServiceStartType(std::wstring_view serviceName);
    static bool SetServiceStartType(std::wstring_view serviceName, uint32_t startType);
    static bool StopService(std::wstring_view serviceName);
    static bool StartService(std::wstring_view serviceName);

    [[nodiscard]] static SettingStatus AuditAction(const ServiceAction& action);
    [[nodiscard]] static bool MatchesTarget(const ServiceAction& action, bool targetProtected) noexcept;
    static bool ApplyAction(const ServiceAction& action, bool enableProtection);

#ifdef PRIVATIZEWIN_ENABLE_TEST_HOOKS
    // Test Hooks for deterministic unit testing without mutating system services
    using AvailabilityResolver = std::function<std::optional<ServiceAvailability>(std::wstring_view serviceName)>;
    using ApplyHook = std::function<std::optional<bool>(const ServiceAction& action, bool enableProtection)>;
    using MatchHook = std::function<std::optional<bool>(const ServiceAction& action, bool targetProtected)>;
    using AuditHook = std::function<std::optional<SettingStatus>(const ServiceAction& action)>;

    inline static AvailabilityResolver s_testAvailabilityResolver = nullptr;
    inline static ApplyHook s_testApplyHook = nullptr;
    inline static MatchHook s_testMatchHook = nullptr;
    inline static AuditHook s_testAuditHook = nullptr;
#endif
};

} // namespace PrivatizeWin
