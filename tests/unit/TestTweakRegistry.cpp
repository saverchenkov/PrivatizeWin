#include "../TestFramework.h"
#include "../../src/core/TweakRegistry.h"
#include "../../src/core/RegistryHelper.h"
#include "../../src/core/ServiceHelper.h"
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
    const std::wstring subKey = L"Software\\PrivatizeWin\\Test_TargetMatching_" +
        std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(GetTickCount64());

    struct KeyCleanup {
        std::wstring key;
        ~KeyCleanup() { RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str()); }
    } cleaner{ subKey };

    // Ensure clean key
    RegDeleteTreeW(HKEY_CURRENT_USER, subKey.c_str());

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
    ASSERT_EQ(RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_WRITE, &hKey), ERROR_SUCCESS);
    DWORD customVal = 2;
    RegSetValueExW(hKey, action.valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&customVal), sizeof(customVal));
    RegCloseKey(hKey);

    // Custom value must NOT match protected AND must NOT match default
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, true));
    ASSERT_FALSE(RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, false));
    ASSERT_EQ(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action), SettingStatus::Custom);

    // 4. Set to wrong type (REG_SZ instead of REG_DWORD)
    ASSERT_EQ(RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_WRITE, &hKey), ERROR_SUCCESS);
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

TEST_CASE(Unit_TweakRegistry, AliasSeparationM006AndP065) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    // M006 and P065 must have distinct canonical IDs
    ASSERT_EQ(TweakRegistry::Instance().GetCanonicalTweakId("M006"), "M006");
    ASSERT_EQ(TweakRegistry::Instance().GetCanonicalTweakId("P065"), "P065");

    // L007 and P090_MACHINE must also have distinct canonical IDs
    ASSERT_EQ(TweakRegistry::Instance().GetCanonicalTweakId("L007"), "L007");
    ASSERT_EQ(TweakRegistry::Instance().GetCanonicalTweakId("P090_MACHINE"), "P090_MACHINE");

    // Planning both M006 and P065 with consistent targets must be valid and neither is dropped
    std::map<std::string, bool> jointPlan = {
        { "M006", true },
        { "P065", true }
    };
    std::string err;
    ASSERT_TRUE(TweakRegistry::Instance().ValidatePlanConflicts(jointPlan, err));

    // Both tweaks exist and have their distinct full actions preserved
    const auto* m006 = TweakRegistry::Instance().GetTweakById("M006");
    const auto* p065 = TweakRegistry::Instance().GetTweakById("P065");
    ASSERT_TRUE(m006 != nullptr);
    ASSERT_TRUE(p065 != nullptr);
    ASSERT_EQ(m006->regActions.size(), 1);
    ASSERT_EQ(p065->regActions.size(), 2);

    bool p065Has338388 = false;
    for (const auto& ra : p065->regActions) {
        if (ra.valueName == L"SubscribedContent-338388Enabled") {
            p065Has338388 = true;
        }
    }
    ASSERT_TRUE(p065Has338388);

    // Contradictory request (M006: true, P065: false) must be rejected
    std::map<std::string, bool> conflictPlan = {
        { "M006", true },
        { "P065", false }
    };
    err.clear();
    ASSERT_FALSE(TweakRegistry::Instance().ValidatePlanConflicts(conflictPlan, err));
    ASSERT_TRUE(err.find("contradictory") != std::string::npos);
}

TEST_CASE(Unit_TweakRegistry, CatalogNoRawServiceStartRegActions) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    const auto& tweaks = TweakRegistry::Instance().GetAllTweaks();

    for (const auto& t : tweaks) {
        for (const auto& ra : t.regActions) {
            std::wstring subKeyLower = ra.subKey;
            std::transform(subKeyLower.begin(), subKeyLower.end(), subKeyLower.begin(), ::towlower);
            // Assert no raw registry write manages service Start type
            if (subKeyLower.find(L"system\\currentcontrolset\\services\\") != std::wstring::npos) {
                ASSERT_TRUE(ra.valueName != L"Start");
            }
        }
    }
}

