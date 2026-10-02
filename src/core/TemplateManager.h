#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include "Types.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]
// R.1: RAII resource management

class TemplateManager {
public:
    static TemplateManager& Instance();

    void InitializeBuiltinTemplates();

    [[nodiscard]] std::vector<std::string> GetAvailableTemplateNames() const;
    [[nodiscard]] std::optional<TemplateProfile> GetTemplate(std::string_view name) const;

    bool LoadTemplateFromFile(std::wstring_view filePath, TemplateProfile& outProfile, std::string* outError = nullptr);
    bool SaveTemplateToFile(std::wstring_view filePath, const TemplateProfile& profile);

    bool ApplyTemplate(
        const TemplateProfile& profile,
        UserSelectionMode mode,
        const std::vector<std::wstring>& users,
        bool dryRun = false
    );

private:
    TemplateManager() = default;
    std::map<std::string, TemplateProfile> m_templates;
};

} // namespace PrivatizeWin
