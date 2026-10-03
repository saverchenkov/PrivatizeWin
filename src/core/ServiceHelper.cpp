#include "ServiceHelper.h"
#include <vector>

namespace PrivatizeWin {

static std::wstring ToNullTerminated(std::wstring_view sv) {
    return std::wstring(sv);
}

bool ServiceHelper::ServiceExists(std::wstring_view serviceName) noexcept {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return true; // Inaccessible SCM: do not assume missing

    std::wstring svcNameStr = ToNullTerminated(serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    if (hService.isValid()) {
        return true;
    }
    const DWORD err = GetLastError();
    if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
        return false;
    }
    // Access denied or other query limitation means service exists
    return true;
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
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) {
        return SettingStatus::Unknown;
    }

    std::wstring svcNameStr = ToNullTerminated(action.serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    if (!hService) {
        const DWORD err = GetLastError();
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            return SettingStatus::NotApplicable;
        }
        return SettingStatus::Unknown; // Access denied or query failed
    }

    DWORD bytesNeeded = 0;
    QueryServiceConfigW(hService.get(), nullptr, 0, &bytesNeeded);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return SettingStatus::Unknown;
    }

    std::vector<BYTE> buffer(bytesNeeded);
    auto pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());
    if (!QueryServiceConfigW(hService.get(), pConfig, bytesNeeded, &bytesNeeded)) {
        return SettingStatus::Unknown;
    }

    if (pConfig->dwStartType == action.startupTypeProtected) {
        return SettingStatus::Protected;
    }
    if (pConfig->dwStartType == action.startupTypeDefault) {
        return SettingStatus::Default;
    }
    return SettingStatus::Default;
}

bool ServiceHelper::ApplyAction(const ServiceAction& action, bool enableProtection) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(action.serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    if (!hService) {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            return true; // Missing service is safely skipped
        }
        return false; // Access denied
    }
    hService.reset();

    const uint32_t targetStartType = enableProtection ? action.startupTypeProtected : action.startupTypeDefault;
    const bool success = SetServiceStartType(action.serviceName, targetStartType);
    if (!success) {
        return false;
    }

    if (enableProtection && action.stopService) {
        StopService(action.serviceName);
    }
    return true;
}

} // namespace PrivatizeWin
