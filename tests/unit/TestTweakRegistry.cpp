#include "../TestFramework.h"
#include "../../src/core/TweakRegistry.h"
#include <set>

using namespace PrivatizeWin;

TEST_CASE(Unit_TweakRegistry, DefaultTweaksCatalog) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    const auto& tweaks = TweakRegistry::Instance().GetAllTweaks();

    ASSERT_TRUE(tweaks.size() >= 20);

    // Verify all tweak IDs are unique
    std::set<std::string> idSet;
    for (const auto& t : tweaks) {
        ASSERT_FALSE(t.id.empty());
        ASSERT_FALSE(t.title.empty());
        ASSERT_FALSE(t.category.empty());
        ASSERT_EQ(idSet.count(t.id), 0);
        idSet.insert(t.id);
    }
}

TEST_CASE(Unit_TweakRegistry, LookupById) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    const auto* t1 = TweakRegistry::Instance().GetTweakById("TEL_DIAGTRACK");
    ASSERT_TRUE(t1 != nullptr);
    ASSERT_EQ(t1->id, "TEL_DIAGTRACK");
    ASSERT_EQ(t1->category, L"Telemetry & Diagnostics");

    const auto* t2 = TweakRegistry::Instance().GetTweakById("NON_EXISTENT_ID");
    ASSERT_TRUE(t2 == nullptr);
}

TEST_CASE(Unit_TweakRegistry, FilterByCategory) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    const auto all = TweakRegistry::Instance().GetTweaksByCategory(L"All Settings");
    ASSERT_EQ(all.size(), TweakRegistry::Instance().GetAllTweaks().size());

    const auto ai = TweakRegistry::Instance().GetTweaksByCategory(L"AI & Copilot");
    ASSERT_TRUE(ai.size() >= 2);
    for (const auto& t : ai) {
        ASSERT_EQ(t.category, L"AI & Copilot");
    }
}

TEST_CASE(Unit_TweakRegistry, LoadCustomTweaksFromJson) {
    const std::string customJson = R"({
        "tweaks": [
            {
                "id": "CUSTOM_TWEAK_01",
                "title": "Custom Test Tweak",
                "category": "Testing",
                "description": "Test description",
                "impact": "No impact",
                "safety": "safe"
            }
        ]
    })";

    const bool ok = TweakRegistry::Instance().LoadExternalTweaks(customJson);
    ASSERT_TRUE(ok);

    const auto* custom = TweakRegistry::Instance().GetTweakById("CUSTOM_TWEAK_01");
    ASSERT_TRUE(custom != nullptr);
    ASSERT_EQ(custom->id, "CUSTOM_TWEAK_01");
    ASSERT_EQ(custom->category, L"Testing");
}

TEST_CASE(Unit_TweakRegistry, SplitUserMachineAndNoPartialState) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    const auto& tweaks = TweakRegistry::Instance().GetAllTweaks();

    ASSERT_TRUE(tweaks.size() >= 300);

    for (const auto& t : tweaks) {
        // Assert scope is strictly User, Machine, or Service
        ASSERT_TRUE(t.scope == TargetScope::User || t.scope == TargetScope::Machine || t.scope == TargetScope::Service);

        // Audit state is strictly all-or-nothing (never returns Partial)
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        ASSERT_TRUE(st == SettingStatus::Applied || st == SettingStatus::NotApplied ||
                    st == SettingStatus::Unknown || st == SettingStatus::NotApplicable);
        ASSERT_TRUE(st != SettingStatus::Partial);

        if (t.id.ends_with("_USER")) {
            ASSERT_TRUE(t.scope == TargetScope::User);
            ASSERT_TRUE(t.title.find(L"(User)") != std::wstring::npos);
            const std::string machId = t.id.substr(0, t.id.length() - 5) + "_MACHINE";
            const auto* machTweak = TweakRegistry::Instance().GetTweakById(machId);
            ASSERT_TRUE(machTweak != nullptr);
            ASSERT_TRUE(machTweak->scope == TargetScope::Machine);
            ASSERT_TRUE(machTweak->title.find(L"(Machine)") != std::wstring::npos);
        }
    }
}

TEST_CASE(Unit_TweakRegistry, PartialStatusSupport) {
    ASSERT_TRUE(SettingStatus::Mixed == SettingStatus::Partial);
    ASSERT_TRUE(SettingStatus::Partial != SettingStatus::Applied);
    ASSERT_TRUE(SettingStatus::Partial != SettingStatus::NotApplied);
}

TEST_CASE(Unit_TweakRegistry, NotApplicableEvaluation) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    // Non-existent tweak evaluates to NotApplicable
    ASSERT_EQ(TweakRegistry::Instance().AuditTweak("NON_EXISTENT_ID", UserSelectionMode::CurrentUser, {}),
              SettingStatus::NotApplicable);

    // Tweak with no actions evaluates to NotApplicable
    const auto* p015 = TweakRegistry::Instance().GetTweakById("P015");
    ASSERT_TRUE(p015 != nullptr);
    ASSERT_TRUE(p015->regActions.empty() && p015->serviceActions.empty());
    ASSERT_EQ(TweakRegistry::Instance().AuditTweak("P015", UserSelectionMode::CurrentUser, {}),
              SettingStatus::NotApplicable);
}


