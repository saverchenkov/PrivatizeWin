#include "TemplateManager.h"
#include "TweakRegistry.h"
#include "SimpleJson.h"
#include <fstream>
#include <sstream>

namespace PrivatizeWin {

TemplateManager& TemplateManager::Instance() {
    static TemplateManager manager;
    return manager;
}

void TemplateManager::InitializeBuiltinTemplates() {
    m_templates.clear();
    const auto& allTweaks = TweakRegistry::Instance().GetAllTweaks();

    // 1. Recommended (Safe)
    {
        TemplateProfile p;
        p.name = "recommended";
        p.description = "Enables all safe privacy settings. No functional side effects or broken apps.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            p.tweakStates[t.id] = (t.safety == SafetyLevel::Safe);
        }
        m_templates[p.name] = p;
    }

    // 2. Strict (Safe + Normal)
    {
        TemplateProfile p;
        p.name = "strict";
        p.description = "Maximum privacy posture: disables all telemetry, Bing web search, AI recall, and edge tracking.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            p.tweakStates[t.id] = (t.safety == SafetyLevel::Safe || t.safety == SafetyLevel::Normal);
        }
        m_templates[p.name] = p;
    }

    // 3. Minimal (Core Telemetry Only)
    {
        TemplateProfile p;
        p.name = "minimal";
        p.description = "Disables only low-level OS telemetry services and diagnostic data collection.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            p.tweakStates[t.id] = (t.id.rfind("TEL_", 0) == 0);
        }
        m_templates[p.name] = p;
    }

    // 4. Defaults / Revert
    {
        TemplateProfile p;
        p.name = "defaults";
        p.description = "Reverts all settings to Windows out-of-the-box defaults.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            p.tweakStates[t.id] = false;
        }
        m_templates[p.name] = p;
        m_templates["revert"] = p;
    }
}

std::vector<std::string> TemplateManager::GetAvailableTemplateNames() const {
    std::vector<std::string> names;
    names.reserve(m_templates.size());
    for (const auto& [name, _] : m_templates) {
        names.push_back(name);
    }
    return names;
}

std::optional<TemplateProfile> TemplateManager::GetTemplate(std::string_view name) const {
    auto it = m_templates.find(std::string(name));
    if (it != m_templates.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool TemplateManager::LoadTemplateFromFile(std::wstring_view filePath, TemplateProfile& outProfile) {
    std::wstring pathStr(filePath);
    std::ifstream file(pathStr);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    try {
        JsonValue root = JsonValue::parse(buffer.str());
        if (!root.isObject()) return false;

        outProfile.name = root["name"].stringValue;
        outProfile.description = root["description"].stringValue;
        outProfile.isBuiltin = false;
        outProfile.tweakStates.clear();

        if (root["tweaks"].isObject()) {
            for (const auto& [id, val] : root["tweaks"].objectValue) {
                outProfile.tweakStates[id] = val.boolValue;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool TemplateManager::SaveTemplateToFile(std::wstring_view filePath, const TemplateProfile& profile) {
    JsonValue root(JsonType::Object);
    root["name"] = profile.name;
    root["description"] = profile.description;

    JsonValue tweaksObj(JsonType::Object);
    for (const auto& [id, enabled] : profile.tweakStates) {
        tweaksObj[id] = JsonValue(enabled);
    }
    root["tweaks"] = tweaksObj;

    std::wstring pathStr(filePath);
    std::ofstream file(pathStr);
    if (!file.is_open()) return false;

    file << root.toString(2);
    file.close();
    return true;
}

bool TemplateManager::ApplyTemplate(
    const TemplateProfile& profile,
    UserSelectionMode mode,
    const std::vector<std::wstring>& users,
    bool dryRun
) {
    bool allOk = true;
    for (const auto& [tweakId, shouldEnable] : profile.tweakStates) {
        if (!dryRun) {
            const bool ok = TweakRegistry::Instance().ApplyTweak(tweakId, shouldEnable, mode, users);
            if (!ok) allOk = false;
        }
    }
    return allOk;
}

} // namespace PrivatizeWin
