#include "PendingHandoff.h"
#include "SimpleJson.h"
#include <algorithm>
#include <unordered_set>
#include <objbase.h>

namespace PrivatizeWin {

static std::wstring NormalizePath(const std::wstring& path) {
    if (path.empty()) return L"";
    wchar_t fullPath[MAX_PATH * 2]{};
    DWORD len = GetFullPathNameW(path.c_str(), static_cast<DWORD>(std::size(fullPath)), fullPath, nullptr);
    if (len == 0 || len >= std::size(fullPath)) return L"";
    std::wstring result(fullPath);
    // Lowercase for comparison
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

bool PendingHandoff::IsValidHandoffPath(const std::wstring& filePath) {
    if (filePath.empty()) return false;

    const std::wstring normTarget = NormalizePath(filePath);
    if (normTarget.empty()) return false;

    // Check directory: must be inside %TEMP% or %LOCALAPPDATA%
    wchar_t tempDirBuf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDirBuf);
    std::wstring normTemp = NormalizePath(tempDirBuf);

    wchar_t localAppBuf[MAX_PATH]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppBuf, MAX_PATH);
    std::wstring normLocalApp = NormalizePath(localAppBuf);

    bool insideTemp = (!normTemp.empty() && normTarget.rfind(normTemp, 0) == 0);
    bool insideLocalApp = (!normLocalApp.empty() && normTarget.rfind(normLocalApp, 0) == 0);

    if (!insideTemp && !insideLocalApp) {
        return false;
    }

    // Check filename pattern: must start with privatizewin_
    size_t lastSlash = normTarget.find_last_of(L"\\/");
    std::wstring filename = (lastSlash != std::wstring::npos) ? normTarget.substr(lastSlash + 1) : normTarget;
    if (filename.rfind(L"privatizewin_", 0) != 0) {
        return false;
    }

    return true;
}

std::wstring PendingHandoff::SaveHandoff(const PendingStatePlan& plan) {
    if (plan.pendingEnable.empty() && plan.pendingRevert.empty()) {
        return L"";
    }

    // Check for overlapping IDs (cannot be in both enable and revert)
    std::unordered_set<std::string> enableSet(plan.pendingEnable.begin(), plan.pendingEnable.end());
    for (const auto& id : plan.pendingRevert) {
        if (enableSet.count(id) > 0) {
            return L""; // Reject overlapping plan
        }
    }

    wchar_t tempDir[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        return L"";
    }

    // Generate unique GUID filename
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) {
        return L"";
    }

    wchar_t guidStr[64]{};
    swprintf_s(guidStr, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    std::wstring filePath = std::wstring(tempDir) + L"PrivatizeWin_Handoff_" + guidStr + L".tmp";

    // Build payload
    JsonValue root;
    root["version"] = JsonValue(1.0);
    root["pendingEnable"] = JsonValue(JsonType::Array);
    for (const auto& id : plan.pendingEnable) {
        root["pendingEnable"].arrayValue.push_back(JsonValue(id));
    }
    root["pendingRevert"] = JsonValue(JsonType::Array);
    for (const auto& id : plan.pendingRevert) {
        root["pendingRevert"].arrayValue.push_back(JsonValue(id));
    }

    std::string payload = std::string(MAGIC_HEADER) + root.toString(0);

    // Create uniquely and write completely
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return L"";
    }

    DWORD bytesWritten = 0;
    const BOOL writeSuccess = WriteFile(hFile, payload.data(), static_cast<DWORD>(payload.size()), &bytesWritten, nullptr);
    CloseHandle(hFile);

    if (!writeSuccess || bytesWritten != static_cast<DWORD>(payload.size())) {
        DeleteFileW(filePath.c_str());
        return L"";
    }

    return filePath;
}

bool PendingHandoff::ValidateAndLoadHandoff(const std::wstring& filePath, PendingStatePlan& outPlan) {
    if (!IsValidHandoffPath(filePath)) {
        return false;
    }

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    const size_t magicLen = strlen(MAGIC_HEADER);
    if (fileSize <= magicLen || fileSize == INVALID_FILE_SIZE || fileSize > MAX_HANDOFF_SIZE) {
        CloseHandle(hFile);
        return false; // Do not delete
    }

    std::string content(fileSize, 0);
    DWORD bytesRead = 0;
    const BOOL readOk = ReadFile(hFile, &content[0], fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);

    if (!readOk || bytesRead != fileSize) {
        return false;
    }

    // Verify magic header
    if (content.compare(0, magicLen, MAGIC_HEADER) != 0) {
        return false;
    }

    std::string jsonPart = content.substr(magicLen);
    try {
        JsonValue root = JsonValue::parse(jsonPart);
        if (!root.isObject()) {
            return false;
        }

        if (!root["pendingEnable"].isArray() || !root["pendingRevert"].isArray()) {
            return false;
        }

        std::vector<std::string> enables;
        for (const auto& item : root["pendingEnable"].arrayValue) {
            if (item.isString() && !item.stringValue.empty()) {
                enables.push_back(item.stringValue);
            }
        }

        std::vector<std::string> reverts;
        for (const auto& item : root["pendingRevert"].arrayValue) {
            if (item.isString() && !item.stringValue.empty()) {
                reverts.push_back(item.stringValue);
            }
        }

        // Verify no overlapping IDs
        std::unordered_set<std::string> enableSet(enables.begin(), enables.end());
        for (const auto& id : reverts) {
            if (enableSet.count(id) > 0) {
                return false; // Reject overlapping IDs
            }
        }

        outPlan.pendingEnable = std::move(enables);
        outPlan.pendingRevert = std::move(reverts);
        return true;
    } catch (...) {
        return false;
    }
}

bool PendingHandoff::ConsumeHandoff(const std::wstring& filePath, PendingStatePlan& outPlan) {
    if (!ValidateAndLoadHandoff(filePath, outPlan)) {
        // Do NOT delete the file if validation failed or if it's unrelated
        return false;
    }

    // Only delete once proven to be an authentic PrivatizeWin handoff
    DeleteFileW(filePath.c_str());
    return true;
}

} // namespace PrivatizeWin
