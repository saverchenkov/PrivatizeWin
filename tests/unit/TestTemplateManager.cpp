#include "../TestFramework.h"
#include "../../src/core/TemplateManager.h"
#include "../../src/core/TweakRegistry.h"
#include <windows.h>

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
    TemplateProfile p;
    p.name = "test_profile";
    p.description = "Test profile for unit testing";
    p.tweakStates["TEL_DIAGTRACK"] = true;
    p.tweakStates["AI_COPILOT"] = false;

    wchar_t tempPath[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempPath);
    const std::wstring testFile = std::wstring(tempPath) + L"privatizewin_test_roundtrip.json";

    const bool saveOk = TemplateManager::Instance().SaveTemplateToFile(testFile, p);
    ASSERT_TRUE(saveOk);

    TemplateProfile loaded;
    const bool loadOk = TemplateManager::Instance().LoadTemplateFromFile(testFile, loaded);
    ASSERT_TRUE(loadOk);

    ASSERT_EQ(loaded.name, "test_profile");
    ASSERT_EQ(loaded.description, "Test profile for unit testing");
    ASSERT_EQ(loaded.tweakStates.size(), 2);
    ASSERT_TRUE(loaded.tweakStates["TEL_DIAGTRACK"]);
    ASSERT_FALSE(loaded.tweakStates["AI_COPILOT"]);

    DeleteFileW(testFile.c_str());
}
