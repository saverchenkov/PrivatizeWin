#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <windows.h>

namespace PrivatizeWin {

enum class Language : int {
    English = 0,
    German = 1,
    Russian = 2,
    French = 3,
    Spanish = 4,
    Italian = 5,
    Portuguese = 6,
    ChineseSimplified = 7,
    Japanese = 8,
    Polish = 9,
    Turkish = 10,
    Ukrainian = 11,
    Count = 12
};

struct LanguageInfo {
    Language lang;
    std::string code;         // "en", "de", "ru", ...
    std::wstring nativeName;  // L"English", L"Deutsch", L"Русский", ...
    std::wstring englishName; // L"English", L"German", L"Russian", ...
    UINT menuId;              // e.g. IDM_LANG_EN
};

class Localization {
public:
    static Localization& Instance();

    void Initialize();
    [[nodiscard]] Language GetCurrentLanguage() const noexcept { return m_currentLang; }
    void SetLanguage(Language lang);
    void SetLanguageByCode(std::string_view code);

    [[nodiscard]] const std::vector<LanguageInfo>& GetSupportedLanguages() const noexcept { return m_languages; }
    [[nodiscard]] const LanguageInfo& GetCurrentLanguageInfo() const;

    // Translation getters
    [[nodiscard]] std::wstring Get(std::string_view key) const;
    [[nodiscard]] std::wstring Format(std::string_view key, const std::vector<std::wstring>& args) const;
    [[nodiscard]] std::wstring GetCategory(std::wstring_view englishCat) const;
    [[nodiscard]] std::wstring GetTweakTitle(std::string_view id, std::wstring_view defaultTitle) const;

    void SavePreference();
    void LoadPreference();

private:
    Localization();
    void BuildLookupTables();
    static Language DetectSystemLanguage();

    Language m_currentLang{ Language::English };
    std::vector<LanguageInfo> m_languages;

    std::vector<std::unordered_map<std::string, std::wstring>> m_uiStringMap;
    std::vector<std::unordered_map<std::wstring, std::wstring>> m_categoryMap;

    std::unordered_map<std::string, std::wstring> m_tweakTitlesDe;
    std::unordered_map<std::string, std::wstring> m_tweakTitlesRu;
};

inline std::wstring Loc(std::string_view key) {
    return Localization::Instance().Get(key);
}

inline std::wstring LocFmt(std::string_view key, const std::vector<std::wstring>& args) {
    return Localization::Instance().Format(key, args);
}

template <typename... Args>
inline std::wstring LocFmt(std::string_view key, Args&&... args) {
    std::vector<std::wstring> strArgs;
    auto toWStr = [](auto&& val) -> std::wstring {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::wstring>) {
            return val;
        } else if constexpr (std::is_same_v<T, const wchar_t*> || std::is_same_v<T, wchar_t*>) {
            return std::wstring(val);
        } else if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) {
            return std::wstring(val.begin(), val.end());
        } else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
            std::string s(val);
            return std::wstring(s.begin(), s.end());
        } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
            return std::to_wstring(val);
        } else {
            return L"";
        }
    };
    (strArgs.push_back(toWStr(std::forward<Args>(args))), ...);
    return Localization::Instance().Format(key, strArgs);
}

} // namespace PrivatizeWin