TEST_CASE(Unit_TweakRegistry, ServiceSettingsHaveConsistentDefaultAndProtected) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    // S116 (SEMgrSvc)
    const auto* s116 = TweakRegistry::Instance().GetTweakById("S116");
    ASSERT_TRUE(s116 != nullptr);
    ASSERT_FALSE(s116->serviceActions.empty());
    ASSERT_EQ(s116->serviceActions[0].serviceName, L"SEMgrSvc");
    ASSERT_EQ(s116->serviceActions[0].startupTypeProtected, 4u);
    ASSERT_EQ(s116->serviceActions[0].startupTypeDefault, 3u); // Demand start

    // S119 (WFDSConMgrSvc)
    const auto* s119 = TweakRegistry::Instance().GetTweakById("S119");
    ASSERT_TRUE(s119 != nullptr);
    ASSERT_FALSE(s119->serviceActions.empty());
    ASSERT_EQ(s119->serviceActions[0].serviceName, L"WFDSConMgrSvc");
    ASSERT_EQ(s119->serviceActions[0].startupTypeProtected, 4u);
    ASSERT_EQ(s119->serviceActions[0].startupTypeDefault, 3u); // Demand start

    // L005_MACHINE (lfsvc)
    const auto* l005 = TweakRegistry::Instance().GetTweakById("L005_MACHINE");
    ASSERT_TRUE(l005 != nullptr);
    ASSERT_FALSE(l005->serviceActions.empty());
    ASSERT_EQ(l005->serviceActions[0].serviceName, L"lfsvc");
    ASSERT_EQ(l005->serviceActions[0].startupTypeProtected, 4u);
    ASSERT_EQ(l005->serviceActions[0].startupTypeDefault, 3u); // Demand start

    // S003 (DiagTrack + dmwappushservice)
    const auto* s003 = TweakRegistry::Instance().GetTweakById("S003");
    ASSERT_TRUE(s003 != nullptr);
    ASSERT_EQ(s003->serviceActions.size(), 2);
    ASSERT_EQ(s003->serviceActions[0].serviceName, L"DiagTrack");
    ASSERT_EQ(s003->serviceActions[0].startupTypeProtected, 4u);
    ASSERT_EQ(s003->serviceActions[0].startupTypeDefault, 2u); // Auto
    ASSERT_EQ(s003->serviceActions[1].serviceName, L"dmwappushservice");
    ASSERT_EQ(s003->serviceActions[1].startupTypeProtected, 4u);
    ASSERT_EQ(s003->serviceActions[1].startupTypeDefault, 3u); // Demand start
}

TEST_CASE(Unit_TweakRegistry, GuiPlanConflictValidation) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    std::unordered_set<std::string> enables = { "TEL_CEIP" };
    std::unordered_set<std::string> reverts = { "P027" };

    std::string err;
    const bool valid = TweakRegistry::Instance().ValidatePendingPlan(enables, reverts, err);
    ASSERT_FALSE(valid);
    ASSERT_FALSE(err.empty());
    ASSERT_TRUE(err.find("Contradictory") != std::string::npos || err.find("contradictory") != std::string::npos ||
                err.find("Conflicting") != std::string::npos || err.find("conflicting") != std::string::npos);
}

