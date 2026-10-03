#include "../TestFramework.h"
#include "core/Localization.h"

using namespace PrivatizeWin;

TEST_CASE(Localization, Basic) {
    auto& loc = Localization::Instance();
    loc.SetLanguage(Language::English);

    ASSERT_EQ(loc.GetCurrentLanguage(), Language::English);
    ASSERT_EQ(loc.Get("app_title"), L"PrivatizeWin \u2014 Windows Privacy Settings");
    ASSERT_EQ(loc.GetCategory(L"Telemetry & Diagnostics"), L"Telemetry & Diagnostics");

    std::wstring fmt = loc.Format("apply_success", { L"5", L"2", L"0" });
    ASSERT_TRUE(fmt.find(L"Applied: 5") != std::wstring::npos);
    ASSERT_TRUE(fmt.find(L"Restored: 2") != std::wstring::npos);
}

TEST_CASE(Localization, German) {
    auto& loc = Localization::Instance();
    loc.SetLanguage(Language::German);

    ASSERT_EQ(loc.GetCurrentLanguage(), Language::German);
    ASSERT_EQ(loc.Get("btn_apply_preset"), L"Profil an&wenden");
    ASSERT_EQ(loc.Get("filter_all"), L"Alle Einstellungen");
    ASSERT_EQ(loc.GetCategory(L"Telemetry & Diagnostics"), L"Telemetrie & Diagnose");
    ASSERT_EQ(loc.GetCategory(L"All Settings"), L"Alle Einstellungen");

    // Check tweak title in German
    std::wstring title = loc.GetTweakTitle("TEL_DIAGTRACK", L"Default");
    ASSERT_TRUE(title.find(L"Dienst") != std::wstring::npos || title.find(L"deaktivieren") != std::wstring::npos);
}

TEST_CASE(Localization, Russian) {
    auto& loc = Localization::Instance();
    loc.SetLanguage(Language::Russian);

    ASSERT_EQ(loc.GetCurrentLanguage(), Language::Russian);
    ASSERT_EQ(loc.Get("btn_apply_preset"), L"&Применить пресет");
    ASSERT_EQ(loc.Get("filter_all"), L"Все настройки");
    ASSERT_EQ(loc.GetCategory(L"Telemetry & Diagnostics"), L"Телеметрия и диагностика");
    ASSERT_EQ(loc.GetCategory(L"All Settings"), L"Все настройки");

    // Check tweak title in Russian
    std::wstring title = loc.GetTweakTitle("TEL_DIAGTRACK", L"Default");
    ASSERT_TRUE(title.find(L"Отключить") != std::wstring::npos);
}

TEST_CASE(Localization, AllLanguagesAvailable) {
    auto& loc = Localization::Instance();
    const auto& langs = loc.GetSupportedLanguages();
    ASSERT_EQ(langs.size(), 12);

    for (const auto& l : langs) {
        loc.SetLanguage(l.lang);
        ASSERT_EQ(loc.GetCurrentLanguage(), l.lang);
        std::wstring title = loc.Get("app_title");
        ASSERT_FALSE(title.empty());
        std::wstring cat = loc.GetCategory(L"AI & Copilot");
        ASSERT_FALSE(cat.empty());
    }

    // Reset to English
    loc.SetLanguage(Language::English);
}
