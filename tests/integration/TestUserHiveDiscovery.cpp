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
