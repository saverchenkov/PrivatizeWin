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
    tweak.impactLevel = tweak.safety;
    tweak.isRecommended = (tweak.impactLevel == ImpactLevel::Low);
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

    // Core Telemetry Services
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
        t.impact = L"Prevents memory dumps from uploading to Microsoft.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting", L"Disabled", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "WU_AUTO_REBOOT";
        t.category = L"Windows Update";
        t.title = L"Disable Automatic Reboot with Logged-On Users";
        t.description = L"Prevents Windows from rebooting automatically while a user is currently logged on.";
        t.impact = L"Prevents unexpected work loss due to unattended restart cycles.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU", L"NoAutoRebootWithLoggedOnUsers", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: AI & Copilot
    // ==========================================
    {
        Tweak t;
        t.id = "AI_COPILOT_USER";
        t.title = L"Disable the Windows Copilot (User)";
        t.scope = TargetScope::User;
        t.category = L"AI & Copilot";
        t.description = L"The Windows Copilot is based on ChatGPT from OpenAI and is an extension to the AI in the Microsoft search engine Bing. In order for this AI to provide answers, further system information is transmitted in addition to the user queries. To prevent this, Copilot can be disabled.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot", L"TurnOffWindowsCopilot", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_COPILOT_MACHINE";
        t.title = L"Disable the Windows Copilot (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"AI & Copilot";
        t.description = L"The Windows Copilot is based on ChatGPT from OpenAI and is an extension to the AI in the Microsoft search engine Bing. In order for this AI to provide answers, further system information is transmitted in addition to the user queries. To prevent this, Copilot can be disabled.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot", L"TurnOffWindowsCopilot", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_RECALL";
        t.category = L"AI & Copilot";
        t.title = L"Disable the provision of Recall functionality to all users";
        t.description = L"This setting disables the Recall component for all users on the system. If it was previously enabled, all saved snapshots will be removed when the computer is restarted.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"AllowRecallEnablement", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C205";
        t.category = L"AI & Copilot";
        t.title = L"Disable the Image Creator in Microsoft Paint";
        t.description = L"The Image Creator in Microsoft Paint can create images with the help of artificial intelligence. To do this, appropriate information must be transferred to Microsoft servers. This setting disables this functionality.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Paint", L"DisableImageCreator", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C102";
        t.category = L"AI & Copilot";
        t.title = L"Disable the Copilot button from the taskbar";
        t.description = L"Removes the Copilot icon from the taskbar so that the search using AI (artificial intelligence) is no longer available. This setting can be used to disable this option.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowCopilotButton", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_SEARCH_HIGHLIGHTS";
        t.category = L"AI & Copilot";
        t.title = L"Disable Bing Chat eligibility in Windows Copilot";
        t.description = L"Controls whether the user is eligible to use Bing Chat features within the Windows Copilot. When disabled, this prevents access to Bing Chat functionality in Copilot, even if the feature would otherwise be available in the user's region and Windows build.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\Shell\\Copilot\\BingChat", L"IsUserEligible", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_RECALL_DATA_USER";
        t.title = L"Disable Windows Copilot+ Recall (User)";
        t.scope = TargetScope::User;
        t.category = L"AI & Copilot";
        t.description = L"This setting deactivates the new Windows Copilot+ Recall feature. This is a component that constantly creates screenshots, evaluates their content and makes the data available via an application. Both to the user himself and to other applications that have the corresponding authorizations. Disabling the Recall feature is strongly recommended.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"DisableAIDataAnalysis", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_RECALL_DATA_MACHINE";
        t.title = L"Disable Windows Copilot+ Recall (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"AI & Copilot";
        t.description = L"This setting deactivates the new Windows Copilot+ Recall feature. This is a component that constantly creates screenshots, evaluates their content and makes the data available via an application. Both to the user himself and to other applications that have the corresponding authorizations. Disabling the Recall feature is strongly recommended.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"DisableAIDataAnalysis", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C206";
        t.category = L"AI & Copilot";
        t.title = L"Disable Cocreator in Microsoft Paint";
        t.description = L"The cocreator in Microsoft Paint can create images with the help of artificial intelligence. These are created locally with the help of special hardware that must be available (so-called Neural Processing Unit or NPU for short). This setting disables this functionality.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Paint", L"DisableCocreator", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C207";
        t.category = L"AI & Copilot";
        t.title = L"Disable AI-powered image fill in Microsoft Paint";
        t.description = L"The AI-powered image filling in Microsoft Paint can complement images with the help of artificial intelligence and integrate new objects into existing images. These are created locally with the help of special hardware that must be available (so-called Neural Processing Unit or NPU for short). This setting disables this functionality.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Paint", L"DisableGenerativeFill", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C208";
        t.category = L"AI & Copilot";
        t.title = L"Disable Click to Do";
        t.description = L"This setting disables Click to Do on Copilot+ PCs. Click to Do can analyze screen content when invoked to suggest actions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"DisableClickToDo", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C209";
        t.category = L"AI & Copilot";
        t.title = L"Disable the Settings agent";
        t.description = L"This setting disables the AI agent in the Windows Settings app that can search for and change settings using an on-device model.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"DisableSettingsAgent", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C210";
        t.category = L"AI & Copilot";
        t.title = L"Disable AI features in Notepad";
        t.description = L"This setting disables Notepad AI features such as Rewrite and related text generation features that may send document text to an online service.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\WindowsNotepad", L"DisableAIFeatures", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C211";
        t.category = L"AI & Copilot";
        t.title = L"Disable AI actions in File Explorer";
        t.description = L"This setting removes the AI actions from the File Explorer context menu, so that AI actions such as image editing or document summarization are no longer offered when you right-click a file.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer", L"HideAIActionsMenu", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Activity History & Clipboard
    // ==========================================
    {
        Tweak t;
        t.id = "PRIV_TIMELINE";
        t.category = L"Activity History & Clipboard";
        t.title = L"Disable recordings of user activity";
        t.description = L"Windows records user activity such as surfing on the Internet and the use of applications in order to be able to create evaluations for the user locally and in the cloud (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"EnableActivityFeed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "A002";
        t.category = L"Activity History & Clipboard";
        t.title = L"Disable storing users' activity history";
        t.description = L"Windows stores user activities such as surfing on the Internet and the use of applications in order to be able to create evaluations for the user (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"PublishUserActivities", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "A003";
        t.category = L"Activity History & Clipboard";
        t.title = L"Disable the submission of user activities to Microsoft";
        t.description = L"Windows sends user activities such as surfing the Internet and the use of applications to Microsoft in order to be able to create evaluations for the user in the cloud (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"UploadUserActivities", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: App Permissions & Hardware Access
    // ==========================================
    {
        Tweak t;
        t.id = "P025_USER";
        t.title = L"Disable app access to device location (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to the location of your computer. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P025_MACHINE";
        t.title = L"Disable app access to device location (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to the location of your computer. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Start_TrackProgs", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P023_USER";
        t.title = L"Disable app access to diagnostics information (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to diagnostic information from your system. These are needed for finding sources of error from the respective manufacturers. You can disable this feature if you don’t wish to permit this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appDiagnostics", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P023_MACHINE";
        t.title = L"Disable app access to diagnostics information (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to diagnostic information from your system. These are needed for finding sources of error from the respective manufacturers. You can disable this feature if you don’t wish to permit this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{2297E4E2-5DBE-466D-A12B-0F8286F0D9CA}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appDiagnostics", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P082_USER";
        t.title = L"Deny app access to generative AI (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"This setting denies apps access to Windows text and image generation capabilities. It writes both known capability names used by recent Windows builds.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy", L"LetAppsAccessGenerativeAI", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy", L"LetAppsAccessSystemAIModels", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\generativeAI", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\systemAIModels", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P082_MACHINE";
        t.title = L"Deny app access to generative AI (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"This setting denies apps access to Windows text and image generation capabilities. It writes both known capability names used by recent Windows builds.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\generativeAI", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\systemAIModels", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P084_USER";
        t.title = L"Deny app access to presence sensing (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"This setting denies apps access to human presence sensors used for features such as wake on approach and lock on leave.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy", L"LetAppsAccessHumanPresence", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\humanPresence", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P084_MACHINE";
        t.title = L"Deny app access to presence sensing (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"This setting denies apps access to human presence sensors used for features such as wake on approach and lock on leave.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\humanPresence", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P098";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable location override";
        t.description = L"If \"Allow location override\" is switched on, Windows, apps and services may use the location of a remote connection instead of the location of this device. The location that is passed on then comes from a source that you cannot see on the device itself. This setting switches the override off.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\Store\\UserLocationOverridePrivacySetting", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P012_USER";
        t.title = L"Disable app access to camera (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"By default, apps, e.g. the browser Edge, Facebook or Twitter can access the camera on your machine, if one exists. Activating camera access can be useful when you want, for example, to make video chats or conferences. Deactivating may result in not being able to transmit video images.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P012_MACHINE";
        t.title = L"Disable app access to camera (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"By default, apps, e.g. the browser Edge, Facebook or Twitter can access the camera on your machine, if one exists. Activating camera access can be useful when you want, for example, to make video chats or conferences. Deactivating may result in not being able to transmit video images.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{E5323777-F976-4f5b-9B55-B94699C46E44}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P013_USER";
        t.title = L"Disable app access to microphone (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If you want to use online chat apps or the voice recorder, this function should remain active. If you don't use voice recording or transfer, then deactivate the microphone access to prevent manipulated apps from activating the microphone and recording a conversation without your permission.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P013_MACHINE";
        t.title = L"Disable app access to microphone (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If you want to use online chat apps or the voice recorder, this function should remain active. If you don't use voice recording or transfer, then deactivate the microphone access to prevent manipulated apps from activating the microphone and recording a conversation without your permission.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{2EEF81BE-33FA-4800-9670-1CD474972C3F}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P062";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable app access to use voice activation";
        t.description = L"When this feature is disabled, apps can no longer be activated by voice commands. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps", L"AgentActivationEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P063";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable app access to use voice activation when device is locked";
        t.description = L"When this feature is disabled, apps can no longer be activated by voice commands when the device is locked. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps", L"AgentActivationOnLockScreenEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P081";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable the standard app for the headset button";
        t.description = L"A button may exist on a headset that restarts the last app when it is pressed. If you accidentally press this button and start an undesired app, this can be a privacy issue. This setting disables this. You can then connect a defined application to the button in the Windows Settings.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps", L"AgentActivationLastUsed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P019_USER";
        t.title = L"Disable app access to notifications (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your messages such as SMS or MMS. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userNotificationListener", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P019_MACHINE";
        t.title = L"Disable app access to notifications (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your messages such as SMS or MMS. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{52079E78-A92B-413F-B213-E8FE35712E72}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userNotificationListener", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P048_USER";
        t.title = L"Disable app access to movements (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your movements, which are recorded by so-called motion trackers. This can limit some apps in their function or stop working at all (e.g. fitness apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\activity", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P048_MACHINE";
        t.title = L"Disable app access to movements (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your movements, which are recorded by so-called motion trackers. This can limit some apps in their function or stop working at all (e.g. fitness apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\activity", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P020_USER";
        t.title = L"Disable app access to contacts (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your contacts. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\contacts", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P020_MACHINE";
        t.title = L"Disable app access to contacts (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your contacts. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{7D7E8402-7C54-4821-A34E-AEEFD62DED93}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\contacts", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P011_USER";
        t.title = L"Disable app access to calendar (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"By deactivating this function, apps will have no access to calendar entries. Some apps may be limited in their functionality or not work at all (e.g. calendar apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appointments", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P011_MACHINE";
        t.title = L"Disable app access to calendar (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"By deactivating this function, apps will have no access to calendar entries. Some apps may be limited in their functionality or not work at all (e.g. calendar apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{D89823BA-7180-4B81-B50C-7E471E6121A3}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appointments", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P050_USER";
        t.title = L"Disable app access to phone calls (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to phones connected to this device and will not be able to make calls. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCall", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P050_MACHINE";
        t.title = L"Disable app access to phone calls (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to phones connected to this device and will not be able to make calls. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCall", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P018_USER";
        t.title = L"Disable app access to call history (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your calling history. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCallHistory", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P018_MACHINE";
        t.title = L"Disable app access to call history (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your calling history. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{8BC668CF-7728-45BD-93F8-CF2B3B41D7AB}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCallHistory", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P021_USER";
        t.title = L"Disable app access to email (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your mails. As a result, some apps may be limited in their functionality or no longer function at all (e.g. mail apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\email", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P021_MACHINE";
        t.title = L"Disable app access to email (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your mails. As a result, some apps may be limited in their functionality or no longer function at all (e.g. mail apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{9231CB4C-BF57-4AF3-8C55-FDA7BFCC04C5}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\email", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "112_USER";
        t.title = L"Disable app access to tasks (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your task lists. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging or calendar apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{E390DF20-07DF-446D-B962-F5C953062741}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "112_MACHINE";
        t.title = L"Disable app access to tasks (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Disabling this function means apps will no longer have access to your task lists. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging or calendar apps).";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userDataTasks", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P014_USER";
        t.title = L"Disable app access to messages (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps are denied access to your notification / mails if these functions are deactivated (Emails, SMS, Messenger). This may lead to Messenger apps or email apps not working correctly anymore.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\chat", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P014_MACHINE";
        t.title = L"Disable app access to messages (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps are denied access to your notification / mails if these functions are deactivated (Emails, SMS, Messenger). This may lead to Messenger apps or email apps not working correctly anymore.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{992AFA70-6F47-4148-B3E9-3003349C1548}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\chat", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P052_USER";
        t.title = L"Disable app access to wireless connections (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\radios", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P052_MACHINE";
        t.title = L"Disable app access to wireless connections (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\radios", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P054";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable app access to loosely coupled devices";
        t.description = L"If this feature is disabled, apps must not establish wireless connections that were not previously authorized (e.g. beacons). This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\bluetoothSync", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P029_USER";
        t.title = L"Disable app access to documents (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your documents. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\documentsLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P029_MACHINE";
        t.title = L"Disable app access to documents (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your documents. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\documentsLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P030_USER";
        t.title = L"Disable app access to images (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your pictures and photos. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\picturesLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P030_MACHINE";
        t.title = L"Disable app access to images (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your pictures and photos. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\picturesLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P031_USER";
        t.title = L"Disable app access to videos (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your videos. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\videosLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P031_MACHINE";
        t.title = L"Disable app access to videos (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your videos. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\videosLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P032_USER";
        t.title = L"Disable app access to the file system (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your file system and therefore your files. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\broadFileSystemAccess", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P032_MACHINE";
        t.title = L"Disable app access to the file system (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer have access to your file system and therefore your files. Some apps may be restricted in your function or may no longer work at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\broadFileSystemAccess", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P058_USER";
        t.title = L"Disable app access to wireless technology (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps are no longer allowed to use the PC's wireless technology. This can limit some apps in their function or stop working if they need a data connection.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\cellularData", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P058_MACHINE";
        t.title = L"Disable app access to wireless technology (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps are no longer allowed to use the PC's wireless technology. This can limit some apps in their function or stop working if they need a data connection.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\cellularData", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P060_USER";
        t.title = L"Disable app access to eye tracking (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer be able to track the user's eyes and gaze in front of the device. This may limit some apps in their function or stop working at all. This may affect applications for users with neuromuscular diseases such as ALS, who can control the PC using this functionality.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\gazeInput", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P060_MACHINE";
        t.title = L"Disable app access to eye tracking (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"When this feature is disabled, apps will no longer be able to track the user's eyes and gaze in front of the device. This may limit some apps in their function or stop working at all. This may affect applications for users with neuromuscular diseases such as ALS, who can control the PC using this functionality.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\gazeInput", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P071_USER";
        t.title = L"Disable the ability for apps to take screenshots (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting you can prevent this.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureProgrammatic", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P071_MACHINE";
        t.title = L"Disable the ability for apps to take screenshots (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting you can prevent this.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureProgrammatic", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P073";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable the ability for desktop apps to take screenshots";
        t.description = L"Windows applications (= applications that you have not installed from the Microsoft Store) can take screenshots (screenshots) of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the Windows application. With this setting you can prevent this.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureProgrammatic\\NonPackaged", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P074_USER";
        t.title = L"Disable the ability for apps to take screenshots without borders (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureWithoutBorder", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P074_MACHINE";
        t.title = L"Disable the ability for apps to take screenshots without borders (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureWithoutBorder", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P076";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Disable the ability for desktop apps to take screenshots without margins";
        t.description = L"Windows applications (= applications that you have not installed from the Microsoft Store) can take screenshots (screenshots) of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the Windows application. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureWithoutBorder\\NonPackaged", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P077_USER";
        t.title = L"Disable app access to music libraries (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is turned off, apps will no longer be allowed to access music libraries. Some apps may be restricted in their function or stop working at all if they need access to music files.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\musicLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P077_MACHINE";
        t.title = L"Disable app access to music libraries (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is turned off, apps will no longer be allowed to access music libraries. Some apps may be restricted in their function or stop working at all if they need access to music files.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\musicLibrary", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P079_USER";
        t.title = L"Disable app access to downloads folder (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to access the downloads folder. Some apps may be restricted in their function or may stop working at all if they need access to this directory.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\downloadsFolder", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P079_MACHINE";
        t.title = L"Disable app access to downloads folder (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to access the downloads folder. Some apps may be restricted in their function or may stop working at all if they need access to this directory.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\downloadsFolder", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P024";
        t.category = L"App Permissions & Hardware Access";
        t.title = L"Prohibit apps from running in the background";
        t.description = L"If this feature is disabled, apps will no longer be able to run in the background. This means that they are always terminated immediately and can no longer process or send messages in the background. As a result, notifications are no longer displayed on the desktop and in the Action Center. Only the number of messages is still shown. If you want to prevent this, deactivate this function. This also saves electricity, which can be relevant for mobile devices.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications", L"GlobalUserDisabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P086_USER";
        t.title = L"Disable app access to passkeys (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps and websites will no longer be allowed to access the passkeys stored on this device. Signing in with a passkey will then no longer be possible and you will have to use your password or another sign-in method instead.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\passkeys", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P086_MACHINE";
        t.title = L"Disable app access to passkeys (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps and websites will no longer be allowed to access the passkeys stored on this device. Signing in with a passkey will then no longer be possible and you will have to use your password or another sign-in method instead.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\passkeys", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P089_USER";
        t.title = L"Disable app access to the list of stored passkeys (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps and websites will no longer be allowed to determine which passkeys are stored on this device. Websites may then no longer offer you a passkey sign-in even though a passkey exists.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\passkeysEnumeration", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P089_MACHINE";
        t.title = L"Disable app access to the list of stored passkeys (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps and websites will no longer be allowed to determine which passkeys are stored on this device. Websites may then no longer offer you a passkey sign-in even though a passkey exists.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\passkeysEnumeration", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P087_USER";
        t.title = L"Disable app access to Bluetooth devices (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to communicate with Bluetooth devices. Apps that rely on Bluetooth accessories such as headphones, controllers or fitness trackers may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\bluetooth", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P087_MACHINE";
        t.title = L"Disable app access to Bluetooth devices (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to communicate with Bluetooth devices. Apps that rely on Bluetooth accessories such as headphones, controllers or fitness trackers may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\bluetooth", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P088_USER";
        t.title = L"Disable app access to human interface devices (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to access human interface devices (HID). Apps that use game controllers or other special input hardware may stop working. Your keyboard and mouse are not affected: Windows reserves these devices for the system, so they remain available in any case.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\humanInterfaceDevice", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P088_MACHINE";
        t.title = L"Disable app access to human interface devices (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to access human interface devices (HID). Apps that use game controllers or other special input hardware may stop working. Your keyboard and mouse are not affected: Windows reserves these devices for the system, so they remain available in any case.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\humanInterfaceDevice", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P090_USER";
        t.title = L"Disable app access to custom sensors (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to read custom sensors built into your device. Apps that evaluate sensor data may be restricted in their function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\sensors.custom", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P090_MACHINE";
        t.title = L"Disable app access to custom sensors (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to read custom sensors built into your device. Apps that evaluate sensor data may be restricted in their function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\sensors.custom", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P091_USER";
        t.title = L"Disable app access to serial ports (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to use serial ports. Apps that communicate with measuring instruments, microcontrollers or other serial hardware may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\serialCommunication", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P091_MACHINE";
        t.title = L"Disable app access to serial ports (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to use serial ports. Apps that communicate with measuring instruments, microcontrollers or other serial hardware may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\serialCommunication", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P092_USER";
        t.title = L"Disable app access to USB devices (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to communicate directly with USB devices. Apps that use USB hardware such as printers, scanners or programming adapters may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\usb", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P092_MACHINE";
        t.title = L"Disable app access to USB devices (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to communicate directly with USB devices. Apps that use USB hardware such as printers, scanners or programming adapters may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\usb", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P093_USER";
        t.title = L"Disable app access to Wi-Fi information (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to read information about nearby Wi-Fi networks. This data can be used to determine your location, but some apps may be restricted in their function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\wifiData", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P093_MACHINE";
        t.title = L"Disable app access to Wi-Fi information (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to read information about nearby Wi-Fi networks. This data can be used to determine your location, but some apps may be restricted in their function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\wifiData", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P094_USER";
        t.title = L"Disable app access to Wi-Fi Direct (User)";
        t.scope = TargetScope::User;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to establish direct Wi-Fi connections to other devices. Features such as wireless displays or direct file transfer may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\wiFiDirect", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P094_MACHINE";
        t.title = L"Disable app access to Wi-Fi Direct (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"App Permissions & Hardware Access";
        t.description = L"If this feature is disabled, apps will no longer be allowed to establish direct Wi-Fi connections to other devices. Features such as wireless displays or direct file transfer may stop working.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\wiFiDirect", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Cortana & Search
    // ==========================================
    {
        Tweak t;
        t.id = "C001_USER";
        t.title = L"Disable and reset Cortana (User)";
        t.scope = TargetScope::User;
        t.category = L"Cortana & Search";
        t.description = L"If you don't want the personal assistant Cortana, then deactivate this function. This prevents Microsoft receiving information like contacts, current calendar events, language patterns, handwriting samples and your entry history.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortana", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C001_MACHINE";
        t.title = L"Disable and reset Cortana (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Cortana & Search";
        t.description = L"If you don't want the personal assistant Cortana, then deactivate this function. This prevents Microsoft receiving information like contacts, current calendar events, language patterns, handwriting samples and your entry history.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search", L"CortanaConsent", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C002";
        t.category = L"Cortana & Search";
        t.title = L"Disable Input Personalization";
        t.description = L"In getting to know the user, \"Microsoft collects information on voice, handwriting and keyboard entries, including information in your calendar and your contacts. With the help of this information your device can recognize entries much better, e.g. your pronounciation and your handwriting.\" Deactivate this function, if you don't want to send personal data to Microsoft. This function is needed for Cortana.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Personalization\\Settings", L"AcceptedPrivacyPolicy", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\InputPersonalization", L"RestrictImplicitInkCollection", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\InputPersonalization", L"RestrictImplicitTextCollection", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore", L"HarvestContacts", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C013";
        t.category = L"Cortana & Search";
        t.title = L"Disable online speech recognition";
        t.description = L"This setting allows you to disable Windows online speech recognition. When enabled, you can \"talk\" to Cortana and other apps. The data will be transmitted to Microsoft to improve the service.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\InputPersonalization", L"AllowInputPersonalization", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C007";
        t.category = L"Cortana & Search";
        t.title = L"Cortana and search are disallowed to use location";
        t.description = L"Cortana uses your geographical location to present your search results accordingly. You have the option of disabling this feature if you don’t wish to indicate your location.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowSearchToUseLocation", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SRCH_BING_START";
        t.category = L"Cortana & Search";
        t.title = L"Disable web search from Windows Desktop Search";
        t.description = L"When starting a Windows desktop search, results from the web will also be presented. This setting will allow you to limit the results of your search to your computer only.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"DisableWebSearch", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C009";
        t.category = L"Cortana & Search";
        t.title = L"Disable display web results in Search";
        t.description = L"Cortana can search throughout the web for you. You can easily disable this feature by using this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"ConnectedSearchUseWeb", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "C010";
        t.category = L"Cortana & Search";
        t.title = L"Disable download and updates of speech recognition and speech synthesis models";
        t.description = L"If you don't wish to use Cortana, this option will also allow you to disable the Cortana module from refreshing and providing downloads.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Speech_OneCore\\Preferences", L"ModelDownloadAllowed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SRCH_CLOUD";
        t.category = L"Cortana & Search";
        t.title = L"Disable cloud search";
        t.description = L"Using Cortana for searches will also involve affiliated cloud sources such as OneDrive or SharePoint. This will result in your local searches being transferred and carried out on Microsoft Servers. Disabling searches in the cloud will prevent this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCloudSearch", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SRCH_CORTANA";
        t.category = L"Cortana & Search";
        t.title = L"Disable Cortana above lock screen";
        t.description = L"Cortona can already respond to voice input in the lock screen. This means that you can also \"address\" a locked device. This can lead to accidental interactions. This setting will prevent this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortanaAboveLock", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "AI_SEARCH_HIGHLIGHTS_WSB";
        t.category = L"Cortana & Search";
        t.title = L"Disable the search highlights in the taskbar";
        t.description = L"So-called search highlights are displayed in the taskbar, which refer to current events and search trends. This can not only make the taskbar more confusing visually, but also transmit information to Microsoft. This can be disabled with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"EnableDynamicContentInWSB", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M025";
        t.category = L"Cortana & Search";
        t.title = L"Disable search with AI in search box";
        t.description = L"In the search field of the taskbar, a Bing icon is displayed that enables searching by means of AI (artificial intelligence). For this, a separate browser window is opened and you can enter a natural language question there. Personal information is transmitted in the process. This AI option can be disabled with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings", L"IsDynamicSearchBoxEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M003_USER";
        t.title = L"Disable extension of Windows search with Bing (User)";
        t.scope = TargetScope::User;
        t.category = L"Cortana & Search";
        t.description = L"If you want to look for a local App or a setting in the Windows search, but don't type in the exact name, Windows will look for an answer by default using Bing, instead of using your local hard disk for adequate results. This will deactivate that function.  <u>Note:</u> Windows Search and Explorer may sporadically reset this setting during search indexer restarts, feature updates, or Group Policy refresh cycles.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer", L"DisableSearchBoxSuggestions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M003_MACHINE";
        t.title = L"Disable extension of Windows search with Bing (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Cortana & Search";
        t.description = L"If you want to look for a local App or a setting in the Windows search, but don't type in the exact name, Windows will look for an answer by default using Bing, instead of using your local hard disk for adequate results. This will deactivate that function.  <u>Note:</u> Windows Search and Explorer may sporadically reset this setting during search indexer restarts, feature updates, or Group Policy refresh cycles.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"BingSearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer", L"DisableSearchBoxSuggestions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M029";
        t.category = L"Cortana & Search";
        t.title = L"Disable Microsoft account cloud content search";
        t.description = L"This setting disables Microsoft account cloud content results in Windows Search for the current user.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings", L"IsMSACloudSearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M030";
        t.category = L"Cortana & Search";
        t.title = L"Disable work or school cloud content search";
        t.description = L"This setting disables work or school account cloud content results in Windows Search for the current user.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings", L"IsAADCloudSearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M031";
        t.category = L"Cortana & Search";
        t.title = L"Disable device search history";
        t.description = L"This setting disables search history stored on the device for the current user.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings", L"IsDeviceSearchHistoryEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Gaming & Xbox
    // ==========================================
    {
        Tweak t;
        t.id = "G001";
        t.category = L"Gaming & Xbox";
        t.title = L"Disable Xbox Game Bar and Game DVR";
        t.description = L"The Xbox Game Bar and Game DVR allow recording and screenshots during gameplay. However, even when the Game Bar is disabled, Windows may still trigger the \"ms-gamingoverlay\" protocol, causing the Microsoft Store to open and search for a handler app. Enabling this setting fully disables the Game Bar, Game DVR, and app capture functionality, preventing unwanted Store prompts.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR", L"AllowGameDVR", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Location & Sensors
    // ==========================================
    {
        Tweak t;
        t.id = "L001";
        t.category = L"Location & Sensors";
        t.title = L"Disable functionality to locate the system";
        t.description = L"Locations services is used so that apps or websites can show you results based on your location, e.g. directions or restaurants in your neighborhood.   <u>Note:</u> Enabling this setting may disable the Windows 11 Night Light feature, which relies on the location service to determine sunrise and sunset times for automatic scheduling.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors", L"DisableLocation", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors", L"DisableWindowsLocationProvider", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L003";
        t.category = L"Location & Sensors";
        t.title = L"Disable scripting functionality to locate the system";
        t.description = L"Locations services is used so that apps or websites can show you results based on your location, e.g. directions or restaurants in your neighborhood.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors", L"DisableLocationScripting", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L004";
        t.category = L"Location & Sensors";
        t.title = L"Disable sensors for locating the system and its orientation";
        t.description = L"Assuming they are available, GPS receivers and gyroscope sensors will be deactivated. For Tablet PCs, this could mean that screen rotation will no longer be recognized. It should not be activated if this function is required.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors", L"DisableSensors", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L005_USER";
        t.title = L"Disable Windows Geolocation Service (User)";
        t.scope = TargetScope::User;
        t.category = L"Location & Sensors";
        t.description = L"The geolocation service in Windows manages the current location of the system and defines geographical boundaries (so-called “geofencing“). Deactivating it means applications can no longer access the geographical location through this service.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Sensor\\Overrides\\{BFA794E4-F964-4FDB-90F6-51056BFE4B44}", L"SensorPermissionState", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L005_MACHINE";
        t.title = L"Disable Windows Geolocation Service (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Location & Sensors";
        t.description = L"The geolocation service in Windows manages the current location of the system and defines geographical boundaries (so-called “geofencing“). Deactivating it means applications can no longer access the geographical location through this service.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\lfsvc", L"Start", RegType::Dword, 0, 1, L"", L"", false });
        t.serviceActions.push_back({ L"lfsvc", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L008";
        t.category = L"Location & Sensors";
        t.title = L"Disable Find My Device";
        t.description = L"This setting disables Find My Device so Windows does not periodically send the device location to the associated Microsoft account.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\FindMyDevice", L"AllowFindMyDevice", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "L007";
        t.category = L"Location & Sensors";
        t.title = L"Disable app access to your location";
        t.description = L"This function allows apps to access your location. Some apps require this in order to deliver their content in your language or deliver content based on your geographical location. Deactivating this function can mean that some apps display content in the wrong language or deliver the wrong geographical content, and in the worst case may render some apps unusable.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{BFA794E4-F964-4FDB-90F6-51056BFE4B44}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Sensor\\Permissions\\{BFA794E4-F964-4FDB-90F6-51056BFE4B44}", L"Deny", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Lock Screen & Desktop
    // ==========================================
    {
        Tweak t;
        t.id = "LOCK_SPOTLIGHT_ADS";
        t.category = L"Lock Screen & Desktop";
        t.title = L"Disable Windows Spotlight";
        t.description = L"Windows Spotlight provides (daily) changing pictures on your lock screen. These are taken from Microsoft Bing. You can rate these pictures. When doing so, information will be sent to Microsoft that can clearly identify you personally as well as your computer. Turning off this function is recommended. Note: Windows Spotlight also stops working if “Disable fun facts, tips, tricks, and more on your lock screen” is enabled.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"RotatingLockScreenEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "K002";
        t.category = L"Lock Screen & Desktop";
        t.title = L"Disable fun facts, tips, tricks, and more on your lock screen";
        t.description = L"Along with tips and tricks for using Windows, the lock screen also fades in advertisements and additional information. These will send a lot of information onto Microsoft that can be used to identify your computer as well as you personally. That’s why this setting should be disabled. Note: Windows Spotlight on the lock screen depends on this content. If this setting is enabled, Windows Spotlight stops working and Windows switches the lock screen to a picture. Leave this setting disabled if you want to keep using Windows Spotlight.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"RotatingLockScreenOverlayEnabled", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-338387Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "K005";
        t.category = L"Lock Screen & Desktop";
        t.title = L"Disable notifications on lock screen";
        t.description = L"Notifications from apps can be displayed on the lock screen. These might contain private information that others could read without having to be logged onto the computer. Setting this setting disables showing notifications.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings", L"NOC_GLOBAL_SETTING_ALLOW_TOASTS_ABOVE_LOCK", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Microsoft Edge
    // ==========================================
    {
        Tweak t;
        t.id = "E001_USER";
        t.title = L"Disable tracking in the web (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Disabling tracking (\"Do Not Track\" or also DNT) means that Edge will send a message to the website indicating that no tracking of the user should take place. This means that the IP and cookies will not be saved. Websites are not obliged to honor this request but this is generally the case. This is why it makes sense to enable this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ConfigureDoNotTrack", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E001_MACHINE";
        t.title = L"Disable tracking in the web (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Disabling tracking (\"Do Not Track\" or also DNT) means that Edge will send a message to the website indicating that no tracking of the user should take place. This means that the IP and cookies will not be saved. Websites are not obliged to honor this request but this is generally the case. This is why it makes sense to enable this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Main", L"DoNotTrack", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ConfigureDoNotTrack", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E002";
        t.category = L"Microsoft Edge";
        t.title = L"Disable page prediction";
        t.description = L"When using page prediction, pages linked to the page you're visiting will be automatically loaded in the background. This is supposed to make surfing faster but it also transmits information onto sites that you may never want to visit. That's why we recommend disabling this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\FlipAhead", L"FPEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E003_USER";
        t.title = L"Disable search and website suggestions (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"While entering searches or URIs in Edge, matching suggestions will appear automatically. This is done by transmitting the input to Microsoft where it must be evaluated in order to create these suggestions. Such a transfer of data enables conclusions to be made regarding surfing behavior.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SearchSuggestEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E003_MACHINE";
        t.title = L"Disable search and website suggestions (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"While entering searches or URIs in Edge, matching suggestions will appear automatically. This is done by transmitting the input to Microsoft where it must be evaluated in order to create these suggestions. Such a transfer of data enables conclusions to be made regarding surfing behavior.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Main", L"ShowSearchSuggestionsGlobal", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SearchSuggestEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E008";
        t.category = L"Microsoft Edge";
        t.title = L"Disable Cortana in Microsoft Edge";
        t.description = L"You can set this option to disable the cloud-based \"Cortana\" assistant in Microsoft Edge. When you do this, Cortana will no longer support you in the Edge browser nor send any data to the cloud.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\ServiceUI", L"EnableCortana", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E007_USER";
        t.title = L"Disable automatic completion of web addresses in address bar (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Automatic completion in the address lines of Edge can possibly transmit data concerning user behavior. As a result, information regarding your surfing activity might also be available to others who use this computer under your user account. By disabling this feature, no suggestions will be made for completing any addresses while entering a web address.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Browser", L"AllowAddressBarDropdown", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AddressBarMicrosoftSearchInBingProviderEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E007_MACHINE";
        t.title = L"Disable automatic completion of web addresses in address bar (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Automatic completion in the address lines of Edge can possibly transmit data concerning user behavior. As a result, information regarding your surfing activity might also be available to others who use this computer under your user account. By disabling this feature, no suggestions will be made for completing any addresses while entering a web address.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AddressBarMicrosoftSearchInBingProviderEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E010";
        t.category = L"Microsoft Edge";
        t.title = L"Disable showing search history";
        t.description = L"Use these settings to disable showing search history. This feature is usually helpful but such information can also be shown to other users, especially when sharing a PC.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\ServiceUI\\ShowSearchHistory", L"", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E011_USER";
        t.title = L"Disable user feedback in toolbar (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Edge (Chromium) displays a smiley on the toolbar for sending feedback to Microsoft. This setting allows you to hide the smiley. After changing the setting, the browser must be restarted for it to take effect.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"UserFeedbackAllowed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E011_MACHINE";
        t.title = L"Disable user feedback in toolbar (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Edge (Chromium) displays a smiley on the toolbar for sending feedback to Microsoft. This setting allows you to hide the smiley. After changing the setting, the browser must be restarted for it to take effect.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"UserFeedbackAllowed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E012_USER";
        t.title = L"Disable storing and autocompleting of credit card data on websites (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can automatically store credit card information and fill it out on later purchases. To do this, the data must be stored reversibly on the local machine, so this poses a potential security risk.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AutofillCreditCardEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E012_MACHINE";
        t.title = L"Disable storing and autocompleting of credit card data on websites (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can automatically store credit card information and fill it out on later purchases. To do this, the data must be stored reversibly on the local machine, so this poses a potential security risk.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AutofillCreditCardEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E009_USER";
        t.title = L"Disable form suggestions (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can suggest previous entries for easier completion when filling out forms. This is usually helpful but such information can also be displayed to other users, especially when sharing a PC.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AutofillAddressEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E009_MACHINE";
        t.title = L"Disable form suggestions (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can suggest previous entries for easier completion when filling out forms. This is usually helpful but such information can also be displayed to other users, especially when sharing a PC.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Main", L"Use FormSuggest", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AutofillAddressEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E004";
        t.category = L"Microsoft Edge";
        t.title = L"Disable sites saving protected media licenses on my device";
        t.description = L"It is sometimes necessary to save information for protected music or video content (so-called Digital Rights Management = DRM) in order to play them on a device. This also requires a specific ID to identify the machine. Enable this setting if you want to stop this from happening.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Privacy", L"EnableEncryptedMediaExtensions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E005";
        t.category = L"Microsoft Edge";
        t.title = L"Do not optimize web search results on the task bar for screen reader";
        t.description = L"When using the task bar to do a web search, it's possible to make settings that define whether the results should be displayed (non-optimized) in Edge or (optimized) in Internet Explorer. The latter is compatible with the screen reader for Windows and allows visually-impaired users to read websites. If you're not intending to use this feature, we recommend disabling it to avoid having to use Internet Explorer as well. Depending on the needs of visually-impaired users, this setting is only somewhat recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Main", L"OptimizeWindowsSearchResultsForScreenReaders", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E013";
        t.category = L"Microsoft Edge";
        t.title = L"Disable Microsoft Edge launch in the background";
        t.description = L"Microsoft Edge can start in the background to improve performance when the system is idle. This happens when Windows starts and whenever Edge is closed. Disabling this may reduce Edge performance while making the system itself faster.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\Main", L"AllowPrelaunch", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E014";
        t.category = L"Microsoft Edge";
        t.title = L"Disable loading the start and new tab pages in the background";
        t.description = L"Microsoft Edge can preload the Start and New Tab pages in the background to improve performance when the system is idle. This happens when Windows starts and whenever Edge is closed. Disabling this may reduce Edge performance while making the system itself faster.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\TabPreloader", L"AllowTabPreloading", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E006_USER";
        t.title = L"Disable SmartScreen Filter (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"The SmartScreen Filter protects you from accessing malicious websites and downloads whenever you're surfing with Edge. In order to do this, information (e.g., the URL) will be sent to Microsoft that allows it to identify such dangerous content. Disabling this function provides more privacy but it also gives you less protection while surfing. That's why we recommend your leaving this function enabled.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SmartScreenEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E006_MACHINE";
        t.title = L"Disable SmartScreen Filter (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"The SmartScreen Filter protects you from accessing malicious websites and downloads whenever you're surfing with Edge. In order to do this, information (e.g., the URL) will be sent to Microsoft that allows it to identify such dangerous content. Disabling this function provides more privacy but it also gives you less protection while surfing. That's why we recommend your leaving this function enabled.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\PhishingFilter", L"EnabledV9", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SmartScreenEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E115_USER";
        t.title = L"Disable check for saved payment methods by sites (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Websites can check whether the current user has stored payment methods in the browser. By setting this policy, you can prevent verification so that no information about it is transmitted from the browser to the Web site.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PaymentMethodQueryEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E115_MACHINE";
        t.title = L"Disable check for saved payment methods by sites (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Websites can check whether the current user has stored payment methods in the browser. By setting this policy, you can prevent verification so that no information about it is transmitted from the browser to the Web site.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PaymentMethodQueryEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E116_USER";
        t.title = L"Disable sending info about websites visited (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about the websites you visit to Microsoft to improve search and products. By setting this policy, you can prevent sending so that no information is transmitted by the browser.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SendSiteInfoToImproveServices", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E116_MACHINE";
        t.title = L"Disable sending info about websites visited (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about the websites you visit to Microsoft to improve search and products. By setting this policy, you can prevent sending so that no information is transmitted by the browser.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SendSiteInfoToImproveServices", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_METRICS_USER";
        t.title = L"Disable sending data about browser usage (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about usage behavior and crashes to Microsoft to improve the product. By setting this policy, you can prevent sending so that no information is transmitted by the browser.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"MetricsReportingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_METRICS_MACHINE";
        t.title = L"Disable sending data about browser usage (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about usage behavior and crashes to Microsoft to improve the product. By setting this policy, you can prevent sending so that no information is transmitted by the browser.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"MetricsReportingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E118_USER";
        t.title = L"Disable personalizing advertising, search, news and other services (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about usage behavior to Microsoft to improve advertising, search, news, and other Microsoft services. Enable this policy to prevent the browser from sending this information.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PersonalizationReportingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E118_MACHINE";
        t.title = L"Disable personalizing advertising, search, news and other services (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge sends information about usage behavior to Microsoft to improve advertising, search, news, and other Microsoft services. Enable this policy to prevent the browser from sending this information.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PersonalizationReportingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E121_USER";
        t.title = L"Disable suggestions from local providers (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can display suggestions from so-called suggestion providers in the address bar, favorites, and browsing history. By setting this policy, you can prevent this display.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"LocalProvidersEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E121_MACHINE";
        t.title = L"Disable suggestions from local providers (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can display suggestions from so-called suggestion providers in the address bar, favorites, and browsing history. By setting this policy, you can prevent this display.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"LocalProvidersEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E123_USER";
        t.title = L"Disable shopping assistant in Microsoft Edge (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"When visiting websites, Microsoft Edge can automatically search for coupons or discounts, or compare prices. For this purpose, data must be transmitted in the background to appropriate servers in order to provide this function. This can lead to the transmission of private information, which is not desired.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeShoppingAssistantEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E123_MACHINE";
        t.title = L"Disable shopping assistant in Microsoft Edge (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"When visiting websites, Microsoft Edge can automatically search for coupons or discounts, or compare prices. For this purpose, data must be transmitted in the background to appropriate servers in order to provide this function. This can lead to the transmission of private information, which is not desired.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeShoppingAssistantEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E124_USER";
        t.title = L"Disable Edge bar (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"The input bar from Microsoft Edge allows web pages to be accessed directly from the search box that appears on the desktop. If you want to disable this bar, you can do so with this option.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"WebWidgetAllowed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E124_MACHINE";
        t.title = L"Disable Edge bar (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"The input bar from Microsoft Edge allows web pages to be accessed directly from the search box that appears on the desktop. If you want to disable this bar, you can do so with this option.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"WebWidgetAllowed", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E128_USER";
        t.title = L"Disable Sidebar in Microsoft Edge (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"The sidebar on the right side of Microsoft Edge is enabled by default and is used for quick access to applications, but also to search with Bing. That is why it is represented by the Bing icon when it is closed. To remove this edge bar, enable this setting. The next time you launch Microsoft Edge, the setting will be applied.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"HubsSidebarEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E128_MACHINE";
        t.title = L"Disable Sidebar in Microsoft Edge (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"The sidebar on the right side of Microsoft Edge is enabled by default and is used for quick access to applications, but also to search with Bing. That is why it is represented by the Bing icon when it is closed. To remove this edge bar, enable this setting. The next time you launch Microsoft Edge, the setting will be applied.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"HubsSidebarEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E130_USER";
        t.title = L"Disable Enhanced Spell Checking (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the enhanced spelling and grammar check provided by Microsoft Editor. Instead, the basic, local spell check is used, which does not rely on the cloud.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"MicrosoftEditorProofingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E130_MACHINE";
        t.title = L"Disable Enhanced Spell Checking (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the enhanced spelling and grammar check provided by Microsoft Editor. Instead, the basic, local spell check is used, which does not rely on the cloud.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"MicrosoftEditorProofingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E132_USER";
        t.title = L"Hide first run experience and splash screen (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the first-run experience and splash screen when Microsoft Edge is launched for the first time, preventing promotional content from being displayed.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"HideFirstRunExperience", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E132_MACHINE";
        t.title = L"Hide first run experience and splash screen (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the first-run experience and splash screen when Microsoft Edge is launched for the first time, preventing promotional content from being displayed.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"HideFirstRunExperience", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "112_NewEdge_SpotlightExperiencesAndRecommendationsEnabled";
        t.category = L"Microsoft Edge";
        t.title = L"Disable spotlight experiences and recommendations";
        t.description = L"This setting disables promotional spotlight experiences and recommendations from Microsoft, reducing visual clutter and potential privacy concerns.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SpotlightExperiencesAndRecommendationsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E134_USER";
        t.title = L"Disable automatic sign-in from web to browser (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting prevents automatic sign-in to the browser when signing into Microsoft websites, improving privacy by separating web and browser accounts.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"WebToBrowserSignInEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E134_MACHINE";
        t.title = L"Disable automatic sign-in from web to browser (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting prevents automatic sign-in to the browser when signing into Microsoft websites, improving privacy by separating web and browser accounts.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"WebToBrowserSignInEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E135_USER";
        t.title = L"Disable Bing Chat on new tab page (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables Bing Chat on the new tab page, reducing AI features and potential data sharing.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageBingChatEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E135_MACHINE";
        t.title = L"Disable Bing Chat on new tab page (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables Bing Chat on the new tab page, reducing AI features and potential data sharing.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageBingChatEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E136_USER";
        t.title = L"Disable content on new tab page (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables content on the new tab page such as news feed and promotional information, showing a clean, minimal new tab page.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageContentEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E136_MACHINE";
        t.title = L"Disable content on new tab page (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables content on the new tab page such as news feed and promotional information, showing a clean, minimal new tab page.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageContentEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "112_NewEdge_AIGenThemesEnabled";
        t.category = L"Microsoft Edge";
        t.title = L"Disable AI-generated themes";
        t.description = L"This setting disables the AI-generated themes feature in Edge, reducing unnecessary AI processing and potential data sharing.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AIGenThemesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E138_USER";
        t.title = L"Disable built-in AI APIs for websites (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables built-in AI APIs that websites can access, preventing websites from using Edge's AI features.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"BuiltInAIAPIsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E138_MACHINE";
        t.title = L"Disable built-in AI APIs for websites (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables built-in AI APIs that websites can access, preventing websites from using Edge's AI features.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"BuiltInAIAPIsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E139_USER";
        t.title = L"Disable inline Compose feature (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the inline Compose feature (AI writing assistant), reducing AI-based data processing.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ComposeInlineEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E139_MACHINE";
        t.title = L"Disable inline Compose feature (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the inline Compose feature (AI writing assistant), reducing AI-based data processing.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ComposeInlineEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E140_USER";
        t.title = L"Disable Copilot access to page context (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables Copilot access to page context, preventing sending page content to Copilot AI.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"CopilotPageContext", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E140_MACHINE";
        t.title = L"Disable Copilot access to page context (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables Copilot access to page context, preventing sending page content to Copilot AI.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"CopilotPageContext", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E141_USER";
        t.title = L"Disable prompts to make Edge the default browser (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables prompts to make Edge the default browser, reducing unwanted notifications.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DefaultBrowserSettingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E141_MACHINE";
        t.title = L"Disable prompts to make Edge the default browser (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables prompts to make Edge the default browser, reducing unwanted notifications.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DefaultBrowserSettingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E142_USER";
        t.title = L"Disable default browser campaigns (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables campaigns to set Edge as the default browser, preventing promotional interruptions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DefaultBrowserSettingsCampaignEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E142_MACHINE";
        t.title = L"Disable default browser campaigns (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables campaigns to set Edge as the default browser, preventing promotional interruptions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DefaultBrowserSettingsCampaignEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E143_USER";
        t.title = L"Disable diagnostic data collection (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables diagnostic data collection, minimizing data sent to Microsoft. Options are: 0=Off, 1=Required, 2=Optional.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DiagnosticData", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E143_MACHINE";
        t.title = L"Disable diagnostic data collection (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables diagnostic data collection, minimizing data sent to Microsoft. Options are: 0=Off, 1=Required, 2=Optional.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"DiagnosticData", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_SHOPPING_USER";
        t.title = L"Disable shopping assistant (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the shopping assistant feature, preventing automatic price comparison and coupon suggestions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeShoppingAssistantEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "EDGE_SHOPPING_MACHINE";
        t.title = L"Disable shopping assistant (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the shopping assistant feature, preventing automatic price comparison and coupon suggestions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeShoppingAssistantEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E145_USER";
        t.title = L"Hide Microsoft 365 Copilot chat icon (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the Microsoft 365 Copilot chat icon from the browser interface.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"Microsoft365CopilotChatIconEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E145_MACHINE";
        t.title = L"Hide Microsoft 365 Copilot chat icon (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the Microsoft 365 Copilot chat icon from the browser interface.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"Microsoft365CopilotChatIconEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E146_USER";
        t.title = L"Hide Microsoft Rewards (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides Microsoft Rewards notifications and features, reducing promotional content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowMicrosoftRewards", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E146_MACHINE";
        t.title = L"Hide Microsoft Rewards (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides Microsoft Rewards notifications and features, reducing promotional content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowMicrosoftRewards", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E147_USER";
        t.title = L"Disable recommendations in settings (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables recommendations in settings and other areas, reducing promotional content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowRecommendationsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E147_MACHINE";
        t.title = L"Disable recommendations in settings (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables recommendations in settings and other areas, reducing promotional content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowRecommendationsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E148_USER";
        t.title = L"Disable cloud-based tab services (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables cloud-based tab services, preventing syncing tab data to the Microsoft cloud.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TabServicesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E148_MACHINE";
        t.title = L"Disable cloud-based tab services (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables cloud-based tab services, preventing syncing tab data to the Microsoft cloud.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TabServicesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E149_USER";
        t.title = L"Disable text prediction in forms (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables text prediction features in forms, reducing AI-based text analysis.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TextPredictionEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E149_MACHINE";
        t.title = L"Disable text prediction in forms (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables text prediction features in forms, reducing AI-based text analysis.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TextPredictionEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E150_USER";
        t.title = L"Disable visual search (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the visual search feature, preventing sending images to Bing for search.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"VisualSearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E150_MACHINE";
        t.title = L"Disable visual search (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the visual search feature, preventing sending images to Bing for search.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"VisualSearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E151_USER";
        t.title = L"Disable AI-powered history search (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables AI-powered search in browsing history, preventing AI processing of browsing history.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeHistoryAISearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E151_MACHINE";
        t.title = L"Disable AI-powered history search (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables AI-powered search in browsing history, preventing AI processing of browsing history.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeHistoryAISearchEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E156_USER";
        t.title = L"Disable Edge Secure Network (built-in VPN) (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the Microsoft Edge Secure Network (built-in VPN) feature, which routes network traffic through Microsoft's servers. Disabling it prevents any data from being transmitted via this service.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeSecureNetworkEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E156_MACHINE";
        t.title = L"Disable Edge Secure Network (built-in VPN) (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the Microsoft Edge Secure Network (built-in VPN) feature, which routes network traffic through Microsoft's servers. Disabling it prevents any data from being transmitted via this service.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"EdgeSecureNetworkEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E152_USER";
        t.title = L"Allow user control of local AI features (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting allows users to control local AI foundational model features in Microsoft Edge.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"GenAILocalFoundationalModelSettings", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E152_MACHINE";
        t.title = L"Allow user control of local AI features (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting allows users to control local AI foundational model features in Microsoft Edge.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"GenAILocalFoundationalModelSettings", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E119_USER";
        t.title = L"Disable use of web service to resolve navigation errors (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), a connection is established to a web service to correct this error (e.g. suggestion of the correct web address). By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ResolveNavigationErrorsUseWebService", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E119_MACHINE";
        t.title = L"Disable use of web service to resolve navigation errors (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), a connection is established to a web service to correct this error (e.g. suggestion of the correct web address). By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ResolveNavigationErrorsUseWebService", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E120_USER";
        t.title = L"Disable suggestion of similar sites when website cannot be found (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), then a similar Web page is suggested for the original input. By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AlternateErrorPagesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E120_MACHINE";
        t.title = L"Disable suggestion of similar sites when website cannot be found (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), then a similar Web page is suggested for the original input. By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"AlternateErrorPagesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E122_USER";
        t.title = L"Disable preload of pages for faster browsing and searching (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can predict which page to load by entering the web address in the address field. This is to do this, certain services are preconfigured in the background to speed up loading. By setting this policy, you can prevent this preloading.  This setting is not recommended because it slows down the speed of searching and displaying a Web page.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NetworkPredictionOptions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E122_MACHINE";
        t.title = L"Disable preload of pages for faster browsing and searching (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge can predict which page to load by entering the web address in the address field. This is to do this, certain services are preconfigured in the background to speed up loading. By setting this policy, you can prevent this preloading.  This setting is not recommended because it slows down the speed of searching and displaying a Web page.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NetworkPredictionOptions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E125_USER";
        t.title = L"Disable saving passwords for websites (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge stores passwords in its own password manager. These are automatically filled in when you visit a website for which a password is stored. If this setting is deactivated, no new passwords will be saved in the future, but existing ones will continue to be used.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PasswordManagerEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E125_MACHINE";
        t.title = L"Disable saving passwords for websites (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"Microsoft Edge stores passwords in its own password manager. These are automatically filled in when you visit a website for which a password is stored. If this setting is deactivated, no new passwords will be saved in the future, but existing ones will continue to be used.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"PasswordManagerEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E126_USER";
        t.title = L"Disable site safety services for more information about a visited website (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"By clicking on the lock in the address bar of Microsoft Edge, you can get more safety information about a website. For this purpose, information is transmitted to Microsoft Bing, which could disclose information. Because this information can be useful for evaluating a website, this setting is only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SiteSafetyServicesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E126_MACHINE";
        t.title = L"Disable site safety services for more information about a visited website (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"By clicking on the lock in the address bar of Microsoft Edge, you can get more safety information about a website. For this purpose, information is transmitted to Microsoft Bing, which could disclose information. Because this information can be useful for evaluating a website, this setting is only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"SiteSafetyServicesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E131";
        t.category = L"Microsoft Edge";
        t.title = L"Disable automatic redirection from Internet Explorer to Microsoft Edge";
        t.description = L"This setting disables the IEToEdge Browser Helper Object (BHO), preventing the automatic redirection from Internet Explorer to Microsoft Edge. Please note that this may cause issues with applications that still rely on Internet Explorer.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ext\\CLSID", L"{1FD49718-1D00-4B19-AF5F-070AF6D5D54C}", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E153_USER";
        t.title = L"Disable startup boost (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the startup boost feature that keeps Edge processes running in the background, saving system resources but increasing initial launch time.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"StartupBoostEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E153_MACHINE";
        t.title = L"Disable startup boost (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting disables the startup boost feature that keeps Edge processes running in the background, saving system resources but increasing initial launch time.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"StartupBoostEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E154_USER";
        t.title = L"Hide default top sites on new tab page (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the default top sites from the new tab page, providing a cleaner new tab experience.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageHideDefaultTopSites", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E154_MACHINE";
        t.title = L"Hide default top sites on new tab page (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the default top sites from the new tab page, providing a cleaner new tab experience.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"NewTabPageHideDefaultTopSites", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E155_USER";
        t.title = L"Hide Adobe Acrobat subscription button (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the Adobe Acrobat subscription button in the PDF viewer, reducing third-party promotions.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowAcrobatSubscriptionButton", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E155_MACHINE";
        t.title = L"Hide Adobe Acrobat subscription button (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting hides the Adobe Acrobat subscription button in the PDF viewer, reducing third-party promotions.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"ShowAcrobatSubscriptionButton", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E129_USER";
        t.title = L"Disable the Microsoft Account Sign-In Button (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"This setting removes the sign-in button in Microsoft Edge. You won’t be able to sign in with a Microsoft account, and data synchronization like favorites, passwords, and settings will be disabled.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"BrowserSignin", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E129_MACHINE";
        t.title = L"Disable the Microsoft Account Sign-In Button (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"This setting removes the sign-in button in Microsoft Edge. You won’t be able to sign in with a Microsoft account, and data synchronization like favorites, passwords, and settings will be disabled.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"BrowserSignin", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E127_USER";
        t.title = L"Disable typosquatting checker for site addresses (User)";
        t.scope = TargetScope::User;
        t.category = L"Microsoft Edge";
        t.description = L"When entering site addresses in the Edge, they are checked for typos and corrected, so that you do not accidentally end up on a wrong (possibly malicious) website. This involves using Microsoft services to which the input must be sent. Because the risk of a fake website is high, enabling this setting is not recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TyposquattingCheckerEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "E127_MACHINE";
        t.title = L"Disable typosquatting checker for site addresses (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Microsoft Edge";
        t.description = L"When entering site addresses in the Edge, they are checked for typos and corrected, so that you do not accidentally end up on a wrong (possibly malicious) website. This involves using Microsoft services to which the input must be sent. Because the risk of a fake website is high, enabling this setting is not recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Edge", L"TyposquattingCheckerEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Miscellaneous
    // ==========================================
    {
        Tweak t;
        t.id = "M022_USER";
        t.title = L"Disable feedback reminders (User)";
        t.scope = TargetScope::User;
        t.category = L"Miscellaneous";
        t.description = L"Microsoft often asks for feedback and transfers \"Diagnostics and user data\". If you want to prevent this, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"DoNotShowFeedbackNotifications", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M022_MACHINE";
        t.title = L"Disable feedback reminders (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Miscellaneous";
        t.description = L"Microsoft often asks for feedback and transfers \"Diagnostics and user data\". If you want to prevent this, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Siuf\\Rules", L"NumberOfSIUFInPeriod", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "SHELL_PROMOTED_APPS";
        t.category = L"Miscellaneous";
        t.title = L"Disable automatic installation of recommended Windows Store Apps";
        t.description = L"Windows automatically installs suggested apps from the Windows Store in the background. This setting should be enabled if you want to prevent this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SilentInstalledAppsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M005";
        t.category = L"Miscellaneous";
        t.title = L"Disable tips, tricks, and suggestions while using Windows";
        t.description = L"From time to time, Windows displays tips and tricks, as well as suggestions for usage. This setting must be enabled if you want to prevent this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SoftLandingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M024";
        t.category = L"Miscellaneous";
        t.title = L"Disable Windows Media Player Diagnostics";
        t.description = L"Windows Media Player may send diagnostic information to Microsoft to improve services. This can be private information, so disabling it is recommended.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\MediaPlayer\\Preferences", L"UsageTracking", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M012";
        t.category = L"Miscellaneous";
        t.title = L"Disable Key Management Service Online Activation";
        t.description = L"Windows periodically sends information to Microsoft to verify the activation state. Enable this setting if you want to block this online check. Doing so may have side effects when using Windows, which is why this setting is only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\Software Protection Platform", L"NoGenTicket", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M013";
        t.category = L"Miscellaneous";
        t.title = L"Disable automatic download and update of map data";
        t.description = L"This setting prevents Windows from automatically downloading and automatically updating (geographic) maps. This restricts applications that need these cards and is therefore only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Maps", L"AutoDownloadAndUpdateMapData", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M014";
        t.category = L"Miscellaneous";
        t.title = L"Disable unsolicited network traffic on the offline maps settings page";
        t.description = L"Accessing the settings page for offline maps may generate network traffic that is already unwanted. Under certain conditions, this activity may be shared with Microsoft and HERE, the card manufacturer. Disabling this setting can prevent this. Since doing so disables the entire Offline Map Settings page, this setting is only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Maps", L"AllowUntriggeredNetworkTrafficOnSettingsPage", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M023";
        t.category = L"Miscellaneous";
        t.title = L"Disable installation of PC Health Check";
        t.description = L"PC Health Check is an application from Microsoft to check the compatibility of the PC for Windows 11. With the KB5005463 patch, this is no longer optional, but is installed automatically. This setting prevents the installation.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\PCHC", L"PreviousUninstall", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M026";
        t.category = L"Miscellaneous";
        t.title = L"Disable remote assistance connections to this computer";
        t.description = L"You can allow other people, such as external support technicians, or even friends and family, to access your PC so that they can help you with any maintenance or troubleshooting. This may pose a risk and should therefore be deactivated. Only for a real and verified request should this option be allowed again.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services", L"fAllowToGetHelp", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M027";
        t.category = L"Miscellaneous";
        t.title = L"Disable remote connections to this computer";
        t.description = L"Disabling remote connections prevents sessions via Terminal Server or Remote Desktop. This prevents unauthorized third parties from taking over the PC. Allow remote connections only when you need to establish a verified connection.  <sb>Warning: Applying this setting blocks remote connections. Do not apply it to a remote PC (for example, in the cloud), because you will no longer be able to connect to it afterwards!</sb>";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"fDenyTSConnections", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M028";
        t.category = L"Miscellaneous";
        t.title = L"Disable the desktop icon for information on \"Windows Spotlight\"";
        t.description = L"If \"Windows Spotlight\" is selected as the background image, then sporadically further information is offered by means of an icon on the desktop. With this setting you can deactivate this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", L"{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M032";
        t.category = L"Miscellaneous";
        t.title = L"Disable Start menu recommendations for tips, shortcuts and new apps";
        t.description = L"This setting disables Start menu recommendations for tips, shortcuts, new apps and similar promoted content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Start_IrisRecommendations", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M033";
        t.category = L"Miscellaneous";
        t.title = L"Disable Start menu account notifications";
        t.description = L"This setting disables Microsoft account related notifications and badges in the Start menu profile area.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Start_AccountNotifications", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M034";
        t.category = L"Miscellaneous";
        t.title = L"Disable Settings app account notifications";
        t.description = L"This setting disables Microsoft account related notifications and suggestions in the Windows Settings app.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SystemSettings\\AccountNotifications", L"EnableAccountNotifications", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "N001";
        t.category = L"Miscellaneous";
        t.title = L"Disable Network Connectivity Status Indicator";
        t.description = L"Windows uses NCSI to establish connectivity to the Internet. To do this, specially defined Microsoft servers are contacted and then data transmitted. Deactivating this function prevents this from happening. Since some programs rely on this NCSI functionality, disabling it can, under certain conditions, result in interference.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet", L"EnableActiveProbing", RegType::Dword, 0, 0, L"", L"", false });
        t.serviceActions.push_back({ L"Internet", 4, 2, true });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Mobile Devices & Phone Link
    // ==========================================
    {
        Tweak t;
        t.id = "D001";
        t.category = L"Mobile Devices & Phone Link";
        t.title = L"Disable access to mobile devices";
        t.description = L"Windows can connect to mobile devices. Data can be transferred between the mobile device (e.g. smartphone) and the Windows PC, which may reveal private data. This can be prevented with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Mobility", L"CrossDeviceEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "D002";
        t.category = L"Mobile Devices & Phone Link";
        t.title = L"Disable Phone Link app";
        t.description = L"The Phone Link application from Microsoft connects the PC to a mobile device. In doing so, data can be forwarded via Microsoft servers, which can mean a potential violation of privacy. This setting deactivates the application.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Mobility", L"PhoneLinkEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "D003";
        t.category = L"Mobile Devices & Phone Link";
        t.title = L"Disable showing suggestions for using mobile devices with Windows";
        t.description = L"Windows displays information on the use of mobile devices with the PC. These notifications can be deactivated with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Mobility", L"OptedIn", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "D104";
        t.category = L"Mobile Devices & Phone Link";
        t.title = L"Disable connecting the PC to mobile devices";
        t.description = L"This setting prevents Windows from connecting to mobile devices, in particular smartphones, and thus prevents data exchange, which can jeopardize the privacy and security of the PC under certain circumstances.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"EnableMmx", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Office & Outlook
    // ==========================================
    {
        Tweak t;
        t.id = "OFFICE_TELEMETRY";
        t.category = L"Office & Outlook";
        t.title = L"Disable telemetry for Microsoft Office";
        t.description = L"Microsoft Office transmits a lot of telemetry information to Microsoft. User data is collected and transmitted. To reduce data outflow, this option should be set.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\Common\\ClientTelemetry", L"DisableTelemetry", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F014";
        t.category = L"Office & Outlook";
        t.title = L"Disable diagnostic data submission";
        t.description = L"Microsoft Office sends diagnostic information to Microsoft. This may include user-related information. This option can be used to disable both required and optional submissions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\Common\\ClientTelemetry", L"SendTelemetry", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F015";
        t.category = L"Office & Outlook";
        t.title = L"Disable participation in the Customer Experience Improvement Program";
        t.description = L"Users can participate in the Customer Experience Improvement Program. In doing so, information about usage behavior is transmitted to Microsoft. Among other things, the IP address of the computer is transmitted. With this option, participation and thus transmission of data can be deactivated.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"QMEnable", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F016";
        t.category = L"Office & Outlook";
        t.title = L"Disable the display of LinkedIn information";
        t.description = L"Microsoft Office can automatically obtain and display information from the LinkedIn network about its own contacts. Information is transmitted to LinkedIn servers in order to provide the data. This data transmission can be prevented with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"LinkedIn", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F001";
        t.category = L"Office & Outlook";
        t.title = L"Disable inline text prediction in mails";
        t.description = L"When writing mail, Microsoft Outlook can suggest text suggestions for completing a sentence. For this purpose, information is transmitted to a cloud service in order to determine the text suggestion. This can lead to unwanted information outflows and should therefore be disabled.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Office\\", L"InlineTextPrediction", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F003";
        t.category = L"Office & Outlook";
        t.title = L"Disable logging for Microsoft Office Telemetry Agent";
        t.description = L"Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. This function can be deactivated for the local computer.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"Enablelogging", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F004";
        t.category = L"Office & Outlook";
        t.title = L"Disable upload of data for Microsoft Office Telemetry Agent";
        t.description = L"Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. The storage of this data on a data storage provided by the company can hereby be deactivated for the local computer.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"EnableUpload", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F005";
        t.category = L"Office & Outlook";
        t.title = L"Obfuscate file names when uploading telemetry data";
        t.description = L"Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. File names are usually transmitted in plain text and can contain sensitive information. This setting enables the obfuscation of these file names, making it more difficult to draw conclusions.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"EnableFileObfuscation", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F007";
        t.category = L"Office & Outlook";
        t.title = L"Disable Microsoft Office surveys";
        t.description = L"Microsoft may from time to time conduct surveys when using Office products to receive feedback from users. Under certain circumstances, this may be personal data.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"SurveyEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F008";
        t.category = L"Office & Outlook";
        t.title = L"Disable feedback to Microsoft";
        t.description = L"Microsoft provides ways to send feedback from the Office products. These can be deactivated with this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F009";
        t.category = L"Office & Outlook";
        t.title = L"Disable Microsoft's feedback tracking";
        t.description = L"If feedback on Office products is transmitted to Microsoft, an e-mail address can also be transmitted that allows Microsoft to ask questions. If this is not to be done, then the setting should be activated.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"IncludeEmail", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F018";
        t.category = L"Office & Outlook";
        t.title = L"Set diagnostic data level to minimum (Neither)";
        t.description = L"Controls the level of diagnostic data sent by Microsoft 365 applications to Microsoft. Setting this to \"Neither\" ensures no diagnostic data is collected or transmitted, maximizing privacy protection.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"DiagnosticDataLevel", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F019";
        t.category = L"Office & Outlook";
        t.title = L"Hide privacy settings notification on first run";
        t.description = L"When Microsoft 365 applications are first launched, a privacy notification may appear prompting users to review privacy settings. This setting suppresses that notification to avoid interruption.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"ShownFirstRunOptin", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F020";
        t.category = L"Office & Outlook";
        t.title = L"Disable the Office first run movie";
        t.description = L"When Office is started for the first time, it plays an introductory video about signing in to Office. This video is retrieved from Microsoft over the internet. This setting suppresses the video and the connection that goes with it.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"disablemovie", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "OUT_HIDE_TOGGLE";
        t.category = L"Office & Outlook";
        t.title = L"Hide the \"Try the new Outlook\" toggle in Outlook";
        t.description = L"Classic Outlook shows a toggle for switching to the new Outlook. Unlike classic Outlook, the new Outlook synchronizes email accounts from other providers (IMAP, POP) via Microsoft's cloud and stores the access credentials there. This setting hides the toggle so that the switch cannot happen by accident. A new Outlook that is already installed is not affected.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"HideNewOutlookToggle", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "OUT_BLOCK_MIGRATE";
        t.category = L"Office & Outlook";
        t.title = L"Disable automatic migration to the new Outlook";
        t.description = L"Microsoft is gradually switching users of classic Outlook to the new Outlook automatically. The new Outlook synchronizes email accounts from other providers (IMAP, POP) via Microsoft's cloud and stores the access credentials there. This setting blocks the automatic switch, so classic Outlook remains in use.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"NewOutlookMigrationUserSetting", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F006";
        t.category = L"Office & Outlook";
        t.title = L"Disable automatic receipt of updates";
        t.description = L"This setting determines whether Microsoft Office sends diagnostic data to Microsoft and then transfers small error corrections back to the computer. These updates improve the stability of Microsoft Office, so disabling the setting is only conditionally recommended.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"UpdateReliabilityData", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F010";
        t.category = L"Office & Outlook";
        t.title = L"Disable connected experiences in Office";
        t.description = L"Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"DisconnectedState", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F011";
        t.category = L"Office & Outlook";
        t.title = L"Disable connected experiences with content analytics";
        t.description = L"Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"UserContentDisabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F012";
        t.category = L"Office & Outlook";
        t.title = L"Disable online content downloading for connected experiences";
        t.description = L"Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"DownloadContentDisabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F013";
        t.category = L"Office & Outlook";
        t.title = L"Disable optional connected experiences in Office";
        t.description = L"Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"ControllerConnectedServicesEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "F021";
        t.category = L"Office & Outlook";
        t.title = L"Disable signing in to Office";
        t.description = L"Office offers to sign in with a Microsoft account or an organizational account so that documents and settings can be synchronized with the cloud. This setting blocks both types of sign-in, so Office no longer establishes a connection for this purpose. Please note that features which require a sign-in, such as OneDrive or Microsoft 365, will then no longer be available.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Policies\\Microsoft\\Office\\", L"SignInOptions", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Privacy & Tracking
    // ==========================================
    {
        Tweak t;
        t.id = "P001";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable sharing of handwriting data";
        t.description = L"If you can write text in your computer manually, you can send a writing sample to Microsoft \"to enhance future hand writing recognition functions in Windows versions\". If you don't want to pass on your handwriting sample to Microsoft, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\TabletPC", L"PreventHandwritingDataSharing", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P002";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable sharing of handwriting error reports";
        t.description = L"If you can write text in your computer manually, you can send error reports to Microsoft \"to enhance future hand writing recognition functions in Windows versions\". If you don't want to pass on your handwriting sample to Microsoft, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\HandwritingErrorReports", L"PreventHandwritingErrorReports", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P003";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Inventory Collector";
        t.description = L"Inventory Collector is primarily used in company networks and enables an overview of installed applications, devices and system information of all computers in the network. If you don't need such an overview of all computers in your network, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat", L"DisableInventory", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P004";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable camera in logon screen";
        t.description = L"Windows offers the possibility to operate the camera App from a locked PC directly from the locked screen. If you are unsure who uses your PC during your absence, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization", L"NoLockScreenCamera", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_AD_ID_USER";
        t.title = L"Disable and reset Advertising ID and info (User)";
        t.scope = TargetScope::User;
        t.category = L"Privacy & Tracking";
        t.description = L"Windows creates a commercial ID to show you advertisements based on your installed and used apps, and your browsing history. These advertisements can also be displayed in non-Microsoft apps.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_AD_ID_MACHINE";
        t.title = L"Disable and reset Advertising ID and info (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Privacy & Tracking";
        t.description = L"Windows creates a commercial ID to show you advertisements based on your installed and used apps, and your browsing history. These advertisements can also be displayed in non-Microsoft apps.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_KEYSTROKES";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable transmission of typing information";
        t.description = L"Windows transfers data of your writing habits. Which data this is specifically and to what extent they are anonymous is unclear at this point.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Input\\TIPC", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P026";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable advertisements via Bluetooth";
        t.description = L"Windows can receive and transmit advertisements via Bluetooth, provided it’s near a compatible transmitter or receiver (normally circa 15-40 meters, up to 250 meters with modern devices). At the same time, additional information for optimizing advertisements can be exchanged. Disable this setting if you want to turn this feature off.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Bluetooth", L"AllowAdvertising", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P027";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable the Windows Customer Experience Improvement Program";
        t.description = L"the Windows Customer Experience Improvement Program collects information about hardware configuration and the use of software and services, in order to compile user trends and patterns. According to Microsoft, no personal information such as names or addresses is included. We recommend disabling this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\SQMClient\\Windows", L"CEIPEnable", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P028";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable backup of text messages into the cloud";
        t.description = L"Text messages saved on the device can also be saved on external servers (e.g. by Microsoft) and restored later, should this be necessary. This setting must be enabled to prevent saving messages outside your own server.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Messaging", L"AllowMessageSync", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P064";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable suggestions in the timeline";
        t.description = L"Windows occasionally displays advertisements in the Windows Explorer timeline (e.g. for OneDrive). Setting this setting suppresses these pop-ups.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-353698Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P065";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable suggestions in Start";
        t.description = L"Windows occasionally displays ads in the Start menu (e.g. for new apps). Setting this setting suppresses these pop-ups.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SystemPaneSuggestionsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-338388Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P066";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable tips, tricks, and suggestions when using Windows";
        t.description = L"Windows occasionally displays tips, tricks, and suggestions in the Notification Pane and Info Center. Setting this setting suppresses these pop-ups.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-338389Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P067";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable showing suggested content in the Settings app";
        t.description = L"Windows occasionally displays suggestions in system settings. Setting this setting suppresses these pop-ups.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-338393Enabled", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-353694Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P070";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable the possibility of suggesting to finish the setup of the device";
        t.description = L"This option disables the occasional display of notices when you start using Microsoft services such as Windows Hello or OneDrive and the associated prompt to create a Microsoft account.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-310093Enabled", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UserProfileEngagement", L"ScoobeSystemSettingEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P069";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Windows Error Reporting";
        t.description = L"In the event of fatal failures in applications or system components, Windows creates an error report and uploads it to Microsoft servers. This may include personal information due to the memory dump and should therefore be disabled.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting", L"Disabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P095";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable cloud consumer account state content";
        t.description = L"Windows uses the state of your Microsoft account to show account related content in the Start menu, in the Settings app and in notifications - for example advertising for Microsoft services. The account state is queried online for this purpose. This setting turns off that content.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent", L"DisableConsumerAccountStateContent", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P096";
        t.category = L"Privacy & Tracking";
        t.title = L"Limit crash dump collection";
        t.description = L"If you have agreed to send optional diagnostic data, Windows Error Reporting may transmit complete memory dumps and heap dumps. These can contain anything that was in memory at the moment of the crash, including personal data. This setting limits the transmission to kernel mini dumps and user mode triage dumps.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"LimitDumpCollection", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P009";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable biometric features (Windows Hello fingerprint, face, iris)";
        t.description = L"This option blocks the Windows biometric framework that Windows Hello uses for fingerprint, facial recognition and iris scan. If you do not sign in to your PC with biometric hardware, you can deactivate this function. Your Windows Hello PIN is not affected. Fingerprint or face data that is already enrolled is not deleted; it stays on the device until you remove it in the Windows settings under \"Accounts > Sign-in options\".";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Biometrics", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P010";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable app notifications";
        t.description = L"When deactivating this function, apps can no longer display notifications on the tiles, the locked screen or desktop. For apps that post reminders, this may not be the best solution.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications", L"ToastEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P015";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable access to the browser’s language list";
        t.description = L"Access to the language list of the browser enables websites to display local contents.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P068";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable text suggestions when typing on the software keyboard";
        t.description = L"With these settings, text suggestions can be deactivated when typing on the software keyboard. Data can be loaded from the Internet to predict the words. If you want to avoid this, you should activate this setting. This means that typing on this keyboard can also take longer.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\TabletTip\\1.7", L"EnableTextPrediction", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P097";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable Microsoft consumer features";
        t.description = L"The Microsoft consumer features install suggested apps automatically, show personalized recommendations and display notifications about your Microsoft account. All of this is obtained from Microsoft over the internet. This setting turns these features off. According to Microsoft the policy only takes effect on the Enterprise and Education editions.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent", L"DisableWindowsConsumerFeatures", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P099";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable display of office.com files in Explorer";
        t.description = L"On its home page, Windows Explorer shows recently used, favorite and recommended files from office.com. To do so it retrieves metadata about your cloud files from Microsoft. This setting prevents both the query and the display. The policy is not available on the Home edition of Windows.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer", L"DisableGraphRecentItems", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P016";
        t.category = L"Privacy & Tracking";
        t.title = L"Disable sending URLs from apps to Windows Store";
        t.description = L"Windows analyzes the websites you access from your apps and sends this information onto the Windows Store. This feature can potentially give you more security but, at the same time, it sends data about your behavior on apps to Microsoft. Deactivate this feature if you don't want this.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppHost", L"EnableWebContentEvaluation", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Security & Network
    // ==========================================
    {
        Tweak t;
        t.id = "S001";
        t.category = L"Security & Network";
        t.title = L"Disable password reveal button";
        t.description = L"If you log on to Windows, you can display the entered password while clicking on the eye symbol for a couple of seconds to check for correctness. There is a risk that somebody might peer over your shoulder while doing that. If you don't want to take this risk, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CredUI", L"DisablePasswordReveal", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S002";
        t.category = L"Security & Network";
        t.title = L"Disable user steps recorder";
        t.description = L"Steps recorder is used to record everything you do on your computer automatically (incl. writing in elements you have clicked on and screenshots of each click motion). The finished record can help a support specialist to solve a problem on a PC. If you don't need this function, then deactivate it to enhance your security.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat", L"DisableUAR", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S003";
        t.category = L"Security & Network";
        t.title = L"Disable telemetry";
        t.description = L"Microsoft collects information about your computer, installed programs, and possible problems in Windows. Error reports are also sent to Microsoft. Disable this function if you don't want Microsoft to have this information.  <u>Note:</u> According to user reports, disabling this setting may result in problems with registration of the XBOX program.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\DiagTrack", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\dmwappushservice", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Control\\WMI\\AutoLogger\\AutoLogger-Diagtrack-Listener", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.serviceActions.push_back({ L"DiagTrack", 4, 2, true });
        t.serviceActions.push_back({ L"dmwappushservice", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S015";
        t.category = L"Security & Network";
        t.title = L"Disable WiFi Sense for all users";
        t.description = L"WiFi sense connects automatically to public wifi hotspots which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal wifi password will be stored in a Microsoft server.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\config", L"AutoConnectAllowedOEM", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S006";
        t.category = L"Security & Network";
        t.title = L"Disable WiFi Sense for user";
        t.description = L"WiFi sense connects automatically to public wifi hotspots which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal wifi password will be stored in a Microsoft server.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\features\\", L"FeatureStates", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S007";
        t.category = L"Security & Network";
        t.title = L"Disable WiFi Sense of my contacts";
        t.description = L"WiFi sense connects automatically to wifis of your contacts which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal WiFi password will be stored in a Microsoft server.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\features\\", L"FeatureStates", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S008";
        t.category = L"Security & Network";
        t.title = L"Disable Internet access of Windows Media Digital Rights Management (DRM)";
        t.description = L"Certain music and video files have a so-called DRM protection, which ensures that these files can only be played on your computer or restricts the amount of copies made. If you don't own DRM protected files, then deactivate this function, otherwise it is possible that you won't be able to use these files anymore.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\WMDRM", L"DisableOnline", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S009";
        t.category = L"Security & Network";
        t.title = L"Disable app access to wireless connections";
        t.description = L"When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{A8804298-2D5F-42E3-9531-9C8C39EB29CE}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S010";
        t.category = L"Security & Network";
        t.title = L"Disable app access to loosely coupled devices";
        t.description = L"If this feature is disabled, apps must not establish wireless connections that were not previously authorized (e.g. beacons). This may limit some apps in their function or stop working at all.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\LooselyCoupled", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S116";
        t.category = L"Security & Network";
        t.title = L"Disable NFC (Near Field Communication)";
        t.description = L"Disables the NFC Secure Element Manager service (SEMgrSvc), preventing NFC-based communication. A system reboot is required for this change to take effect.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\SEMgrSvc", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.serviceActions.push_back({ L"SEMgrSvc", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S117";
        t.category = L"Security & Network";
        t.title = L"Disable wireless display (Miracast/WiDi)";
        t.description = L"Prevents this PC from being discovered or projected to as a wireless display (Miracast/WiDi). Disabling wireless display protocols helps reduce the attack surface.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Connect", L"AllowProjectionToPC", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S118";
        t.category = L"Security & Network";
        t.title = L"Disable mobile broadband (cellular/WWAN)";
        t.description = L"Disables the Windows WWAN AutoConfig service, which manages mobile broadband (cellular) connections. Note: This setting only has an effect if a mobile broadband adapter is present in the system.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WwanSvc", L"WwanAutoConfig", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S119";
        t.category = L"Security & Network";
        t.title = L"Disable WiFi Direct";
        t.description = L"Disables the WiFi Direct Connection Manager service (WFDSConMgrSvc), preventing WiFi Direct peer-to-peer connections. A system reboot is required for this change to take effect. Features that build on WiFi Direct will then no longer be available, for example wireless displays (Miracast), Nearby Sharing and printing via WiFi Direct; depending on the WLAN adapter, the mobile hotspot may stop working as well.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\WFDSConMgrSvc", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.serviceActions.push_back({ L"WFDSConMgrSvc", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S120";
        t.category = L"Security & Network";
        t.title = L"Restrict Bluetooth pairing";
        t.description = L"Restricts Bluetooth functionality via Group Policy, preventing new device pairing. This helps reduce the wireless attack surface.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Bluetooth", L"AllowBluetooth", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S012";
        t.category = L"Security & Network";
        t.title = L"Disable Microsoft SpyNet membership";
        t.description = L"As soon as Microsoft Defender recognizes a threat caused by a change in files on your computer, this information can be sent to Microsoft for analysis. This is part of a so-called SpyNet membership. If you do not want this, deactivate this option.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet", L"SpyNetReporting", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S013";
        t.category = L"Security & Network";
        t.title = L"Disable submitting data samples to Microsoft";
        t.description = L"As soon as Microsoft Defender recognizes a possible threat, samples of data can be sent to Microsoft for analysis. If you do not want to send sample data, deactivate this option.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet", L"SubmitSamplesConsent", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S014";
        t.category = L"Security & Network";
        t.title = L"Disable reporting of malware infection information";
        t.description = L"If Microsoft Defender or another security program finds an infection on your computer caused by malware, this information will be sent to Microsoft. If you do not want this, deactivate this option.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\MRT", L"DontReportInfectionInformation", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "S011";
        t.category = L"Security & Network";
        t.title = L"Disable Microsoft Defender";
        t.description = L"Deactivation not recommended! Microsoft Defender is the built-in Windows antivirus solution. Only deactivate this function if you use another regularly updated antivirus solution.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender", L"DisableAntiSpyware", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Synchronization
    // ==========================================
    {
        Tweak t;
        t.id = "Y001";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of all settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you deactivate this setting, the entire synchronization of all further settings in this category will be deactivated too, regardless of their individual settings.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync", L"SyncPolicy", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y002";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of design settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Personalization", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y003";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of browser settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\BrowserSettings", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y004";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of credentials (passwords)";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Credentials", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y005";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of language settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Language", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y006";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of accessibility settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Accessibility", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "Y007";
        t.category = L"Synchronization";
        t.title = L"Disable synchronization of advanced Windows settings";
        t.description = L"If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Windows", L"Enabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Taskbar & Start Menu
    // ==========================================
    {
        Tweak t;
        t.id = "M019_USER";
        t.title = L"Disable news and interests in the task bar (User)";
        t.scope = TargetScope::User;
        t.category = L"Taskbar & Start Menu";
        t.description = L"This setting allows you to disable the display of news and interesting topics in the taskbar if you do not want them to be displayed. In the active state, data from Microsoft Bing services is retrieved at regular intervals and thus Internet connections are established.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Feeds", L"EnableFeeds", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Dsh", L"AllowNewsAndInterests", RegType::Dword, 1, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M019_MACHINE";
        t.title = L"Disable news and interests in the task bar (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Taskbar & Start Menu";
        t.description = L"This setting allows you to disable the display of news and interesting topics in the taskbar if you do not want them to be displayed. In the active state, data from Microsoft Bing services is retrieved at regular intervals and thus Internet connections are established.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds", L"ShellFeedsTaskbarViewMode", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M016";
        t.category = L"Taskbar & Start Menu";
        t.title = L"Disable search box in task bar";
        t.description = L"You can disable the search box in the task bar with this setting, if you do not want to have it displayed.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"SearchboxTaskbarMode", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M015";
        t.category = L"Taskbar & Start Menu";
        t.title = L"Disable People icon in the taskbar";
        t.description = L"Windows can display the People icon in the taskbar. Disable this setting to prevent this.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People", L"PeopleBand", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M017_USER";
        t.title = L"UNKNOWN (User)";
        t.scope = TargetScope::User;
        t.category = L"Taskbar & Start Menu";
        t.description = L"The \"Meet now\" feature is part of Microsoft Skype and is used to quickly create an online meeting. If you do not want this feature, then activate this setting.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"HideSCAMeetNow", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M017_MACHINE";
        t.title = L"UNKNOWN (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Taskbar & Start Menu";
        t.description = L"The \"Meet now\" feature is part of Microsoft Skype and is used to quickly create an online meeting. If you do not want this feature, then activate this setting.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"HideSCAMeetNow", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M021";
        t.category = L"Taskbar & Start Menu";
        t.title = L"Disable widgets in Windows Explorer";
        t.description = L"Windows 11 introduced widgets that can display personalized information. For this purpose, information is exchanged with corresponding servers on the Internet. If you want to deactivate this option in Windows Explorer, then you have to activate this setting.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarDa", RegType::Dword, 0, 1, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Telemetry & Diagnostics
    // ==========================================
    {
        Tweak t;
        t.id = "TEL_DIAGDATA";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable application telemetry";
        t.description = L"By deactivating this function, Microsoft will not send telemetry data, i.e. usage data of programs, crashes, your entry behavior and similar are no longer sent to Microsoft.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowTelemetry", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat", L"AITEnable", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_TAILORED_USER";
        t.title = L"Disable diagnostic data from customizing user experiences (User)";
        t.scope = TargetScope::User;
        t.category = L"Telemetry & Diagnostics";
        t.description = L"Microsoft can record diagnostic data from your computer and evaluate it in order to improve your use of Windows. While doing so, a large amount of such data will be compiled and transmitted. Disable this feature if you want to stop this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy", L"TailoredExperiencesWithDiagnosticDataEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "PRIV_TAILORED_MACHINE";
        t.title = L"Disable diagnostic data from customizing user experiences (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Telemetry & Diagnostics";
        t.description = L"Microsoft can record diagnostic data from your computer and evaluate it in order to improve your use of Windows. While doing so, a large amount of such data will be compiled and transmitted. Disable this feature if you want to stop this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy", L"TailoredExperiencesWithDiagnosticDataEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "U006";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable diagnostic log collection";
        t.description = L"Diagnostic logs are created to collect information about problems on the device. These are sent when diagnostic data delivery is enabled. This option prevents the creation of log files.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "U007";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Disable downloading of OneSettings configuration settings";
        t.description = L"Microsoft's OneSettings service allows automatic download of configuration settings to address problems on the machine. This involves establishing a connection to Microsoft servers and possibly exchanging machine-related information. This option can be used to disable this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "U008";
        t.category = L"Telemetry & Diagnostics";
        t.title = L"Do not send device name in diagnostic data";
        t.description = L"This setting prevents Windows from including the device name in diagnostic data sent to Microsoft.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Windows Explorer
    // ==========================================
    {
        Tweak t;
        t.id = "M006";
        t.category = L"Windows Explorer";
        t.title = L"Disable occasionally showing app suggestions in Start menu";
        t.description = L"The Start menu displays suggestions for apps from the Windows Store at irregular intervals. When you click these apps, they are automatically installed and made available on the system. This setting must be enabled if you want to avoid this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SystemPaneSuggestionsEnabled", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M011";
        t.category = L"Windows Explorer";
        t.title = L"Do not show recently opened items in Jump Lists on \"Start\" or the taskbar";
        t.description = L"Windows saves recently opened items in special lists so that these may be quickly started over the Start menu or the taskbar. Enable this setting if you wish to hide these items or the file names.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Start_TrackDocs", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "M010";
        t.category = L"Windows Explorer";
        t.title = L"Disable ads in Windows Explorer/OneDrive";
        t.description = L"Advertizing pop-ups may appear when connecting the Windows Explorer and integration of OneDrive, when these ads come directly from Microsoft. This setting will block such advertisements.   <u>Note:</u> Disabling this setting will also disable all other notices from Microsoft OneDrive. We therefore recommend this setting with reservation.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::Machine;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSyncProviderNotifications", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "O003";
        t.category = L"Windows Explorer";
        t.title = L"Disable OneDrive access to network before login";
        t.description = L"OneDrive checks for updates or synchronizes files before users log in. You can use this setting to disable network access and prevent this from happening.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\OneDrive", L"PreventNetworkTrafficPreUserSignIn", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "O001";
        t.category = L"Windows Explorer";
        t.title = L"Disable Microsoft OneDrive";
        t.description = L"If you do not want to use Microsoft’s Cloud storage service OneDrive then you can deactivate it here.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Advanced;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive", L"DisableFileSyncNGSC", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

    // ==========================================
    // Category: Windows Update
    // ==========================================
    {
        Tweak t;
        t.id = "A004_USER";
        t.title = L"Disable automatic Windows Updates (User)";
        t.scope = TargetScope::User;
        t.category = L"Windows Update";
        t.description = L"Deactivating is not recommended! With this you deactivate the automatic installation of Windows Updates. Security leaks will not be tackled automatically.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"AllowClipboardHistory", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU", L"NoAutoUpdate", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "A004_MACHINE";
        t.title = L"Disable automatic Windows Updates (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Windows Update";
        t.description = L"Deactivating is not recommended! With this you deactivate the automatic installation of Windows Updates. Security leaks will not be tackled automatically.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Clipboard", L"EnableClipboardHistory", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SYSTEM\\CurrentControlSet\\Services\\wuauserv", L"Start", RegType::Dword, 0, 0, L"", L"", false });
        t.serviceActions.push_back({ L"wuauserv", 4, 2, true });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "A005";
        t.category = L"Windows Update";
        t.title = L"Disable Windows Updates for other products (e.g. Microsoft Office)";
        t.description = L"Deactivation is not recommended! The automatic update of many products, like e.g. Microsoft Office, is prevented by this.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"AllowCrossDeviceClipboard", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Services\\7971f918-a847-4430-9279-4a52d1efe18d", L"RegisteredWithAU", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P007_USER";
        t.title = L"Disable optional updates (including preview updates) (User)";
        t.scope = TargetScope::User;
        t.category = L"Windows Update";
        t.description = L"Windows 10 (from version 20H2 onward) allows optional updates, including preview updates, to be installed. These updates may include new features or fixes that have not yet been fully tested. Disable this setting to prevent optional updates from being installed automatically.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userAccountInformation", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"SetAllowOptionalContent", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"AllowOptionalContent", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P007_MACHINE";
        t.title = L"Disable optional updates (including preview updates) (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Windows Update";
        t.description = L"Windows 10 (from version 20H2 onward) allows optional updates, including preview updates, to be installed. These updates may include new features or fixes that have not yet been fully tested. Disable this setting to prevent optional updates from being installed automatically.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\{C1D23ACC-752B-43E5-8448-8D0E519CD6D6}", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userAccountInformation", L"Value", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "WU_DELIVERY_OPT_USER";
        t.title = L"Disable Windows Update via peer-to-peer (User)";
        t.scope = TargetScope::User;
        t.category = L"Windows Update";
        t.description = L"Windows Updates do not have to be downloaded only from Microsoft servers, but can also be downloaded from PCs in your network or the Internet. This often speeds up the process. It is a disadvantage that update data is sent from your computer too so that upload speeds can be reduced. If you don't want this, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config", L"DODownloadMode", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization", L"DODownloadMode", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "WU_DELIVERY_OPT_MACHINE";
        t.title = L"Disable Windows Update via peer-to-peer (Machine)";
        t.scope = TargetScope::Machine;
        t.category = L"Windows Update";
        t.description = L"Windows Updates do not have to be downloaded only from Microsoft servers, but can also be downloaded from PCs in your network or the Internet. This often speeds up the process. It is a disadvantage that update data is sent from your computer too so that upload speeds can be reduced. If you don't want this, then deactivate this function.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.regActions.push_back({ TargetScope::Machine, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization", L"DODownloadMode", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "W011";
        t.category = L"Windows Update";
        t.title = L"Disable updates to the speech recognition and speech synthesis modules";
        t.description = L"Windows periodically reviews the availability of new speech recognition and synthesis modules that are used for converting text to speech and vice versa. These are then downloaded automatically in the background. Disable this setting if you want to prevent this from happening.";
        t.impact = L"Disabling improves privacy with zero functional side effects.";
        t.safety = SafetyLevel::Safe;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Speech", L"AllowSpeechModelUpdate", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "W004";
        t.category = L"Windows Update";
        t.title = L"Activate deferring of upgrades";
        t.description = L"Upgrades (not security updates) can be pushed back for some months to install them at a date of your choosing.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"DeferUpgrade", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"DeferUpgradePeriod", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"DeferUpdatePeriod", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "W005";
        t.category = L"Windows Update";
        t.title = L"Disable automatic downloading manufacturers' apps and icons for devices";
        t.description = L"Many device manufacturers provide Windows with special programs that enable their devices to be used more easily or even used at all. This setting can prevent the downloading of such programs.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Device Metadata", L"PreventDeviceMetadataFromNetwork", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "W010";
        t.category = L"Windows Update";
        t.title = L"Disable automatic driver updates through Windows Update";
        t.description = L"Hardware drivers will be automatically updated with Windows Updates. Often, the drivers of the hardware producers are more current and more specific. Gamers may profit more from the use of hardware drivers. If you want to update hardware drivers yourself at your preferred time, then deactivate this function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", L"ExcludeWUDriversInQualityUpdate", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching", L"SearchOrderConfig", RegType::Dword, 0, 0, L"", L"", false });
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DriverSearching", L"SearchOrderConfig", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "W009";
        t.category = L"Windows Update";
        t.title = L"Disable automatic app updates through Windows Update";
        t.description = L"Apps will be automatically updated with Windows Updates. If you want to update the apps through Windows Store yourself at your preferred time, then deactivate this function.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsStore\\WindowsUpdate", L"AutoDownload", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }
    {
        Tweak t;
        t.id = "P017";
        t.category = L"Windows Update";
        t.title = L"Disable Windows dynamic configuration and update rollouts";
        t.description = L"Microsoft can change settings on the Windows system \"experimentally\" if there are problems with Windows updates. This is intended to test and/or verify certain configurations. And known issues can also be rolled back through updates (so-called Known Issues Rollbacks, KIR for short). If you do not want to participate in this procedure, then you should deactivate it. However, this can possibly lead to malfunctions of Windows and is therefore only recommended to a limited extent.";
        t.impact = L"Disabling may impact convenience or specific hardware features.";
        t.safety = SafetyLevel::Normal;
        t.scope = TargetScope::User;
        t.regActions.push_back({ TargetScope::User, L"SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\System", L"AllowExperimentation", RegType::Dword, 0, 0, L"", L"", false });
        AddTweak(std::move(t));
    }

}

SettingStatus TweakRegistry::AuditTweak(std::string_view id, UserSelectionMode mode, const std::vector<std::wstring>& users) const {
    const Tweak* t = GetTweakById(id);
    if (!t) return SettingStatus::NotApplicable;

    const size_t totalChecks = t->serviceActions.size() + t->regActions.size();
    if (totalChecks == 0) return SettingStatus::NotApplicable;

    size_t appliedCount = 0;
    size_t notAppliedCount = 0;
    size_t unknownCount = 0;
    size_t partialCount = 0;

    // Check services
    for (const auto& sa : t->serviceActions) {
        const SettingStatus st = ServiceHelper::AuditAction(sa);
        if (st == SettingStatus::Applied) appliedCount++;
        else if (st == SettingStatus::NotApplied) notAppliedCount++;
        else if (st == SettingStatus::Unknown) unknownCount++;
        else if (st == SettingStatus::Partial) partialCount++;
        else notAppliedCount++;
    }

    // Check registry actions
    for (const auto& ra : t->regActions) {
        SettingStatus st = SettingStatus::NotApplied;
        if (ra.scope == TargetScope::Machine) {
            st = RegistryHelper::AuditAction(HKEY_LOCAL_MACHINE, ra);
        } else {
            st = UserHiveManager::AuditUserAction(ra, mode, users);
        }

        if (st == SettingStatus::Applied) appliedCount++;
        else if (st == SettingStatus::NotApplied) notAppliedCount++;
        else if (st == SettingStatus::Unknown) unknownCount++;
        else if (st == SettingStatus::Partial) partialCount++;
        else notAppliedCount++;
    }

    if (unknownCount == totalChecks) return SettingStatus::Unknown;
    if (appliedCount == totalChecks) return SettingStatus::Applied;
    if (appliedCount > 0 || partialCount > 0) return SettingStatus::Partial;
    return SettingStatus::NotApplied;
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
