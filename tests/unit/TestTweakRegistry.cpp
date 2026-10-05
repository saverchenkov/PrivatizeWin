#include "../TestFramework.h"
#include "../../src/core/TweakRegistry.h"
#include "../../src/core/RegistryHelper.h"
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

        // Audit state returns a valid SettingStatus enum value
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        ASSERT_TRUE(st == SettingStatus::Applied || st == SettingStatus::NotApplied ||
                    st == SettingStatus::Partial || st == SettingStatus::Unknown ||
                    st == SettingStatus::NotApplicable || st == SettingStatus::Custom);

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

TEST_CASE(Unit_TweakRegistry, CeipDisablesCorrectly) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    for (const char* id : { "TEL_CEIP", "P027" }) {
        const auto* t = TweakRegistry::Instance().GetTweakById(id);
        ASSERT_TRUE(t != nullptr);
        ASSERT_TRUE(!t->regActions.empty());

        bool foundMainKey = false;
        bool foundPolicyKey = false;

        for (const auto& a : t->regActions) {
            if (a.valueName == L"CEIPEnable") {
                ASSERT_EQ(a.dwordProtected, 0u); // 0 = disabled (protected)
                ASSERT_EQ(a.dwordDefault, 1u);   // 1 = enabled (Windows default)
                if (a.subKey.find(L"SQMClient\\Windows") != std::wstring::npos &&
                    a.subKey.find(L"Policies") == std::wstring::npos) {
                    foundMainKey = true;
                }
                if (a.subKey.find(L"Policies") != std::wstring::npos) {
                    foundPolicyKey = true;
                }
            }
        }
        ASSERT_TRUE(foundMainKey);
        ASSERT_TRUE(foundPolicyKey);
    }
}

TEST_CASE(Unit_TweakRegistry, SupportedSettingsVerification) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    // U006
    const auto* u006 = TweakRegistry::Instance().GetTweakById("U006");
    ASSERT_TRUE(u006 != nullptr);
    ASSERT_FALSE(u006->regActions.empty());
    ASSERT_EQ(u006->regActions[0].valueName, L"LimitDiagnosticLogCollection");
    ASSERT_EQ(u006->regActions[0].dwordProtected, 1u);

    // U007
    const auto* u007 = TweakRegistry::Instance().GetTweakById("U007");
    ASSERT_TRUE(u007 != nullptr);
    ASSERT_FALSE(u007->regActions.empty());
    ASSERT_EQ(u007->regActions[0].valueName, L"DisableOneSettingsDownloads");
    ASSERT_EQ(u007->regActions[0].dwordProtected, 1u);

    // U008
    const auto* u008 = TweakRegistry::Instance().GetTweakById("U008");
    ASSERT_TRUE(u008 != nullptr);
    ASSERT_FALSE(u008->regActions.empty());
    ASSERT_EQ(u008->regActions[0].valueName, L"AllowDeviceNameInTelemetry");
    ASSERT_EQ(u008->regActions[0].dwordProtected, 0u);

    // P015
    const auto* p015 = TweakRegistry::Instance().GetTweakById("P015");
    ASSERT_TRUE(p015 != nullptr);
    ASSERT_FALSE(p015->regActions.empty());
    ASSERT_EQ(p015->regActions[0].valueName, L"HttpAcceptLanguageOptOut");
    ASSERT_EQ(p015->regActions[0].dwordProtected, 1u);

    // A004_MACHINE: authoritative service action, no conflicting regAction
    const auto* a004 = TweakRegistry::Instance().GetTweakById("A004_MACHINE");
    ASSERT_TRUE(a004 != nullptr);
    ASSERT_TRUE(a004->regActions.empty());
    ASSERT_FALSE(a004->serviceActions.empty());
    ASSERT_EQ(a004->serviceActions[0].serviceName, L"wuauserv");
    ASSERT_EQ(a004->serviceActions[0].startupTypeProtected, 4u); // disabled
    ASSERT_EQ(a004->serviceActions[0].startupTypeDefault, 3u);   // demand start (manual)
}

TEST_CASE(Unit_TweakRegistry, NotApplicableEvaluation) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    // Non-existent tweak evaluates to NotApplicable
    ASSERT_EQ(TweakRegistry::Instance().AuditTweak("NON_EXISTENT_ID", UserSelectionMode::CurrentUser, {}),
              SettingStatus::NotApplicable);

    // Dynamic tweak with no actions evaluates to NotApplicable and Apply fails
    TweakRegistry::Instance().LoadExternalTweaks(R"({
        "tweaks": [
            {
                "id": "ZERO_ACTION_TEST",
                "title": "Zero Action Tweak",
                "category": "Testing",
                "safety": "safe"
            }
        ]
    })");

    ASSERT_EQ(TweakRegistry::Instance().AuditTweak("ZERO_ACTION_TEST", UserSelectionMode::CurrentUser, {}),
              SettingStatus::NotApplicable);
    ASSERT_FALSE(TweakRegistry::Instance().ApplyTweak("ZERO_ACTION_TEST", true, UserSelectionMode::CurrentUser, {}));
}

