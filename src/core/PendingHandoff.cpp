#include "PendingHandoff.h"
#include "SimpleJson.h"
#include "TweakRegistry.h"
#include <algorithm>
#include <unordered_set>
#include <cmath>
#include <objbase.h>

namespace PrivatizeWin {

static std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return std::string();
    std::string result(static_cast<size_t>(sizeNeeded), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &result[0], sizeNeeded, nullptr, nullptr);
    return result;
}

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

static std::wstring GetFinalPath(HANDLE hFile) {
    wchar_t finalPathBuf[MAX_PATH * 2]{};
    DWORD finalLen = GetFinalPathNameByHandleW(hFile, finalPathBuf, static_cast<DWORD>(std::size(finalPathBuf)), FILE_NAME_NORMALIZED);
    if (finalLen == 0 || finalLen >= std::size(finalPathBuf)) {
        return L"";
    }
    std::wstring finalPath(finalPathBuf);
    if (finalPath.rfind(L"\\\\?\\unc\\", 0) == 0 || finalPath.rfind(L"\\\\?\\UNC\\", 0) == 0) {
        finalPath = L"\\\\" + finalPath.substr(8);
    } else if (finalPath.rfind(L"\\\\?\\", 0) == 0) {
        finalPath = finalPath.substr(4);
    }
    std::transform(finalPath.begin(), finalPath.end(), finalPath.begin(), ::towlower);
    return finalPath;
}

static std::wstring GetDirectoryFinalPath(const std::wstring& dir) {
    if (dir.empty()) return L"";
    HANDLE hDir = CreateFileW(dir.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (hDir == INVALID_HANDLE_VALUE) {
        return NormalizePath(dir);
    }
    std::wstring finalPath = GetFinalPath(hDir);
    CloseHandle(hDir);
    if (finalPath.empty()) {
        return NormalizePath(dir);
    }
    return finalPath;
}

bool PendingHandoff::IsValidHandoffPath(const std::wstring& filePath) {
    if (filePath.empty()) return false;

    // Reject relative traversal
    if (filePath.find(L"..") != std::wstring::npos) return false;

    const std::wstring normTarget = NormalizePath(filePath);
    if (normTarget.empty()) return false;

    // Check directory: must be inside %TEMP% or %LOCALAPPDATA% directory
    wchar_t tempDirBuf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDirBuf);
    std::wstring normTemp = NormalizePath(tempDirBuf);
    if (!normTemp.empty() && normTemp.back() != L'\\') normTemp += L'\\';

    wchar_t localAppBuf[MAX_PATH]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppBuf, MAX_PATH);
    std::wstring normLocalApp = NormalizePath(localAppBuf);
    if (!normLocalApp.empty() && normLocalApp.back() != L'\\') normLocalApp += L'\\';

    bool insideTemp = (!normTemp.empty() && normTarget.rfind(normTemp, 0) == 0);
    bool insideLocalApp = (!normLocalApp.empty() && normTarget.rfind(normLocalApp, 0) == 0);

    if (!insideTemp && !insideLocalApp) {
        return false;
    }

    // Check filename pattern: must start with privatizewin_ and end with .tmp
    size_t lastSlash = normTarget.find_last_of(L"\\/");
    std::wstring filename = (lastSlash != std::wstring::npos) ? normTarget.substr(lastSlash + 1) : normTarget;
    if (filename.rfind(L"privatizewin_", 0) != 0) {
        return false;
    }
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != L".tmp") {
        return false;
    }

    return true;
}

std::wstring PendingHandoff::SaveHandoff(const PendingStatePlan& plan, std::wstring* outToken) {
    if (plan.pendingEnable.empty() && plan.pendingRevert.empty()) {
        return L"";
    }

    // Validate plan conflicts
    std::string conflictErr;
    if (!TweakRegistry::Instance().ValidatePendingPlan(plan.pendingEnable, plan.pendingRevert, conflictErr)) {
        return L"";
    }

    wchar_t tempDir[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        return L"";
    }

    // Generate unique GUID filename and authentication token
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) {
        return L"";
    }

    wchar_t guidStr[64]{};
    swprintf_s(guidStr, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    GUID tokenGuid{};
    if (FAILED(CoCreateGuid(&tokenGuid))) {
        return L"";
    }
    wchar_t tokenStr[64]{};
    swprintf_s(tokenStr, L"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
        tokenGuid.Data1, tokenGuid.Data2, tokenGuid.Data3,
        tokenGuid.Data4[0], tokenGuid.Data4[1], tokenGuid.Data4[2], tokenGuid.Data4[3],
        tokenGuid.Data4[4], tokenGuid.Data4[5], tokenGuid.Data4[6], tokenGuid.Data4[7]);

    std::wstring filePath = std::wstring(tempDir) + L"PrivatizeWin_Handoff_" + guidStr + L".tmp";

    // Build payload
    JsonValue root;
    root["version"] = JsonValue(1.0);
    root["token"] = JsonValue(WStringToUtf8(tokenStr));
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

    if (outToken) {
        *outToken = tokenStr;
    }

    return filePath;
}

