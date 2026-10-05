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

    SERVICE_STATUS_PROCESS ssp{};
    DWORD bytesNeeded = 0;
    if (QueryServiceStatusEx(hService.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded)) {
        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            return true;
        }
    }

    SERVICE_STATUS status{};
    if (!ControlService(hService.get(), SERVICE_CONTROL_STOP, &status)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_NOT_ACTIVE) {
            return false;
        }
    }

    // Wait for the service to actually stop (poll up to 3 seconds)
    for (int i = 0; i < 30; ++i) {
        if (QueryServiceStatusEx(hService.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded)) {
            if (ssp.dwCurrentState == SERVICE_STOPPED) {
                return true;
            }
        }
        Sleep(100);
    }

    return false;
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

bool ServiceHelper::MatchesTarget(const ServiceAction& action, bool targetProtected) noexcept {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(action.serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS));
    if (!hService) {
        return false;
    }

    DWORD bytesNeeded = 0;
    QueryServiceConfigW(hService.get(), nullptr, 0, &bytesNeeded);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return false;
    }

    std::vector<BYTE> buffer(bytesNeeded);
    auto pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());
    if (!QueryServiceConfigW(hService.get(), pConfig, bytesNeeded, &bytesNeeded)) {
        return false;
    }

    if (targetProtected) {
        if (pConfig->dwStartType != action.startupTypeProtected) {
            return false;
        }
        if (action.stopService) {
            SERVICE_STATUS_PROCESS ssp{};
            if (QueryServiceStatusEx(hService.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded)) {
                return (ssp.dwCurrentState == SERVICE_STOPPED);
            }
            return false;
        }
        return true;
    } else {
        return (pConfig->dwStartType == action.startupTypeDefault);
    }
}

SettingStatus ServiceHelper::AuditAction(const ServiceAction& action) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) {
        return SettingStatus::Unknown;
    }

    std::wstring svcNameStr = ToNullTerminated(action.serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS));
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
        if (action.stopService) {
            SERVICE_STATUS_PROCESS ssp{};
            if (QueryServiceStatusEx(hService.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded)) {
                if (ssp.dwCurrentState != SERVICE_STOPPED) {
                    return SettingStatus::Partial;
                }
            }
        }
        return SettingStatus::Protected;
    }
    if (pConfig->dwStartType == action.startupTypeDefault) {
        return SettingStatus::Default;
    }
    return SettingStatus::Custom;
}

bool ServiceHelper::ApplyAction(const ServiceAction& action, bool enableProtection) {
    UniqueScHandle hSCM(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!hSCM) return false;

    std::wstring svcNameStr = ToNullTerminated(action.serviceName);
    UniqueScHandle hService(OpenServiceW(hSCM.get(), svcNameStr.c_str(), SERVICE_QUERY_CONFIG));
    if (!hService) {
        return false;
    }
    hService.reset();

    const uint32_t targetStartType = enableProtection ? action.startupTypeProtected : action.startupTypeDefault;
    const bool success = SetServiceStartType(action.serviceName, targetStartType);
    if (!success) {
        return false;
    }

    if (enableProtection && action.stopService) {
        if (!StopService(action.serviceName)) {
            return false;
        }
    }
    return true;
}

} // namespace PrivatizeWin
