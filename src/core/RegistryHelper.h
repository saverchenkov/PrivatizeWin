#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include "Types.h"
#include "SmartHandle.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// I.2: Avoid non-const global variables
// F.16: For "in" parameters, pass string types by string_view
// I.10: Use [[nodiscard]] for values that should not be ignored
// R.1: RAII resource management

class RegistryHelper {
public:
    [[nodiscard]] static bool KeyExists(HKEY hRoot, std::wstring_view subKey) noexcept;
    [[nodiscard]] static bool ValueExists(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName) noexcept;
    
    [[nodiscard]] static std::optional<uint32_t> ReadDword(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName);
    [[nodiscard]] static std::optional<std::wstring> ReadString(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName);
    
    static bool WriteDword(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName, uint32_t value);
    static bool WriteString(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName, std::wstring_view value);
    
    static bool DeleteValue(HKEY hRoot, std::wstring_view subKey, std::wstring_view valueName);
    static bool DeleteKeyIfEmpty(HKEY hRoot, std::wstring_view subKey);

    [[nodiscard]] static SettingStatus AuditAction(HKEY hRoot, const RegistryAction& action);
    [[nodiscard]] static bool MatchesTarget(HKEY hRoot, const RegistryAction& action, bool targetProtected) noexcept;
    static bool ApplyAction(HKEY hRoot, const RegistryAction& action, bool enableProtection);
};

} // namespace PrivatizeWin
