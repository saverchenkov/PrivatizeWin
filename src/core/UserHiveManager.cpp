#include "UserHiveManager.h"
#include "RegistryHelper.h"
#include <sddl.h>
#include <iostream>
#include <algorithm>

namespace PrivatizeWin {

bool UserHiveManager::EnablePrivilege(std::wstring_view privilegeName) {
    UniqueHandle hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, hToken.put())) {
        return false;
    }

    std::wstring privStr(privilegeName);
    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, privStr.c_str(), &luid)) {
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    const BOOL ok = AdjustTokenPrivileges(hToken.get(), FALSE, &tp, sizeof(tp), nullptr, nullptr);
    return (ok && GetLastError() == ERROR_SUCCESS);
}

std::vector<UserProfile> UserHiveManager::DiscoverUserProfiles() {
    std::vector<UserProfile> profiles;
    const std::wstring profileListKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";

    UniqueHKey hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, profileListKey.c_str(), 0, KEY_READ, hKey.put()) != ERROR_SUCCESS) {
        return profiles;
    }

    DWORD subKeys = 0;
    DWORD maxSubKeyLen = 0;
    RegQueryInfoKeyW(hKey.get(), nullptr, nullptr, nullptr, &subKeys, &maxSubKeyLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    std::vector<wchar_t> keyNameBuffer(maxSubKeyLen + 1, 0);

    for (DWORD i = 0; i < subKeys; ++i) {
        DWORD nameLen = static_cast<DWORD>(keyNameBuffer.size());
        if (RegEnumKeyExW(hKey.get(), i, keyNameBuffer.data(), &nameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            std::wstring sid(keyNameBuffer.data(), nameLen);

            if (sid.rfind(L"S-1-5-21-", 0) != 0) {
                continue;
            }

            const std::wstring subKeyPath = profileListKey + L"\\" + sid;
            const auto profilePathOpt = RegistryHelper::ReadString(HKEY_LOCAL_MACHINE, subKeyPath, L"ProfileImagePath");
            if (!profilePathOpt.has_value()) continue;

            UserProfile up;
            up.sid = sid;
            up.profilePath = profilePathOpt.value();

            const size_t lastSlash = up.profilePath.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                up.username = up.profilePath.substr(lastSlash + 1);
            } else {
                up.username = sid;
            }

            up.isLoaded = RegistryHelper::KeyExists(HKEY_USERS, sid);
            profiles.push_back(std::move(up));
        }
    }

    return profiles;
}