TEST_CASE(Unit_TweakRegistry, TargetMatchingCustomAndWrongType) {
    const wchar_t* subKey = L"Software\\PrivatizeWin\\Test_TargetMatching";
    // Ensure clean key
    RegDeleteTreeW(HKEY_CURRENT_USER, subKey);

    RegistryAction action;
    action.subKey = subKey;
    action.valueName = L"SettingVal";
    action.type = RegType::Dword;
    action.dwordProtected = 1;
    action.dwordDefault = 0;
    action.deleteOnDefault = false;

    // 1. Initial absent state: when deleteOnDefault is false, missing value does NOT match default
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::Custom);

    // 1b. Apply default (0)
    ASSERT_TRUE(RegistryHelper::ApplyAction(HKEY_CURRENT_USER, action, false));
    ASSERT_TRUE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::NotApplied);

    // 2. Set to protected value 1
    ASSERT_TRUE(RegistryHelper::ApplyAction(HKEY_CURRENT_USER, action, true));
    ASSERT_TRUE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::Applied);

    // 3. Set to custom value 2 (neither protected 1 nor default 0)
    HKEY hKey = nullptr;
    ASSERT_EQ(RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_WRITE, &hKey), ERROR_SUCCESS);
    DWORD customVal = 2;
    RegSetValueExW(hKey, action.valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&customVal), sizeof(customVal));
    RegCloseKey(hKey);

    // Custom value must NOT match protected AND must NOT match default
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::Custom);

    // 4. Set to wrong type (REG_SZ instead of REG_DWORD)
    ASSERT_EQ(RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_WRITE, &hKey), ERROR_SUCCESS);
    const wchar_t szVal[] = L"1";
    RegSetValueExW(hKey, action.valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(szVal), sizeof(szVal));
    RegCloseKey(hKey);

    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::Custom);

    // 5. Test deleteOnDefault = true
    action.deleteOnDefault = true;
    // With value still present, it does NOT match default
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    // Apply default (which deletes the value)
    ASSERT_TRUE(RegistryHelper::ApplyAction(HKEY_CURRENT_USER, action, false));
    // Now absent, matches default
    ASSERT_TRUE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));

    // Clean up test key
    RegDeleteTreeW(HKEY_CURRENT_USER, subKey);
}

TEST_CASE(Unit_TweakRegistry, PlanConflictDetection) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    // 1. Contradictory requests for aliases (TEL_CEIP and P027)
    std::map<std::string, bool> conflictingPlan = {
        { "TEL_CEIP", true },
        { "P027", false }
    };
    std::string conflictErr;
    ASSERT_FALSE(TweakRegistry::Instance().ValidatePlanConflicts(conflictingPlan, conflictErr));
    ASSERT_FALSE(conflictErr.empty());

    // 2. Consistent requests for aliases
    std::map<std::string, bool> consistentPlan = {
        { "TEL_CEIP", true },
        { "P027", true }
    };
    conflictErr.clear();
    ASSERT_TRUE(TweakRegistry::Instance().ValidatePlanConflicts(consistentPlan, conflictErr));
    ASSERT_TRUE(conflictErr.empty());
}

TEST_CASE(Unit_TweakRegistry, UsersNoneApplicabilityAndExecution) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    // User tweak with UserSelectionMode::NoUsers
    const auto appUser = TweakRegistry::Instance().GetTweakApplicability("PRIV_AD_ID_USER", UserSelectionMode::NoUsers);
    ASSERT_EQ(appUser, TweakRegistry::TweakApplicability::NotApplicableUserScope);

    // Machine tweak with UserSelectionMode::NoUsers
    const auto appMachine = TweakRegistry::Instance().GetTweakApplicability("TEL_DIAGTRACK", UserSelectionMode::NoUsers);
    ASSERT_EQ(appMachine, TweakRegistry::TweakApplicability::Applicable);

    // ApplyTweakEx with UserSelectionMode::NoUsers returns NotApplicable for user-only tweak
    const auto res = TweakRegistry::Instance().ApplyTweakEx("PRIV_AD_ID_USER", true, UserSelectionMode::NoUsers, {});
    ASSERT_EQ(res, TweakRegistry::ApplyResult::NotApplicable);
}



