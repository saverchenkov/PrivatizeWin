#include "Localization.h"
#include "LocalizationData.h"
#include "../../res/resource.h"
#include <algorithm>
#include <sstream>

namespace PrivatizeWin {

Localization& Localization::Instance() {
    static Localization instance;
    return instance;
}

Localization::Localization() {
    m_languages = {
        { Language::English,           "en", L"English",             L"English",              IDM_LANG_EN },
        { Language::German,            "de", L"Deutsch",             L"German",               IDM_LANG_DE },
        { Language::Russian,           "ru", L"Русский",             L"Russian",              IDM_LANG_RU },
        { Language::French,            "fr", L"Français",            L"French",               IDM_LANG_FR },
        { Language::Spanish,           "es", L"Español",             L"Spanish",              IDM_LANG_ES },
        { Language::Italian,           "it", L"Italiano",            L"Italian",              IDM_LANG_IT },
        { Language::Portuguese,        "pt", L"Português (Brasil)",  L"Portuguese (Brazil)",  IDM_LANG_PT },
        { Language::ChineseSimplified, "zh", L"简体中文",             L"Chinese (Simplified)", IDM_LANG_ZH },
        { Language::Japanese,          "ja", L"日本語",               L"Japanese",             IDM_LANG_JA },
        { Language::Polish,            "pl", L"Polski",              L"Polish",               IDM_LANG_PL },
        { Language::Turkish,           "tr", L"Türkçe",              L"Turkish",              IDM_LANG_TR },
        { Language::Ukrainian,         "uk", L"Українська",          L"Ukrainian",            IDM_LANG_UK }
    };

    BuildLookupTables();
}

void Localization::Initialize() {
    LoadPreference();
}

void Localization::BuildLookupTables() {
    constexpr int langCount = static_cast<int>(Language::Count);
    m_uiStringMap.resize(langCount);
    m_categoryMap.resize(langCount);

    for (const auto& entry : g_uiStrings) {
        for (int l = 0; l < langCount; ++l) {
            if (entry.values[l] && entry.values[l][0] != L'\0') {
                m_uiStringMap[l][entry.key] = entry.values[l];
            }
        }
    }

    for (const auto& cat : g_categories) {
        const std::wstring enName = cat.values[0];
        for (int l = 0; l < langCount; ++l) {
            if (cat.values[l] && cat.values[l][0] != L'\0') {
                m_categoryMap[l][enName] = cat.values[l];
            }
        }
    }

    for (const auto& tweak : g_tweakTitles) {
        if (tweak.de && tweak.de[0] != L'\0') {
            m_tweakTitlesDe[tweak.id] = tweak.de;
        }
        if (tweak.ru && tweak.ru[0] != L'\0') {
            m_tweakTitlesRu[tweak.id] = tweak.ru;
        }
    }
}

Language Localization::DetectSystemLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD prim = PRIMARYLANGID(langId);

    switch (prim) {
    case LANG_GERMAN:    return Language::German;
    case LANG_RUSSIAN:   return Language::Russian;
    case LANG_FRENCH:    return Language::French;
    case LANG_SPANISH:   return Language::Spanish;
    case LANG_ITALIAN:   return Language::Italian;
    case LANG_PORTUGUESE:return Language::Portuguese;
    case LANG_CHINESE:   return Language::ChineseSimplified;
    case LANG_JAPANESE:  return Language::Japanese;
    case LANG_POLISH:    return Language::Polish;
    case LANG_TURKISH:   return Language::Turkish;
    case LANG_UKRAINIAN: return Language::Ukrainian;
    default:             return Language::English;
    }
}

void Localization::SetLanguage(Language lang, bool persist) {
    const int idx = static_cast<int>(lang);
    if (idx >= 0 && idx < static_cast<int>(Language::Count)) {
        m_currentLang = lang;
        if (persist) {
            SavePreference();
        }
    }
}

void Localization::SetLanguageByCode(std::string_view code, bool persist) {
    for (const auto& info : m_languages) {
        if (info.code == code) {
            SetLanguage(info.lang, persist);
            return;
        }
    }
}

const LanguageInfo& Localization::GetCurrentLanguageInfo() const {
    const size_t idx = static_cast<size_t>(m_currentLang);
    if (idx < m_languages.size()) {
        return m_languages[idx];
    }
    return m_languages[0];
}

std::wstring Localization::Get(std::string_view key) const {
    const int curIdx = static_cast<int>(m_currentLang);
    const auto it = m_uiStringMap[curIdx].find(std::string(key));
    if (it != m_uiStringMap[curIdx].end()) {
        return it->second;
    }

    // Fallback to English
    const auto itEn = m_uiStringMap[0].find(std::string(key));
    if (itEn != m_uiStringMap[0].end()) {
        return itEn->second;
    }

    return std::wstring(key.begin(), key.end());
}

std::wstring Localization::Format(std::string_view key, const std::vector<std::wstring>& args) const {
    std::wstring result = Get(key);
    for (size_t i = 0; i < args.size(); ++i) {
        std::wstring placeholder = L"{" + std::to_wstring(i) + L"}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::wstring::npos) {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    return result;
}

std::wstring Localization::GetCategory(std::wstring_view englishCat) const {
    const int curIdx = static_cast<int>(m_currentLang);
    const auto it = m_categoryMap[curIdx].find(std::wstring(englishCat));
    if (it != m_categoryMap[curIdx].end()) {
        return it->second;
    }
    return std::wstring(englishCat);
}

std::wstring Localization::GetTweakTitle(std::string_view id, std::wstring_view defaultTitle) const {
    if (m_currentLang == Language::German) {
        const auto it = m_tweakTitlesDe.find(std::string(id));
        if (it != m_tweakTitlesDe.end() && !it->second.empty()) {
            return it->second;
        }
    } else if (m_currentLang == Language::Russian) {
        const auto it = m_tweakTitlesRu.find(std::string(id));
        if (it != m_tweakTitlesRu.end() && !it->second.empty()) {
            return it->second;
        }
    }
    return std::wstring(defaultTitle);
}

void Localization::SavePreference() {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PrivatizeWin", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        const DWORD dwVal = static_cast<DWORD>(m_currentLang);
        RegSetValueExW(hKey, L"Language", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwVal), sizeof(dwVal));
        RegCloseKey(hKey);
    }
}

void Localization::LoadPreference() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PrivatizeWin", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwVal = 0;
        DWORD dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Language", nullptr, nullptr, reinterpret_cast<LPBYTE>(&dwVal), &dwSize) == ERROR_SUCCESS) {
            if (dwVal < static_cast<DWORD>(Language::Count)) {
                m_currentLang = static_cast<Language>(dwVal);
                RegCloseKey(hKey);
                return;
            }
        }
        RegCloseKey(hKey);
    }

    // Auto-detect system language
    m_currentLang = DetectSystemLanguage();
}

} // namespace PrivatizeWin
