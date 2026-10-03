#include "RegistryHelper.h"
#include <vector>

namespace PrivatizeWin {

static std::wstring ToNullTerminated(std::wstring_view sv) {
    return std::wstring(sv);
}

bool RegistryHelper::KeyExists(HKEY hRoot, std::wstring_view subKey) noexcept {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    const LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    return (status == ERROR_SUCCESS);
}

bool RegistryHelper::ValueExists(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName) noexcept {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);

    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    if (status != ERROR_SUCCESS) {
        return false;
    }

    status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, nullptr, nullptr, nullptr);
    return (status == ERROR_SUCCESS);
}

std::optional<uint32_t> RegistryHelper::ReadDword(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName) {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);

    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD dwType = 0;
    DWORD dwData = 0;
    DWORD cbData = sizeof(dwData);
    status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwData), &cbData);

    if (status == ERROR_SUCCESS && dwType == REG_DWORD) {
        return static_cast<uint32_t>(dwData);
    }
    return std::nullopt;
}

std::optional<std::wstring> RegistryHelper::ReadString(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName) {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);

    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD dwType = 0;
    DWORD cbData = 0;
    status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, nullptr, &cbData);
    if (status != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_EXPAND_SZ)) {
        return std::nullopt;
    }

    std::vector<wchar_t> buffer(cbData / sizeof(wchar_t) + 1, 0);
    status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(buffer.data()), &cbData);

    if (status == ERROR_SUCCESS) {
        return std::wstring(buffer.data());
    }
    return std::nullopt;
}

bool RegistryHelper::WriteDword(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName, uint32_t value) {
    UniqueHKey key;
    DWORD disposition = 0;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);

    LSTATUS status = RegCreateKeyExW(hRoot, subKeyStr.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, key.put(), &disposition);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    const DWORD dwValue = static_cast<DWORD>(value);
    status = RegSetValueExW(key.get(), valNameStr.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(dwValue));
    return (status == ERROR_SUCCESS);
}

bool RegistryHelper::WriteString(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName, std::wstring_view value) {
    UniqueHKey key;
    DWORD disposition = 0;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);
    std::wstring valStr = ToNullTerminated(value);

    LSTATUS status = RegCreateKeyExW(hRoot, subKeyStr.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, key.put(), &disposition);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    const DWORD cbData = static_cast<DWORD>((valStr.length() + 1) * sizeof(wchar_t));
    status = RegSetValueExW(key.get(), valNameStr.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(valStr.c_str()), cbData);
    return (status == ERROR_SUCCESS);
}

bool RegistryHelper::DeleteValue(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName) {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(subKey);
    std::wstring valNameStr = ToNullTerminated(valueName);

    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_SET_VALUE, key.put());
    if (status != ERROR_SUCCESS) {
        return (status == ERROR_FILE_NOT_FOUND);
    }

    status = RegDeleteValueW(key.get(), valNameStr.c_str());
    return (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
}

bool RegistryHelper::DeleteKeyIfEmpty(HKEY hRoot, std::wstring_view subKey) {
    std::wstring subKeyStr = ToNullTerminated(subKey);
    UniqueHKey key;
    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    if (status != ERROR_SUCCESS) return true;

    DWORD subKeys = 0;
    DWORD values = 0;
    RegQueryInfoKeyW(key.get(), nullptr, nullptr, nullptr, &subKeys, nullptr, nullptr, &values, nullptr, nullptr, nullptr, nullptr);
    key.reset();

    if (subKeys == 0 && values == 0) {
        return (RegDeleteKeyW(hRoot, subKeyStr.c_str()) == ERROR_SUCCESS);
    }
    return false;
}

SettingStatus RegistryHelper::AuditAction(HKEY hRoot, const RegistryAction& action) {
    UniqueHKey key;
    std::wstring subKeyStr = ToNullTerminated(action.subKey);
    std::wstring valNameStr = ToNullTerminated(action.valueName);

    LSTATUS status = RegOpenKeyExW(hRoot, subKeyStr.c_str(), 0, KEY_READ, key.put());
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_ACCESS_DENIED) {
            return SettingStatus::Unknown;
        }
        if (status == ERROR_FILE_NOT_FOUND) {
            return action.deleteOnDefault ? SettingStatus::Default : SettingStatus::NotApplied;
        }
        return SettingStatus::Unknown;
    }

    DWORD dwType = 0;
    if (action.type == RegType::Dword) {
        DWORD dwData = 0;
        DWORD cbData = sizeof(dwData);
        status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwData), &cbData);
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_ACCESS_DENIED) return SettingStatus::Unknown;
            if (status == ERROR_FILE_NOT_FOUND) return action.deleteOnDefault ? SettingStatus::Default : SettingStatus::NotApplied;
            return SettingStatus::Unknown;
        }
        if (dwType != REG_DWORD) {
            return SettingStatus::NotApplied;
        }
        if (dwData == action.dwordProtected) {
            return SettingStatus::Protected;
        }
        if (!action.deleteOnDefault && dwData == action.dwordDefault) {
            return SettingStatus::Default;
        }
        return action.deleteOnDefault ? SettingStatus::NotApplied : SettingStatus::Default;
    } else {
        DWORD cbData = 0;
        status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, nullptr, &cbData);
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_ACCESS_DENIED) return SettingStatus::Unknown;
            if (status == ERROR_FILE_NOT_FOUND) return action.deleteOnDefault ? SettingStatus::Default : SettingStatus::NotApplied;
            return SettingStatus::Unknown;
        }
        if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) {
            return SettingStatus::NotApplied;
        }
        std::vector<wchar_t> strBuf(cbData / sizeof(wchar_t) + 1, 0);
        status = RegQueryValueExW(key.get(), valNameStr.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(strBuf.data()), &cbData);
        if (status == ERROR_SUCCESS) {
            std::wstring strVal(strBuf.data());
            if (strVal == action.strProtected) {
                return SettingStatus::Protected;
            }
            if (!action.deleteOnDefault && strVal == action.strDefault) {
                return SettingStatus::Default;
            }
        }
        return action.deleteOnDefault ? SettingStatus::NotApplied : SettingStatus::Default;
    }
}

bool RegistryHelper::ApplyAction(HKEY hRoot, const RegistryAction& action, bool enableProtection) {
    if (enableProtection) {
        if (action.type == RegType::Dword) {
            return WriteDword(hRoot, action.subKey, action.valueName, action.dwordProtected);
        } else {
            return WriteString(hRoot, action.subKey, action.valueName, action.strProtected);
        }
    } else {
        if (action.deleteOnDefault) {
            const bool deleted = DeleteValue(hRoot, action.subKey, action.valueName);
            if (deleted) {
                DeleteKeyIfEmpty(hRoot, action.subKey);
            }
            return deleted;
        } else {
            if (action.type == RegType::Dword) {
                return WriteDword(hRoot, action.subKey, action.valueName, action.dwordDefault);
            } else {
                return WriteString(hRoot, action.subKey, action.valueName, action.strDefault);
            }
        }
    }
}

} // namespace PrivatizeWin
