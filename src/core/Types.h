#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace PrivatizeWin {

// Functional impact level of the tweak (Item 5)
enum class ImpactLevel {
    Low = 0,        // Minimal or no side effects on user experience
    Moderate = 1,   // Mild impact (e.g. disables location or Bing in Start)
    High = 2,       // Potential functional loss (e.g. biometric, camera access)
    // Aliases for backwards compatibility
    Safe = Low,
    Normal = Moderate,
    Advanced = High
};
using SafetyLevel = ImpactLevel;

// Current audit state on the machine (Item 4)
enum class SettingStatus {
    Applied,        // Setting active / applied on the machine (Already Enabled)
    NotApplied,     // Setting not applied (Windows default)
    Partial,        // Partially applied across different keys or services (Partially Enabled)
    Unknown,        // Inaccessible or detection failed
    NotApplicable,  // Not applicable to this Windows version
    // Aliases for backwards compatibility
    Protected = Applied,
    Default = NotApplied,
    Mixed = Partial,
    NotSupported = NotApplicable
};


// Target registry or service scope
enum class TargetScope {
    Machine,     // HKEY_LOCAL_MACHINE
    User,        // HKEY_CURRENT_USER / All Users
    Service      // Windows Service controller
};


// Value type for registry
enum class RegType {
    Dword,
    String
};

struct RegistryAction {
    TargetScope scope{ TargetScope::Machine };
    std::wstring subKey;
    std::wstring valueName;
    RegType type{ RegType::Dword };
    uint32_t dwordProtected{ 1 };
    uint32_t dwordDefault{ 0 };
    std::wstring strProtected;
    std::wstring strDefault;
    bool deleteOnDefault{ false };
};

struct ServiceAction {
    std::wstring serviceName;
    uint32_t startupTypeProtected{ 4 }; // 4 = SERVICE_DISABLED
    uint32_t startupTypeDefault{ 2 };   // 2 = SERVICE_AUTO_START, 3 = SERVICE_DEMAND_START
    bool stopService{ true };
};

struct Tweak {
    std::string id;
    std::wstring category;
    std::wstring title;
    std::wstring description;
    std::wstring impact;           // Specific consequences and tradeoff
    ImpactLevel impactLevel{ ImpactLevel::Low };
    bool isRecommended{ true };    // Recommendation membership separate from impact
    bool requiresReboot{ false };
    bool requiresSignOut{ false };
    std::wstring affectsFeatures;
    TargetScope scope{ TargetScope::Machine };
    std::vector<RegistryAction> regActions;
    std::vector<ServiceAction> serviceActions;
    std::string minWindowsBuild; // e.g. "22000" for Win11

    // Compatibility alias field
    ImpactLevel safety{ ImpactLevel::Low };
};

struct CategoryInfo {
    std::wstring name;
    int totalCount{ 0 };
    int appliedCount{ 0 };
    int protectedCount{ 0 }; // compatibility
};

struct TemplateProfile {
    std::string name;
    std::string description;
    bool isBuiltin{ false };
    std::map<std::string, bool> tweakStates; // tweakId -> true (enable tweak) / false (leave/revert)
};

enum class UserSelectionMode {
    AllUsers,       // Default: iterate all profiles in HKEY_USERS + Default profile
    CurrentUser,    // Only the caller's HKCU
    NoUsers,        // Only HKLM tweaks
    SpecificUsers   // Custom list of usernames / SIDs
};

struct CliOptions {
    bool isCli{ false };
    std::string applyTemplate;
    std::wstring applyTemplateW;
    bool listTemplates{ false };
    bool revertAll{ false };
    bool showStatus{ false };
    std::string statusFormat{ "text" }; // text or json
    
    // Scheduled task flags
    bool installTask{ false };
    std::string taskFrequency{ "daily" }; // daily, logon, weekly
    std::string taskTemplate{ "recommended" };
    std::wstring taskTemplateW;
    bool uninstallTask{ false };
    
    // Safety & User flags
    bool dryRun{ false };
    bool quiet{ false };
    bool forceRestorePoint{ false };
    bool noRestorePoint{ false };
    
    UserSelectionMode userMode{ UserSelectionMode::AllUsers };
    std::vector<std::wstring> specificUsernames;

    std::wstring resumePendingFile;

    bool hasError{ false };
    std::string errorMessage;
};

} // namespace PrivatizeWin
