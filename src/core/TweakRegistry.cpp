#include "TweakRegistry.h"
#include "RegistryHelper.h"
#include "ServiceHelper.h"
#include "UserHiveManager.h"
#include "SimpleJson.h"
#include <algorithm>

namespace PrivatizeWin {

TweakRegistry& TweakRegistry::Instance() {
    static TweakRegistry registry;
    return registry;
}

void TweakRegistry::AddTweak(Tweak tweak) {
    m_idIndexMap[tweak.id] = m_tweaks.size();
    m_tweaks.push_back(std::move(tweak));
}

const Tweak* TweakRegistry::GetTweakById(std::string_view id) const {
    auto it = m_idIndexMap.find(std::string(id));
    if (it != m_idIndexMap.end()) {
        return &m_tweaks[it->second];
    }
    return nullptr;
}

std::vector<Tweak> TweakRegistry::GetTweaksByCategory(std::wstring_view category) const {
    std::vector<Tweak> result;
    for (const auto& t : m_tweaks) {
        if (category == L"All Settings" || t.category == category) {
            result.push_back(t);
        }
    }
    return result;
}

std::vector<CategoryInfo> TweakRegistry::GetCategories() const {
    std::map<std::wstring, int> counts;
    for (const auto& t : m_tweaks) {
        counts[t.category]++;
    }

    std::vector<CategoryInfo> cats;
    CategoryInfo all;
    all.name = L"All Settings";
    all.totalCount = static_cast<int>(m_tweaks.size());
    cats.push_back(all);

    for (const auto& [name, count] : counts) {
        CategoryInfo ci;
        ci.name = name;
        ci.totalCount = count;
        cats.push_back(ci);
    }
    return cats;
}

void TweakRegistry::InitializeDefaultTweaks() {
    m_tweaks.clear();
    m_idIndexMap.clear();

    // ==========================================
    // 1. Telemetry & Diagnostics
    // ==========================================
    {
        Tweak t;
        t.id = "TEL_DIAGTRACK";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable Connected User Experiences and Telemetry Service";
        t.description = L"The DiagTrack service collects diagnostic events and transmits them continuously to Microsoft.";
        t.impact = L"Disabling stops background telemetry transmission. Completely safe.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Service;
        t.serviceActions.push_back({ L"DiagTrack", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_DMWAP";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable WAP Push Message Routing Service";
        t.description = L"Routes telemetry and diagnostic packets on behalf of Windows telemetry components.";
        t.impact = L"Stops auxiliary telemetry routing. Safe for all workstations.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Service;
        t.serviceActions.push_back({ L"dmwappushservice", 4, 3, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_DIAGDATA";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Set Telemetry Data Collection to Minimum / Security Level";
        t.description = L"Forces the Windows telemetry pipeline to level 0 (Security) or level 1 (Required/Basic).";
        t.impact = L"Prevents transmission of application usage history, memory dumps, and visited URLs.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", RegType::Dword, 0, 3, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_AIT";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable Application Impact Telemetry (AIT)";
        t.description = L"Collects telemetry data on application crashes, compatibility events, and usage metrics.";
        t.impact = L"Saves disk write cycles and prevents tracking of installed application behavior.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"CommercialDataOptIn", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_CEIP";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable Customer Experience Improvement Program (CEIP)";
        t.description = L"Windows CEIP gathers anonymous information on how users interact with Windows components.";
        t.impact = L"Stops scheduled CEIP uploads and background task telemetry.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\SQMClient\\Windows", L"CEIPEnable", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_FEEDBACK";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable Windows Feedback Notification Prompts";
        t.description = L"Prevents Windows from periodically displaying pop-ups asking for ratings and feedback.";
        t.impact = L"Eliminates feedback prompts. Setting is completely transparent to the user.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"DoNotShowFeedbackNotifications", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "TEL_CRASHDUMP";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Restrict Windows Crash Dump & Error Reporting Telemetry";
        t.description = L"Restricts Windows Error Reporting (WER) from automatically transmitting crash dumps to the cloud.";
        t.impact = L"Prevents memory dumps (which may contain sensitive personal data) from uploading to Microsoft.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting", L"Disabled", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 2. AI, Copilot & Recall (Windows 11)
    // ==========================================
    {
        Tweak t;
        t.id = "AI_COPILOT";
        t.category = L"AI & Copilot";
        t.title = L"Disable Windows Copilot AI Integration";
        t.description = L"Completely disables the Windows Copilot side panel, hotkeys, and AI background process.";
        t.impact = L"Copilot icon is removed from the taskbar and Win+C shortcut is deactivated.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Both;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot", L"TurnOffWindowsCopilot", RegType::Dword, 1, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot", L"TurnOffWindowsCopilot", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_RECALL";
        t.category = L"AI & Copilot";
        t.title = L"Disable Windows Recall Automated Screen Snapshots";
        t.description = L"Disables Microsoft Recall from recording screenshots of your desktop, apps, and browser windows.";
        t.impact = L"Stops local and cloud AI snapshot indexing. Highly recommended for privacy.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Both;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot", L"DisableAIDataAnalysis", RegType::Dword, 1, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Recall", L"EnableRecall", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_SEARCH_HIGHLIGHTS";
        t.category = L"AI & Copilot";
        t.title = L"Disable Search Highlights and AI Dynamic Web Content in Taskbar";
        t.description = L"Disables MSN news, trending AI topics, and promotional web content inside the search box.";
        t.impact = L"Restores clean, fast, local Windows file search in the taskbar.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"EnableDynamicContentInSearchBox", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 3. Cortana, Start & Search
    // ==========================================
    {
        Tweak t;
        t.id = "SRCH_BING_START";
        t.category = L"Cortana & Search";
        t.title = L"Disable Bing Web Search in Start Menu";
        t.description = L"Prevents keystrokes typed in the Windows Start Menu from being sent to Bing web servers.";
        t.impact = L"Search only queries local files, apps, and settings. Significantly speeds up Start search.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer", L"DisableSearchBoxSuggestions", RegType::Dword, 1, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"BingSearchEnabled", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SRCH_CORTANA";
        t.category = L"Cortana & Search";
        t.title = L"Disable Cortana Voice Assistant";
        t.description = L"Completely disables Cortana voice activation and background voice processing.";
        t.impact = L"Cortana voice services are disabled. Safe for all users.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortana", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SRCH_CLOUD";
        t.category = L"Cortana & Search";
        t.title = L"Disable Cloud Search Integration";
        t.description = L"Prevents Windows Search from querying Microsoft account or OneDrive cloud indices.";
        t.impact = L"Restricts search indexing to local computer storage only.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCloudSearch", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 4. Privacy, Tracking & Advertising
    // ==========================================
    {
        Tweak t;
        t.id = "PRIV_AD_ID";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Advertising ID for Relevant Ads";
        t.description = L"Disables the unique cross-app Advertising ID assigned to each user profile.";
        t.impact = L"Apps can no longer track user activity across different programs. Highly recommended.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Both;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AdvertisingInfo", L"DisabledByGroupPolicy", RegType::Dword, 1, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo", L"Enabled", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_TAILORED";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Tailored Experiences with Diagnostic Data";
        t.description = L"Prevents Microsoft from using diagnostic data to show personalized recommendations and tips.";
        t.impact = L"Eliminates personalized marketing recommendations in Windows.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent", L"DisableTailoredExperiencesWithDiagnosticData", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_KEYSTROKES";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Inking and Typing Keystroke Personalization";
        t.description = L"Stops Windows from recording handwriting and keystrokes to improve spelling prediction.";
        t.impact = L"Prevents keystroke samples from being transmitted to Microsoft cloud servers.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\InputPersonalization", L"RestrictImplicitInkCollection", RegType::Dword, 1, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\InputPersonalization", L"RestrictImplicitTextCollection", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_TIMELINE";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Activity History & Timeline Cloud Sync";
        t.description = L"Prevents Windows from tracking and uploading documents opened, websites browsed, and active tasks.";
        t.impact = L"Protects past activity history from syncing to Microsoft cloud accounts.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"EnableActivityFeed", RegType::Dword, 0, 1, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"UploadUserActivities", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 5. Office & Outlook Migration (ShutUp10 3.6 Parity)
    // ==========================================
    {
        Tweak t;
        t.id = "OUT_HIDE_TOGGLE";
        t.category = L"Office & Outlook";
        t.title = L"Hide the 'Try the new Outlook' Toggle in Outlook";
        t.description = L"Hides the promotional toggle in classic desktop Outlook that prompts users to switch to the web app.";
        t.impact = L"Keeps classic Outlook interface clean and prevents accidental switching.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Office\\16.0\\Outlook\\Options\\General", L"HideNewOutlookToggle", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "OUT_BLOCK_MIGRATE";
        t.category = L"Office & Outlook";
        t.title = L"Disable Automatic Migration to the New Outlook Web App";
        t.description = L"Prevents Microsoft Windows from automatically replacing the native Mail and Calendar apps with web Outlook.";
        t.impact = L"Protects user choice and preserves local desktop mail clients.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Office\\16.0\\Outlook\\Preferences", L"NewOutlookMigrationUserSetting", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "OFFICE_TELEMETRY";
        t.category = L"Office & Outlook";
        t.title = L"Disable Microsoft Office Diagnostic Data and Connected Experiences";
        t.description = L"Prevents Microsoft Office 365/2019/2021/2024 from transmitting diagnostic telemetry.";
        t.impact = L"Shuts down Office telemetry beacons. Standard Office document features remain unaffected.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\office\\16.0\\common\\privacy", L"disconnectedstate", RegType::Dword, 2, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 6. Microsoft Edge & Web Privacy
    // ==========================================
    {
        Tweak t;
        t.id = "EDGE_METRICS";
        t.category = L"Microsoft Edge";
        t.title = L"Disable Microsoft Edge Diagnostic & Metrics Reporting";
        t.description = L"Prevents Edge from sending crash metrics, usage patterns, and hardware configurations.";
        t.impact = L"Stops Edge background telemetry. Does not impact web browsing.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"MetricsReportingEnabled", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_SHOPPING";
        t.category = L"Microsoft Edge";
        t.title = L"Disable Edge Shopping Assistant and Price Tracking Telemetry";
        t.description = L"Disables the Edge built-in shopping assistant that tracks visited e-commerce URLs to display coupons.";
        t.impact = L"Keeps your shopping habits and visited store URLs strictly private.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeShoppingAssistantEnabled", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_SEARCH_SUGGEST";
        t.category = L"Microsoft Edge";
        t.title = L"Disable Edge Address Bar Keystroke Suggestions";
        t.description = L"Prevents every keystroke typed into the Edge address bar from being sent live to search servers.";
        t.impact = L"URLs only submit when Enter is pressed. Stops real-time URL reporting.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SearchSuggestEnabled", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 7. Windows Update & Bandwidth
    // ==========================================
    {
        Tweak t;
        t.id = "WU_DELIVERY_OPT";
        t.category = L"Windows Update";
        t.title = L"Disable Delivery Optimization (P2P Windows Update Uploads)";
        t.description = L"Prevents your PC from being used as a torrent-like peer to upload Windows updates to other internet PCs.";
        t.impact = L"Saves upstream internet bandwidth and prevents background P2P network connections.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization", L"DODownloadMode", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "WU_AUTO_REBOOT";
        t.category = L"Windows Update";
        t.title = L"Disable Automatic Reboot with Logged-On Users";
        t.description = L"Prevents Windows Update from forcibly restarting your PC when you have unsaved work open.";
        t.impact = L"Windows will notify you to reboot instead of forcibly killing your applications.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU", L"NoAutoRebootWithLoggedOnUsers", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // 8. Lock Screen, Shell & Ads
    // ==========================================
    {
        Tweak t;
        t.id = "LOCK_SPOTLIGHT_ADS";
        t.category = L"Lock Screen & Shell";
        t.title = L"Disable Windows Spotlight Lock Screen Advertisements";
        t.description = L"Disables Microsoft Spotlight promotional suggestions, ads, and interactive tips on the Lock Screen.";
        t.impact = L"Maintains a clean static or slideshow lock screen wallpaper.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent", L"DisableWindowsSpotlightFeatures", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SHELL_PROMOTED_APPS";
        t.category = L"Lock Screen & Shell";
        t.title = L"Disable Automatic Installation of Suggested Third-Party Apps";
        t.description = L"Prevents Windows from automatically downloading and pinning sponsored apps (e.g. Candy Crush, TikTok).";
        t.impact = L"Keeps your Start Menu free from uninvited third-party bloatware.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent", L"DisableWindowsConsumerFeatures", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
}

SettingStatus TweakRegistry::AuditTweak(std::string_view id, UserSelectionMode mode, const std::vector<std::wstring>& users) const {
    const Tweak* t = GetTweakById(id);
    if (!t) return SettingStatus::Default;

    // Check services
    for (const auto& sa : t->serviceActions) {
        if (ServiceHelper::AuditAction(sa) != SettingStatus::Protected) {
            return SettingStatus::Default;
        }
    }

    // Check registry actions
    for (const auto& ra : t->regActions) {
        if (ra.scope == TargetScope::Machine) {
            if (RegistryHelper::AuditAction(HKEY_LOCAL_MACHINE, ra) != SettingStatus::Protected) {
                return SettingStatus::Default;
            }
        } else if (ra.scope == TargetScope::User) {
            if (UserHiveManager::AuditUserAction(ra, mode, users) != SettingStatus::Protected) {
                return SettingStatus::Default;
            }
        }
    }

    return SettingStatus::Protected;
}

bool TweakRegistry::ApplyTweak(std::string_view id, bool enableProtection, UserSelectionMode mode, const std::vector<std::wstring>& users) {
    const Tweak* t = GetTweakById(id);
    if (!t) return false;

    bool allOk = true;

    // Apply service actions
    for (const auto& sa : t->serviceActions) {
        if (!ServiceHelper::ApplyAction(sa, enableProtection)) {
            allOk = false;
        }
    }

    // Apply registry actions
    for (const auto& ra : t->regActions) {
        if (ra.scope == TargetScope::Machine) {
            if (!RegistryHelper::ApplyAction(HKEY_LOCAL_MACHINE, ra, enableProtection)) {
                allOk = false;
            }
        } else if (ra.scope == TargetScope::User) {
            if (!UserHiveManager::ApplyUserAction(ra, enableProtection, mode, users)) {
                allOk = false;
            }
        }
    }

    return allOk;
}

bool TweakRegistry::LoadExternalTweaks(std::string_view jsonContent) {
    try {
        JsonValue root = JsonValue::parse(std::string(jsonContent));
        if (!root.isObject() || !root["tweaks"].isArray()) {
            return false;
        }

        for (const auto& item : root["tweaks"].arrayValue) {
            if (!item.isObject()) continue;
            Tweak t;
            t.id = item["id"].stringValue;
            if (t.id.empty()) continue;

            t.title = std::wstring(item["title"].stringValue.begin(), item["title"].stringValue.end());
            t.category = std::wstring(item["category"].stringValue.begin(), item["category"].stringValue.end());
            t.description = std::wstring(item["description"].stringValue.begin(), item["description"].stringValue.end());
            t.impact = std::wstring(item["impact"].stringValue.begin(), item["impact"].stringValue.end());

            std::string safetyStr = item["safety"].stringValue;
            if (safetyStr == "normal") t.safety = SafetyLevel::Normal;
            else if (safetyStr == "advanced") t.safety = SafetyLevel::Advanced;
            else t.safety = SafetyLevel::Safe;

            AddTweak(std::move(t));
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace PrivatizeWin
