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

UserTargetResult UserHiveManager::ForEachTargetUser(
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers,
    const std::function<bool(HKEY hUserRoot, const UserProfile& profile)>& callback
) {
    UserTargetResult result{};

    if (mode == UserSelectionMode::NoUsers) {
        result.allSucceeded = true;
        return result;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        result.requestedUsers = 1;
        result.resolvedUsers = 1;
        result.mountedUsers = 1;
        UserProfile curProfile;
        curProfile.username = L"Current User";
        curProfile.sid = L"CURRENT";
        curProfile.isLoaded = true;
        if (!callback(HKEY_CURRENT_USER, curProfile)) {
            result.failedUsers++;
            result.allSucceeded = false;
        }
        return result;
    }

    EnablePrivilege(L"SeBackupPrivilege");
    EnablePrivilege(L"SeRestorePrivilege");

    const auto profiles = DiscoverUserProfiles();

    if (mode == UserSelectionMode::SpecificUsers) {
        result.requestedUsers = specificUsers.size();
        if (specificUsers.empty()) {
            result.allSucceeded = false;
            return result;
        }

        for (const auto& filter : specificUsers) {
            auto it = std::find_if(profiles.begin(), profiles.end(), [&](const UserProfile& p) {
                return (_wcsicmp(p.username.c_str(), filter.c_str()) == 0 || _wcsicmp(p.sid.c_str(), filter.c_str()) == 0);
            });

            if (it == profiles.end()) {
                // Requested user does not exist on this machine
                result.failedUsers++;
                result.allSucceeded = false;
                continue;
            }

            result.resolvedUsers++;
            const UserProfile& p = *it;

            if (p.isLoaded) {
                UniqueHKey hUserRoot;
                if (RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                    RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                    result.mountedUsers++;
                    if (!callback(hUserRoot.get(), p)) {
                        result.failedUsers++;
                        result.allSucceeded = false;
                    }
                } else {
                    result.failedUsers++;
                    result.allSucceeded = false;
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
                    bool cbOk = true;
                    {
                        UniqueHKey hUserRoot;
                        if (RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                            RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                            result.mountedUsers++;
                            cbOk = callback(hUserRoot.get(), p);
                        } else {
                            cbOk = false;
                        }
                    }

                    if (!wasAlreadyMounted) {
                        for (int retry = 0; retry < 5; ++retry) {
                            if (RegUnLoadKeyW(HKEY_USERS, mountName.c_str()) == ERROR_SUCCESS) {
                                break;
                            }
                            Sleep(25);
                        }
                    }

                    if (!cbOk) {
                        result.failedUsers++;
                        result.allSucceeded = false;
                    }
                } else {
                    result.failedUsers++;
                    result.allSucceeded = false;
                }
            }
        }
        return result;
    }

    if (mode == UserSelectionMode::AllUsers) {
        wchar_t sysDrive[MAX_PATH] = { 0 };
        GetEnvironmentVariableW(L"SystemDrive", sysDrive, MAX_PATH);
        const std::wstring defaultNtuser = std::wstring(sysDrive) + L"\\Users\\Default\\NTUSER.DAT";
        const bool defaultExists = (GetFileAttributesW(defaultNtuser.c_str()) != INVALID_FILE_ATTRIBUTES);

        result.requestedUsers = profiles.size() + (defaultExists ? 1 : 0);

        for (const auto& p : profiles) {
            result.resolvedUsers++;
            if (p.isLoaded) {
                UniqueHKey hUserRoot;
                if (RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                    RegOpenKeyExW(HKEY_USERS, p.sid.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                    result.mountedUsers++;
                    if (!callback(hUserRoot.get(), p)) {
                        result.failedUsers++;
                        result.allSucceeded = false;
                    }
                } else {
                    result.failedUsers++;
                    result.allSucceeded = false;
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
                    bool cbOk = true;
                    {
                        UniqueHKey hUserRoot;
                        if (RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                            RegOpenKeyExW(HKEY_USERS, mountName.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                            result.mountedUsers++;
                            cbOk = callback(hUserRoot.get(), p);
                        } else {
                            cbOk = false;
                        }
                    }

                    if (!wasAlreadyMounted) {
                        for (int retry = 0; retry < 5; ++retry) {
                            if (RegUnLoadKeyW(HKEY_USERS, mountName.c_str()) == ERROR_SUCCESS) {
                                break;
                            }
                            Sleep(25);
                        }
                    }

                    if (!cbOk) {
                        result.failedUsers++;
                        result.allSucceeded = false;
                    }
                } else {
                    result.failedUsers++;
                    result.allSucceeded = false;
                }
            }
        }

        if (defaultExists) {
            result.resolvedUsers++;
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
                bool cbOk = true;
                {
                    UniqueHKey hUserRoot;
                    if (RegOpenKeyExW(HKEY_USERS, defaultMount.c_str(), 0, KEY_READ | KEY_WRITE, hUserRoot.put()) == ERROR_SUCCESS ||
                        RegOpenKeyExW(HKEY_USERS, defaultMount.c_str(), 0, KEY_READ, hUserRoot.put()) == ERROR_SUCCESS) {
                        result.mountedUsers++;
                        UserProfile defaultProfile;
                        defaultProfile.username = L"Default User Template";
                        defaultProfile.sid = L"DEFAULT_TEMPLATE";
                        defaultProfile.isLoaded = false;
                        cbOk = callback(hUserRoot.get(), defaultProfile);
                    } else {
                        cbOk = false;
                    }
                }

                if (!wasAlreadyMounted) {
                    for (int retry = 0; retry < 5; ++retry) {
                        if (RegUnLoadKeyW(HKEY_USERS, defaultMount.c_str()) == ERROR_SUCCESS) {
                            break;
                        }
                        Sleep(25);
                    }
                }

                if (!cbOk) {
                    result.failedUsers++;
                    result.allSucceeded = false;
                }
            } else {
                result.failedUsers++;
                result.allSucceeded = false;
            }
        }

        return result;
    }

    return result;
}

SettingStatus UserHiveManager::AuditUserAction(
    const RegistryAction& action,
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers
) {
    if (mode == UserSelectionMode::NoUsers) {
        return SettingStatus::Applied;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        return RegistryHelper::AuditAction(HKEY_CURRENT_USER, action);
    }

    size_t appliedUsers = 0;
    size_t defaultUsers = 0;
    size_t customUsers = 0;
    size_t unknownUsers = 0;

    const auto res = ForEachTargetUser(mode, specificUsers, [&](HKEY hUserRoot, const UserProfile&) -> bool {
        const SettingStatus status = RegistryHelper::AuditAction(hUserRoot, action);
        if (status == SettingStatus::Applied) {
            appliedUsers++;
        } else if (status == SettingStatus::Default || status == SettingStatus::NotApplied) {
            defaultUsers++;
        } else if (status == SettingStatus::Custom) {
            customUsers++;
        } else {
            unknownUsers++;
        }
        return true;
    });

    unknownUsers += res.failedUsers;

    if (res.requestedUsers == 0) {
        return SettingStatus::Unknown;
    }

    if (appliedUsers == res.requestedUsers && unknownUsers == 0) {
        return SettingStatus::Applied;
    }

    if (defaultUsers == res.requestedUsers && unknownUsers == 0) {
        return SettingStatus::Default;
    }

    if (appliedUsers > 0 && defaultUsers > 0) {
        return SettingStatus::Partial;
    }

    if (appliedUsers > 0) {
        return SettingStatus::Partial;
    }

    if (customUsers > 0) {
        return SettingStatus::Custom;
    }

    if (unknownUsers > 0) {
        return SettingStatus::Unknown;
    }

    return SettingStatus::Default;
}

bool UserHiveManager::MatchesUserActionTarget(
    const RegistryAction& action,
    bool targetProtected,
    UserSelectionMode mode,
    const std::vector<std::wstring>& specificUsers
) {
    if (mode == UserSelectionMode::NoUsers) {
        return true;
    }

    if (mode == UserSelectionMode::CurrentUser) {
        return RegistryHelper::MatchesTarget(HKEY_CURRENT_USER, action, targetProtected);
    }

    bool allMatch = true;
    const auto res = ForEachTargetUser(mode, specificUsers, [&](HKEY hUserRoot, const UserProfile&) -> bool {
        if (!RegistryHelper::MatchesTarget(hUserRoot, action, targetProtected)) {
            allMatch = false;
        }
        return true;
    });

    if (res.requestedUsers == 0 || res.failedUsers > 0 || res.mountedUsers == 0) {
        return false;
    }

    return allMatch;
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

    const auto res = ForEachTargetUser(mode, specificUsers, [&](HKEY hUserRoot, const UserProfile&) -> bool {
        return RegistryHelper::ApplyAction(hUserRoot, action, enableProtection);
    });

    return res.allSucceeded && (res.failedUsers == 0) && (res.mountedUsers > 0);
}

} // namespace PrivatizeWin
