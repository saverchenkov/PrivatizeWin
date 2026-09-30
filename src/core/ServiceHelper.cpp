#include "ServiceHelper.h"
#include <vector>

namespace PrivatizeWin {

static std::wstring ToNullTerminated(std::wstring_view sv) {
    return std::wstring(sv);
}

bool ServiceHelper::ServiceExists(std::wstring_view serviceName) noexcept {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    return hService.isValid();
}

std::optional<uint32_t> ServiceHelper::GetServiceStartType(std::wstring_view serviceName) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return std::nullopt;

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    if (!hService) {
        return std::nullopt;
    }

    DWORD bytesNeeded = 0;
    QueryServiceConfigW(hService.get(), nullptr, 0, &bytesNeeded);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return std::nullopt;
    }

    std::vector<BYTE> buffer(bytesNeeded);
    auto pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());

    if (QueryServiceConfigW(hService.get(), pConfig, bytesNeeded, &bytesNeeded)) {
        return static_cast<uint32_t>(pConfig->dwStartType);
    }

    return std::nullopt;
}

bool ServiceHelper::SetServiceStartType(std::wstring_view serviceName, uint32_t startType) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_CHANGE_CONFIG));
    if (!hService) {
        return false;
    }

    const BOOL success = ChangeServiceConfigW(
        hService.get(),
        SERVICE_NO_CHANGE,
        startType,
        SERVICE_NO_CHANGE,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    return (success == TRUE);
}

bool ServiceHelper::StopService(std::wstring_view serviceName) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS));
    if (!hService) {
        return false;
    }

    SERVICE_STATUS status{};
    const BOOL success = ControlService(hService.get(), SERVICE_CONTROL_STOP, &status);
    return (success == TRUE);
}

bool ServiceHelper::StartService(std::wstring_view serviceName) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_START));
    if (!hService) {
        return false;
    }

    const BOOL success = ::StartServiceW(hService.get(), 0, nullptr);
    return (success == TRUE);
}

SettingStatus ServiceHelper::AuditAction(const ServiceAction& action) {
    if (!ServiceExists(action.serviceName)) {
        return SettingStatus::Protected;
    }

    const auto startType = GetServiceStartType(action.serviceName);
    if (!startType.has_value()) {
        return SettingStatus::Default;
    }

    if (startType.value() == action.startupTypeProtected) {
        return SettingStatus::Protected;
    }
    return SettingStatus::Default;
}

bool ServiceHelper::ApplyAction(const ServiceAction& action, bool enableProtection) {
    if (!ServiceExists(action.serviceName)) {
        return true;
    }

    const uint32_t targetStartType = enableProtection ? action.startupTypeProtected : action.startupTypeDefault;
    const bool success = SetServiceStartType(action.serviceName, targetStartType);

    if (enableProtection && action.stopService) {
        StopService(action.serviceName);
    }
    return success;
}

} // namespace PrivatizeWin