void UserHiveManager::ForEachTargetUser(
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers,
    const std::function<void(HKEY hUserRoot, const UserProfile& profile)>& callback
) {
    if (mode == UserSelectionMode::NoUsers) {
        return;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        UserProfile curProfile;
        curProfile.username = L"Current User";
        curProfile.sid = L"CURRENT";
        curProfile.isLoaded = true;
        callback(HKEY_CURRENT_USER, curProfile);
        return;
    }

    EnablePrivilege(L"SeBackupPrivilege");
    EnablePrivilege(L"SeRestorePrivilege");

    const auto profiles = DiscoverUserProfiles();

    for (const auto& p : profiles) {
        if (mode == UserSelectionMode::SpecificUsers) {
            bool matched = false;
            for (const auto& filter : specificUsers) {
                if (_wcsicmp(p.username.c_str(), filter.c_str()) == 0 || _wcsicmp(p.sid.c_str(), filter.c_str()) == 0) {
                    matched = true;
                    break;
                }
            }
            if (!matched) continue;
        }

        if (p.isLoaded) {
            UniqueHKey hUserRoot;
            if (RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                callback(hUserRoot.get(), p);
            }
        } else {
            const std::wstring ntuserPath = p.profilePath + L"\\NTUSER.DAT";
            const std::wstring mountName = L"PrivatizeWin_" + p.sid;

            LSTATUS loadStatus = RegLoadKeyW(HKEY_USERS, mountName.c_str(), ntuserPath.c_str());
            bool wasAlreadyMounted = false;
            if (loadStatus != ERROR_SUCCESS) {
                UniqueHKey testKey;
                if (RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ, testKey.put()) == ERROR_SUCCESS) {
                    wasAlreadyMounted = true;
                    loadStatus = ERROR_SUCCESS;
                }
            }

            if (loadStatus == ERROR_SUCCESS) {
                {
                    UniqueHKey hUserRoot;
                    if (RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                        RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                        callback(hUserRoot.get(), p);
                    }
                }
                if (!wasAlreadyMounted) {
                    for (int retry = 0; retry < 3; ++retry) {
                        if (RegUnLoadKeyW(HKEY_USERS, mountName.c_str()) == ERROR_SUCCESS) {
                            break;
                        }
                        Sleep(10);
                    }
                }
            }
        }
    }

    if (mode == UserSelectionMode::AllUsers) {
        wchar_t sysDrive[MAX_PATH] = { 0 };
        GetEnvironmentVariableW(L"SystemDrive", sysDrive, MAX_PATH);
        const std::wstring defaultNtuser = std::wstring(sysDrive) + L"\\Users\\Default\\NTUSER.DAT";
        const std::wstring defaultMount = L"PrivatizeWin_DefaultUser";

        LSTATUS loadStatus = RegLoadKeyW(HKEY_USERS, defaultMount.c_str(), defaultNtuser.c_str());
        bool wasAlreadyMounted = false;
        if (loadStatus != ERROR_SUCCESS) {
            UniqueHKey testKey;
            if (RegOpenKeyExW(HKEY_USERS, defaultMount.c_str(), 0, KEY_READ, testKey.put()) == ERROR_SUCCESS) {
                wasAlreadyMounted = true;
                loadStatus = ERROR_SUCCESS;
            }
        }

        if (loadStatus == ERROR_SUCCESS) {
            {
                UniqueHKey hUserRoot;
                if (RegOpenKeyExW(HKEY_USERS, defaultMount.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                    RegOpenKeyExW(HKEY_USERS, defaultMount.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                    UserProfile defaultProfile;
                    defaultProfile.username = L"Default User Template";
                    defaultProfile.sid = L"DEFAULT_TEMPLATE";
                    defaultProfile.isLoaded = false;
                    callback(hUserRoot.get(), defaultProfile);
                }
            }
            if (!wasAlreadyMounted) {
                for (int retry = 0; retry < 3; ++retry) {
                    if (RegUnLoadKeyW(HKEY_USERS, defaultMount.c_str()) == ERROR_SUCCESS) {
                        break;
                    }
                    Sleep(10);
                }
            }
        }
    }
}

SettingStatus UserHiveManager::AuditUserAction(
    const RegistryAction& action,
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers
) {
    if (mode == UserSelectionMode::NoUsers) {
        return SettingStatus::Protected;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        return RegistryHelper::AuditAction(HKEY_CURRENT_USER, action);
    }

    int totalUsers = 0;
    int protectedUsers = 0;

    ForEachTargetUser(mode, specificUsers, [&](HKEY hUserRoot, const UserProfile&) {
        totalUsers++;
        const SettingStatus status = RegistryHelper::AuditAction(hUserRoot, action);
        if (status == SettingStatus::Protected) {
            protectedUsers++;
        }
    });

    if (totalUsers == 0) {
        return RegistryHelper::AuditAction(HKEY_CURRENT_USER, action);
    }
    if (protectedUsers == totalUsers) {
        return SettingStatus::Applied;
    }
    return SettingStatus::NotApplied;
}


bool UserHiveManager::ApplyUserAction(
    const RegistryAction& action,
    bool enableProtection,
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers
) {
    if (mode == UserSelectionMode::NoUsers) {
        return true;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        return RegistryHelper::ApplyAction(HKEY_CURRENT_USER, action, enableProtection);
    }

    bool allSucceeded = true;
    ForEachTargetUser(mode, specificUsers, [&](HKEY hUserRoot, const UserProfile&) {
        const bool ok = RegistryHelper::ApplyAction(hUserRoot, action, enableProtection);
        if (!ok) allSucceeded = false;
    });

    return allSucceeded;
}

} // namespace PrivatizeWin
