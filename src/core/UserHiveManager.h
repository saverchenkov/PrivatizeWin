#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include "Types.h"
#include "SmartHandle.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]
// R.1: RAII resource management (UniqueHandle, UniqueHKey)

struct UserProfile {
    std::wstring sid;
    std::wstring username;
    std::wstring profilePath;
    bool isLoaded{ false };
};

struct UserTargetResult {
    size_t requestedUsers{ 0 };
    size_t resolvedUsers{ 0 };
    size_t mountedUsers{ 0 };
    size_t failedUsers{ 0 };
    bool allSucceeded{ true };
};

class UserHiveManager {
public:
    static bool EnablePrivilege(std::wstring_view privilegeName);
    [[nodiscard]] static std::vector<UserProfile> DiscoverUserProfiles();
    
    static UserTargetResult ForEachTargetUser(
        UserSelectionMode mode,
        const std::vector<std::wstring>& specificUsers,
        const std::function<bool(HKEY hUserRoot, const UserProfile& profile)>& callback
    );

    [[nodiscard]] static SettingStatus AuditUserAction(
        const RegistryAction& action,
        UserSelectionMode mode,
        const std::vector<std::wstring>& specificUsers
    );

    static bool ApplyUserAction(
        const RegistryAction& action,
        bool enableProtection,
        UserSelectionMode mode,
        const std::vector<std::wstring>& specificUsers
    );
};

} // namespace PrivatizeWin
