#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include "Types.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]
// C.45: In-class initializers

class TweakRegistry {
public:
    static TweakRegistry& Instance();

    void InitializeDefaultTweaks();
    bool LoadExternalTweaks(std::string_view jsonContent);

    [[nodiscard]] const std::vector<Tweak>& GetAllTweaks() const noexcept { return m_tweaks; }
    [[nodiscard]] const Tweak* GetTweakById(std::string_view id) const;
    [[nodiscard]] std::vector<Tweak> GetTweaksByCategory(std::wstring_view category) const;
    [[nodiscard]] std::vector<CategoryInfo> GetCategories() const;

    [[nodiscard]] SettingStatus AuditTweak(std::string_view id, UserSelectionMode mode, const std::vector<std::wstring>& users) const;
    bool ApplyTweak(std::string_view id, bool enableProtection, UserSelectionMode mode, const std::vector<std::wstring>& users);

private:
    TweakRegistry() = default;
    std::vector<Tweak> m_tweaks;
    std::map<std::string, size_t> m_idIndexMap;

    void AddTweak(Tweak tweak);
};

} // namespace PrivatizeWin
