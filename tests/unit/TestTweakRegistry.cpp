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