static bool ProcessHandoff(const std::wstring& filePath, PendingStatePlan& outPlan, const std::wstring& expectedToken, bool deleteOnSuccess) {
    if (expectedToken.empty()) {
        return false; // Authentication token is strictly required
    }

    if (!PendingHandoff::IsValidHandoffPath(filePath)) {
        return false;
    }

    const DWORD desiredAccess = GENERIC_READ | (deleteOnSuccess ? DELETE : 0);
    const DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
    HANDLE hFile = CreateFileW(filePath.c_str(), desiredAccess, shareMode, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Fail closed if file info cannot be queried
    BY_HANDLE_FILE_INFORMATION fi{};
    if (!GetFileInformationByHandle(hFile, &fi)) {
        CloseHandle(hFile);
        return false;
    }

    // Reject reparse points
    if ((fi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        CloseHandle(hFile);
        return false;
    }

    // Resolve junctions via GetFinalPathNameByHandleW
    const std::wstring finalPath = GetFinalPath(hFile);
    if (finalPath.empty()) {
        CloseHandle(hFile);
        return false;
    }

    wchar_t tempDirBuf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDirBuf);
    std::wstring finalTemp = GetDirectoryFinalPath(tempDirBuf);
    if (!finalTemp.empty() && finalTemp.back() != L'\\') finalTemp += L'\\';

    wchar_t localAppBuf[MAX_PATH]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppBuf, MAX_PATH);
    std::wstring finalLocalApp = GetDirectoryFinalPath(localAppBuf);
    if (!finalLocalApp.empty() && finalLocalApp.back() != L'\\') finalLocalApp += L'\\';

    bool insideFinalTemp = (!finalTemp.empty() && finalPath.rfind(finalTemp, 0) == 0);
    bool insideFinalLocalApp = (!finalLocalApp.empty() && finalPath.rfind(finalLocalApp, 0) == 0);
    if (!insideFinalTemp && !insideFinalLocalApp) {
        CloseHandle(hFile);
        return false;
    }

    // Check final filename pattern
    size_t lastSlash = finalPath.find_last_of(L"\\/");
    std::wstring finalFilename = (lastSlash != std::wstring::npos) ? finalPath.substr(lastSlash + 1) : finalPath;
    if (finalFilename.rfind(L"privatizewin_", 0) != 0 || finalFilename.size() < 4 || finalFilename.substr(finalFilename.size() - 4) != L".tmp") {
        CloseHandle(hFile);
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    const size_t magicLen = strlen(PendingHandoff::MAGIC_HEADER);
    if (fileSize <= magicLen || fileSize == INVALID_FILE_SIZE || fileSize > PendingHandoff::MAX_HANDOFF_SIZE) {
        CloseHandle(hFile);
        return false;
    }

    std::string content(static_cast<size_t>(fileSize), '\0');
    DWORD bytesRead = 0;
    const BOOL readOk = ReadFile(hFile, &content[0], fileSize, &bytesRead, nullptr);
    if (!readOk || bytesRead != fileSize) {
        CloseHandle(hFile);
        return false;
    }

    // Verify magic header
    if (content.compare(0, magicLen, PendingHandoff::MAGIC_HEADER) != 0) {
        CloseHandle(hFile);
        return false;
    }

    std::string jsonPart = content.substr(magicLen);
    try {
        JsonValue root = JsonValue::parse(jsonPart);
        if (!root.isObject()) {
            CloseHandle(hFile);
            return false;
        }

        // Verify version is strictly 1.0 (reject 1.5, non-numbers, etc.)
        if (!root["version"].isNumber() || root["version"].numberValue != 1.0) {
            CloseHandle(hFile);
            return false;
        }

        // Verify token
        const std::string expectedTokenStr = WStringToUtf8(expectedToken);
        if (!root["token"].isString() || root["token"].stringValue != expectedTokenStr) {
            CloseHandle(hFile);
            return false;
        }

        if (!root["pendingEnable"].isArray() || !root["pendingRevert"].isArray()) {
            CloseHandle(hFile);
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

        // Validate plan conflicts using TweakRegistry
        std::string conflictErr;
        if (!TweakRegistry::Instance().ValidatePendingPlan(enables, reverts, conflictErr)) {
            CloseHandle(hFile);
            return false;
        }

        if (deleteOnSuccess) {
            FILE_DISPOSITION_INFO fdi{};
            fdi.DeleteFile = TRUE;
            if (!SetFileInformationByHandle(hFile, FileDispositionInfo, &fdi, sizeof(fdi))) {
                CloseHandle(hFile);
                return false;
            }
        }

        CloseHandle(hFile);

        outPlan.pendingEnable = std::move(enables);
        outPlan.pendingRevert = std::move(reverts);
    } catch (...) {
        CloseHandle(hFile);
        return false;
    }

#ifdef PRIVATIZEWIN_ENABLE_TEST_HOOKS
    if (deleteOnSuccess && PendingHandoff::s_postCloseCallback) {
        PendingHandoff::s_postCloseCallback(filePath);
    }
#endif

    return true;
}

bool PendingHandoff::ValidateAndLoadHandoff(const std::wstring& filePath, PendingStatePlan& outPlan, const std::wstring& expectedToken) {
    return ProcessHandoff(filePath, outPlan, expectedToken, false);
}

bool PendingHandoff::ConsumeHandoff(const std::wstring& filePath, PendingStatePlan& outPlan, const std::wstring& expectedToken) {
    return ProcessHandoff(filePath, outPlan, expectedToken, true);
}

} // namespace PrivatizeWin
