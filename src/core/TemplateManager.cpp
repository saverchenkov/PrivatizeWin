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

    // 1. Recommended (Safe) - Sparse: only includes Safe privacy tweaks
    {
        TemplateProfile p;
        p.name = "recommended";
        p.description = "Enables all safe privacy settings. No functional side effects or broken apps.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            if (t.regActions.empty() && t.serviceActions.empty()) continue;
            if (t.safety == SafetyLevel::Safe) {
                p.tweakStates[t.id] = true;
            }
        }
        m_templates[p.name] = p;
    }

    // 2. Strict (Safe + Normal) - Sparse: includes Safe and Normal privacy tweaks
    {
        TemplateProfile p;
        p.name = "strict";
        p.description = "Maximum privacy posture: disables all telemetry, Bing web search, AI recall, and edge tracking.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            if (t.regActions.empty() && t.serviceActions.empty()) continue;
            if (t.safety == SafetyLevel::Safe || t.safety == SafetyLevel::Normal) {
                p.tweakStates[t.id] = true;
            }
        }
        m_templates[p.name] = p;
    }

    // 3. Minimal (Core Telemetry Only) - Sparse: only includes Telemetry & Diagnostics category
    {
        TemplateProfile p;
        p.name = "minimal";
        p.description = "Disables only low-level OS telemetry services and diagnostic data collection.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            if (t.regActions.empty() && t.serviceActions.empty()) continue;
            if (t.category == L"Telemetry & Diagnostics") {
                p.tweakStates[t.id] = true;
            }
        }
        m_templates[p.name] = p;
    }

    // 4. Defaults / Revert (Explicit reset of all tweaks to Windows defaults)
    {
        TemplateProfile p;
        p.name = "defaults";
        p.description = "Reverts all settings to Windows out-of-the-box defaults.";
        p.isBuiltin = true;
        for (const auto& t : allTweaks) {
            if (t.regActions.empty() && t.serviceActions.empty()) continue;
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

bool TemplateManager::LoadTemplateFromFile(std::wstring_view filePath, TemplateProfile& outProfile, std::string* outError) {
    if (outError) outError->clear();
    std::wstring pathStr(filePath);
    std::ifstream file(pathStr);
    if (!file.is_open()) {
        if (outError) *outError = "Could not open template file.";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    try {
        JsonValue root = JsonValue::parse(buffer.str());
        if (!root.isObject()) {
            if (outError) *outError = "Profile root must be a JSON object.";
            return false;
        }

        if (!root.has("name") || !root["name"].isString()) {
            if (outError) *outError = "Profile requires a string 'name' property.";
            return false;
        }
        outProfile.name = root["name"].stringValue;
        outProfile.description = (root.has("description") && root["description"].isString()) ? root["description"].stringValue : "";
        outProfile.isBuiltin = false;
        outProfile.tweakStates.clear();

        if (!root.has("tweaks") || !root["tweaks"].isObject()) {
            if (outError) *outError = "Profile requires an object 'tweaks' property.";
            return false;
        }

        const auto& tweaksObj = root["tweaks"].objectValue;
        if (tweaksObj.empty()) {
            if (outError) *outError = "Profile 'tweaks' object must not be empty.";
            return false;
        }

        for (const auto& [id, val] : tweaksObj) {
            if (!val.isBool()) {
                if (outError) *outError = "Tweak '" + id + "' has non-boolean value; must be true or false.";
                return false;
            }
            if (!TweakRegistry::Instance().GetTweakById(id)) {
                if (outError) *outError = "Unknown tweak ID '" + id + "'.";
                return false;
            }
            outProfile.tweakStates[id] = val.boolValue;
        }
        return true;
    } catch (const std::exception& ex) {
        if (outError) *outError = std::string("JSON parse error: ") + ex.what();
        return false;
    } catch (...) {
        if (outError) *outError = "Unknown error parsing JSON profile.";
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

} // namespace PrivatizeWin
