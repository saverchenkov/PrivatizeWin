#include "../TestFramework.h"
#include "../../src/core/UserHiveManager.h"

using namespace PrivatizeWin;

TEST_CASE(Integration_UserHiveManager, DiscoverUserProfilesOnSystem) {
    const auto profiles = UserHiveManager::DiscoverUserProfiles();

    // Any operational Windows interactive desktop has at least 1 user profile
    ASSERT_TRUE(!profiles.empty());

    for (const auto& p : profiles) {
        ASSERT_FALSE(p.sid.empty());
        ASSERT_FALSE(p.username.empty());
        ASSERT_FALSE(p.profilePath.empty());
        // Verify SID format starts with S-1-5-21
        ASSERT_TRUE(p.sid.rfind(L"S-1-5-21-", 0) == 0);
    }
}

TEST_CASE(Integration_UserHiveManager, NonExistentUserFailsExplicitly) {
    const std::vector<std::wstring> badUsers = { L"NonExistentUser_987654321_NoSuchUser" };

    const auto res = UserHiveManager::ForEachTargetUser(
        UserSelectionMode::SpecificUsers,
        badUsers,
        [](HKEY, const UserProfile&) -> bool {
            return true;
        }
    );

    ASSERT_EQ(res.requestedUsers, static_cast<size_t>(1));
    ASSERT_EQ(res.resolvedUsers, static_cast<size_t>(0));
    ASSERT_EQ(res.failedUsers, static_cast<size_t>(1));
    ASSERT_FALSE(res.allSucceeded);

    RegistryAction action{};
    action.scope = TargetScope::User;
    action.subKey = L"Software\\PrivatizeWin_TestKey";
    action.valueName = L"TestVal";
    action.type = RegType::Dword;
    action.dwordProtected = 1;
    action.dwordDefault = 0;

    // Audit must return Unknown and NEVER fall back to HKEY_CURRENT_USER
    const SettingStatus st = UserHiveManager::AuditUserAction(action, UserSelectionMode::SpecificUsers, badUsers);
    ASSERT_TRUE(st == SettingStatus::Unknown);

    // Apply must return false because the requested user could not be resolved
    const bool applied = UserHiveManager::ApplyUserAction(action, true, UserSelectionMode::SpecificUsers, badUsers);
    ASSERT_FALSE(applied);
}

