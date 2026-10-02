#include "../TestFramework.h"
#include "../../src/core/TemplateManager.h"
#include "../../src/core/TweakRegistry.h"
#include <windows.h>
#include <fstream>

using namespace PrivatizeWin;

TEST_CASE(Unit_TemplateManager, BuiltinPresetsInitialization) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    const auto names = TemplateManager::Instance().GetAvailableTemplateNames();
    ASSERT_TRUE(names.size() >= 4);

    const auto rec = TemplateManager::Instance().GetTemplate("recommended");
    ASSERT_TRUE(rec.has_value());
    ASSERT_TRUE(rec->isBuiltin);
    ASSERT_TRUE(!rec->tweakStates.empty());

    const auto strict = TemplateManager::Instance().GetTemplate("strict");
    ASSERT_TRUE(strict.has_value());

    const auto min = TemplateManager::Instance().GetTemplate("minimal");
    ASSERT_TRUE(min.has_value());

    const auto def = TemplateManager::Instance().GetTemplate("defaults");
    ASSERT_TRUE(def.has_value());
}

TEST_CASE(Unit_TemplateManager, FileSaveAndLoadRoundTrip) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateProfile p;
    p.name = "test_profile";
    p.description = "Test profile for unit testing";
    p.tweakStates["TEL_DIAGTRACK"] = true;
    p.tweakStates["AI_COPILOT_USER"] = false;

    wchar_t tempPath[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempPath);
    const std::wstring testFile = std::wstring(tempPath) + L"privatizewin_test_roundtrip.json";

    const bool saveOk = TemplateManager::Instance().SaveTemplateToFile(testFile, p);
    ASSERT_TRUE(saveOk);

    TemplateProfile loaded;
    std::string err;
    const bool loadOk = TemplateManager::Instance().LoadTemplateFromFile(testFile, loaded, &err);
    ASSERT_TRUE(loadOk);

    ASSERT_EQ(loaded.name, "test_profile");
    ASSERT_EQ(loaded.description, "Test profile for unit testing");
    ASSERT_EQ(loaded.tweakStates.size(), 2);
    ASSERT_TRUE(loaded.tweakStates["TEL_DIAGTRACK"]);
    ASSERT_FALSE(loaded.tweakStates["AI_COPILOT_USER"]);

    DeleteFileW(testFile.c_str());
}

TEST_CASE(Unit_TemplateManager, PresetsAreSparseAndSafe) {
    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    const auto rec = TemplateManager::Instance().GetTemplate("recommended");
    ASSERT_TRUE(rec.has_value());
    // Recommended must only contain 'true' (sparse)
    for (const auto& [id, state] : rec->tweakStates) {
        ASSERT_TRUE(state);
    }
    // Acceptance test: Recommended must NOT disable Windows Updates or Defender
    ASSERT_FALSE(rec->tweakStates.contains("A004_USER"));
    ASSERT_FALSE(rec->tweakStates.contains("A004_MACHINE"));
    ASSERT_FALSE(rec->tweakStates.contains("A005"));
    ASSERT_FALSE(rec->tweakStates.contains("S011"));

    const auto min = TemplateManager::Instance().GetTemplate("minimal");
    ASSERT_TRUE(min.has_value());
    // Minimal must be strictly sparse: exactly 11 telemetry tweaks
    ASSERT_EQ(min->tweakStates.size(), 11);
    for (const auto& [id, state] : min->tweakStates) {
        ASSERT_TRUE(state);
    }
}

TEST_CASE(Unit_TemplateManager, RejectInvalidProfiles) {
    TweakRegistry::Instance().InitializeDefaultTweaks();

    wchar_t tempPath[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempPath);

    // 1. Non-boolean value
    {
        const std::wstring testFile = std::wstring(tempPath) + L"privatizewin_test_bad_bool.json";
        const std::string badJson = R"({
            "name": "bad",
            "tweaks": {
                "TEL_DIAGTRACK": "true"
            }
        })";
        std::ofstream f(testFile);
        f << badJson;
        f.close();

        TemplateProfile p;
        std::string err;
        const bool ok = TemplateManager::Instance().LoadTemplateFromFile(testFile, p, &err);
        ASSERT_FALSE(ok);
        ASSERT_TRUE(err.find("non-boolean") != std::string::npos);
        DeleteFileW(testFile.c_str());
    }

    // 2. Unknown tweak ID
    {
        const std::wstring testFile = std::wstring(tempPath) + L"privatizewin_test_unknown_id.json";
        const std::string badJson = R"({
            "name": "bad",
            "tweaks": {
                "UNKNOWN_TWEAK_ID_XYZ": true
            }
        })";
        std::ofstream f(testFile);
        f << badJson;
        f.close();

        TemplateProfile p;
        std::string err;
        const bool ok = TemplateManager::Instance().LoadTemplateFromFile(testFile, p, &err);
        ASSERT_FALSE(ok);
        ASSERT_TRUE(err.find("Unknown tweak ID") != std::string::npos);
        DeleteFileW(testFile.c_str());
    }
}
