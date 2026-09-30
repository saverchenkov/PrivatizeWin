#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace PrivatizeWin {

// Recommendation level for user safety
enum class SafetyLevel {
    Safe,        // Recommended for everyone, no functional side effects (green)
    Normal,      // Mild impact (e.g. disables location or Bing in Start) (yellow)
    Advanced     // Potential functional loss (e.g. disabling biometric, camera access) (red)
};

// Current audit state on the machine
enum class SettingStatus {
    Protected,   // Telemetry disabled / Privacy active
    Default,     // Default Windows behavior (telemetry enabled)
    Mixed,       // Partially applied across different keys or users
    NotSupported // Not applicable to this Windows version
};

// Target registry or service scope
enum class TargetScope {
    Machine,     // HKEY_LOCAL_MACHINE
    User,        // HKEY_CURRENT_USER / All Users
    Both,        // Both HKLM and HKCU
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
    std::wstring impact;
    SafetyLevel safety{ SafetyLevel::Safe };
    TargetScope scope{ TargetScope::Machine };
    std::vector<RegistryAction> regActions;
    std::vector<ServiceAction> serviceActions;
    std::string minWindowsBuild; // e.g. "22000" for Win11
};

struct CategoryInfo {
    std::wstring name;
    int totalCount{ 0 };
    int protectedCount{ 0 };
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
    bool listTemplates{ false };
    bool revertAll{ false };
    bool showStatus{ false };
    std::string statusFormat{ "text" }; // text or json
    
    // Scheduled task flags
    bool installTask{ false };
    std::string taskFrequency{ "daily" }; // daily, logon, weekly
    std::string taskTemplate{ "recommended" };
    bool uninstallTask{ false };
    
    // Safety & User flags
    bool dryRun{ false };
    bool quiet{ false };
    bool forceRestorePoint{ false };
    bool noRestorePoint{ false };
    
    UserSelectionMode userMode{ UserSelectionMode::AllUsers };
    std::vector<std::wstring> specificUsernames;
};

} // namespace PrivatizeWin
