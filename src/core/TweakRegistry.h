#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_set>
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

    enum class TweakApplicability {
        Applicable,
        NotApplicableBuild,
        NotApplicableUserScope,
        NotApplicableService,
        UnknownTweak
    };

    enum class ApplyResult {
        Success,
        Failed,
        NotApplicable
    };

    struct ResolvedTweakActions {
        std::vector<RegistryAction> regActions;
        std::vector<ServiceAction> serviceActions;
        size_t missingServicesCount{ 0 };
        size_t inaccessibleServicesCount{ 0 };
        size_t skippedUserRegCount{ 0 };

        [[nodiscard]] bool HasApplicableActions() const noexcept {
            return !regActions.empty() || !serviceActions.empty();
        }
    };

    [[nodiscard]] ResolvedTweakActions GetResolvedActions(const Tweak& tweak, UserSelectionMode mode) const;
    [[nodiscard]] ResolvedTweakActions GetResolvedActions(std::string_view id, UserSelectionMode mode) const;

    [[nodiscard]] SettingStatus AuditTweak(std::string_view id, UserSelectionMode mode, const std::vector<std::wstring>& users) const;
    [[nodiscard]] bool MatchesTargetState(std::string_view id, bool targetProtected, UserSelectionMode mode = UserSelectionMode::CurrentUser, const std::vector<std::wstring>& users = {}) const;
    [[nodiscard]] std::string GetCanonicalTweakId(std::string_view id) const;
    [[nodiscard]] TweakApplicability GetTweakApplicability(std::string_view id, UserSelectionMode mode) const;
    [[nodiscard]] bool ValidatePlanConflicts(const std::map<std::string, bool>& tweakRequests, std::string& outErrorMessage) const;
    [[nodiscard]] bool ValidatePendingPlan(const std::unordered_set<std::string>& enables, const std::unordered_set<std::string>& reverts, std::string& outErrorMessage) const;
    [[nodiscard]] bool ValidatePendingPlan(const std::vector<std::string>& enables, const std::vector<std::string>& reverts, std::string& outErrorMessage) const;

    [[nodiscard]] ApplyResult ApplyTweakEx(std::string_view id, bool enableProtection, UserSelectionMode mode, const std::vector<std::wstring>& users);
    bool ApplyTweak(std::string_view id, bool enableProtection, UserSelectionMode mode, const std::vector<std::wstring>& users);
    void AddTweak(Tweak tweak);

private:
    TweakRegistry();
    std::vector<Tweak> m_tweaks;
    std::map<std::string, size_t> m_idIndexMap;
    std::map<std::string, std::string> m_aliasMap;
};

} // namespace PrivatizeWin