TEST_CASE(Unit_TweakRegistry, CompositeTweakWithMissingServiceAppliesAndAuditsConsistently) {
    const std::wstring testKey = L"Software\\PrivatizeWin_Test_Composite";
    const std::wstring testVal = L"CompositeSetting";

    // Clean up any old test artifacts
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testVal);
    RegistryHelper::DeleteKeyIfEmpty(HKEY_CURRENT_USER, testKey);

    std::vector<std::wstring> executedServices;
    std::vector<std::wstring> matchedServices;
    std::vector<std::wstring> auditedServices;
    uint32_t simulatedServiceStartType = 3; // 3 = default (demand), 4 = protected (disabled)

    ServiceHelper::s_testAvailabilityResolver = [](std::wstring_view svc) -> std::optional<ServiceAvailability> {
        if (svc == L"AvailableService") return ServiceAvailability::Available;
        if (svc == L"MissingService") return ServiceAvailability::Missing;
        return std::nullopt;
    };

    ServiceHelper::s_testApplyHook = [&](const ServiceAction& sa, bool enable) -> std::optional<bool> {
        executedServices.push_back(sa.serviceName);
        simulatedServiceStartType = enable ? sa.startupTypeProtected : sa.startupTypeDefault;
        return true;
    };

    ServiceHelper::s_testMatchHook = [&](const ServiceAction& sa, bool targetProtected) -> std::optional<bool> {
        matchedServices.push_back(sa.serviceName);
        const uint32_t expected = targetProtected ? sa.startupTypeProtected : sa.startupTypeDefault;
        return (simulatedServiceStartType == expected);
    };

    ServiceHelper::s_testAuditHook = [&](const ServiceAction& sa) -> std::optional<SettingStatus> {
        auditedServices.push_back(sa.serviceName);
        if (simulatedServiceStartType == sa.startupTypeProtected) return SettingStatus::Applied;
        if (simulatedServiceStartType == sa.startupTypeDefault) return SettingStatus::Default;
        return SettingStatus::Custom;
    };

    // Construct composite tweak: 1 user reg action, 1 available service, 1 missing service
    Tweak t;
    t.id = "TEST_COMPOSITE_PARTIAL_SERVICES";
    t.title = L"Composite Tweak Test";
    t.category = L"Testing";
    t.scope = TargetScope::User;
    t.regActions.push_back({ TargetScope::User, testKey, testVal, RegType::Dword, 1, 0, L"", L"", false });
    t.serviceActions.push_back({ L"AvailableService", 4, 3, false });
    t.serviceActions.push_back({ L"MissingService", 4, 3, false });
    TweakRegistry::Instance().AddTweak(t);

    // 1. Planning phase
    const auto app = TweakRegistry::Instance().GetTweakApplicability(t.id, UserSelectionMode::CurrentUser);
    ASSERT_EQ(static_cast<int>(app), static_cast<int>(TweakRegistry::TweakApplicability::Applicable));

    const auto resolved = TweakRegistry::Instance().GetResolvedActions(t.id, UserSelectionMode::CurrentUser);
    ASSERT_TRUE(resolved.HasApplicableActions());
    ASSERT_EQ(resolved.serviceActions.size(), 1);
    ASSERT_EQ(resolved.serviceActions[0].serviceName, L"AvailableService");
    ASSERT_EQ(resolved.missingServicesCount, 1);
    ASSERT_EQ(resolved.regActions.size(), 1);

    // Initial state: default
    RegistryHelper::WriteDword(HKEY_CURRENT_USER, testKey, testVal, 0);
    simulatedServiceStartType = 3;

    // Initial audit must report Default
    const auto initAudit = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
    ASSERT_EQ(static_cast<int>(initAudit), static_cast<int>(SettingStatus::Default));

    // 2. Execution phase (Enable / Apply)
    executedServices.clear();
    const auto applyRes = TweakRegistry::Instance().ApplyTweakEx(t.id, true, UserSelectionMode::CurrentUser, {});
    ASSERT_EQ(static_cast<int>(applyRes), static_cast<int>(TweakRegistry::ApplyResult::Success));

    // Assert that execution ONLY ran the available service, never attempting the missing service
    ASSERT_EQ(executedServices.size(), 1);
    ASSERT_EQ(executedServices[0], L"AvailableService");

    // Registry action was executed
    const auto regVal = RegistryHelper::ReadDword(HKEY_CURRENT_USER, testKey, testVal);
    ASSERT_TRUE(regVal.has_value());
    ASSERT_EQ(regVal.value(), 1u);

    // 3. Target verification phase
    matchedServices.clear();
    const bool matchesProtected = TweakRegistry::Instance().MatchesTargetState(t.id, true, UserSelectionMode::CurrentUser, {});
    ASSERT_TRUE(matchesProtected);
    ASSERT_EQ(matchedServices.size(), 1);
    ASSERT_EQ(matchedServices[0], L"AvailableService");

    // 4. Audit phase
    auditedServices.clear();
    const auto postApplyAudit = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
    ASSERT_EQ(static_cast<int>(postApplyAudit), static_cast<int>(SettingStatus::Applied));
    ASSERT_EQ(auditedServices.size(), 1);
    ASSERT_EQ(auditedServices[0], L"AvailableService");

    // 5. Restore / Revert phase
    executedServices.clear();
    const auto revertRes = TweakRegistry::Instance().ApplyTweakEx(t.id, false, UserSelectionMode::CurrentUser, {});
    ASSERT_EQ(static_cast<int>(revertRes), static_cast<int>(TweakRegistry::ApplyResult::Success));
    ASSERT_EQ(executedServices.size(), 1);
    ASSERT_EQ(executedServices[0], L"AvailableService");

    // Revert target verification
    const bool matchesDefault = TweakRegistry::Instance().MatchesTargetState(t.id, false, UserSelectionMode::CurrentUser, {});
    ASSERT_TRUE(matchesDefault);

    // Revert audit
    const auto postRevertAudit = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
    ASSERT_EQ(static_cast<int>(postRevertAudit), static_cast<int>(SettingStatus::Default));

    // 6. Test tweak with ONLY missing services: must report NotApplicableService
    Tweak tOnlyMissing;
    tOnlyMissing.id = "TEST_ONLY_MISSING_SERVICES";
    tOnlyMissing.title = L"Only Missing Services";
    tOnlyMissing.category = L"Testing";
    tOnlyMissing.serviceActions.push_back({ L"MissingService", 4, 3, false });
    TweakRegistry::Instance().AddTweak(tOnlyMissing);

    const auto missingApp = TweakRegistry::Instance().GetTweakApplicability(tOnlyMissing.id, UserSelectionMode::CurrentUser);
    ASSERT_EQ(static_cast<int>(missingApp), static_cast<int>(TweakRegistry::TweakApplicability::NotApplicableService));

    // Reset test hooks and cleanup registry
    ServiceHelper::s_testAvailabilityResolver = nullptr;
    ServiceHelper::s_testApplyHook = nullptr;
    ServiceHelper::s_testMatchHook = nullptr;
    ServiceHelper::s_testAuditHook = nullptr;
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testVal);
    RegistryHelper::DeleteKeyIfEmpty(HKEY_CURRENT_USER, testKey);
}




