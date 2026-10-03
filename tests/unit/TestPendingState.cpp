#include "../TestFramework.h"
#include "../../src/core/SimpleJson.h"
#include "../../src/core/TweakRegistry.h"
#include <windows.h>
#include <unordered_set>
#include <string>

using namespace PrivatizeWin;

TEST_CASE(Unit_PendingState, RoundTripSerializationAndParsing) {
    std::unordered_set<std::string> pendingEnable = { "A001_USER", "A002_USER", "TEL_DIAG" };
    std::unordered_set<std::string> pendingRevert = { "TEL_CEIP" };

    // 1. Serialize to JSON
    JsonValue root;
    root["pendingEnable"] = JsonValue(JsonType::Array);
    for (const auto& id : pendingEnable) {
        root["pendingEnable"].arrayValue.push_back(JsonValue(id));
    }

    root["pendingRevert"] = JsonValue(JsonType::Array);
    for (const auto& id : pendingRevert) {
        root["pendingRevert"].arrayValue.push_back(JsonValue(id));
    }

    const std::string jsonStr = root.toString(2);
    ASSERT_TRUE(!jsonStr.empty());
    ASSERT_TRUE(jsonStr.find("A001_USER") != std::string::npos);
    ASSERT_TRUE(jsonStr.find("TEL_CEIP") != std::string::npos);

    // 2. Write to temporary file
    wchar_t tempPath[MAX_PATH]{};
    ASSERT_TRUE(GetTempPathW(MAX_PATH, tempPath) > 0);
    const std::wstring testFile = std::wstring(tempPath) + L"PrivatizeWin_Test_Pending.json";

    HANDLE hFile = CreateFileW(testFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    DWORD bytesWritten = 0;
    WriteFile(hFile, jsonStr.data(), static_cast<DWORD>(jsonStr.size()), &bytesWritten, nullptr);
    CloseHandle(hFile);

    // 3. Read back and parse
    HANDLE hRead = CreateFileW(testFile.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hRead, INVALID_HANDLE_VALUE);
    DWORD fileSize = GetFileSize(hRead, nullptr);
    ASSERT_TRUE(fileSize > 0);

    std::string readContent(fileSize, 0);
    DWORD bytesRead = 0;
    ReadFile(hRead, &readContent[0], fileSize, &bytesRead, nullptr);
    CloseHandle(hRead);

    // Clean up temporary file
    DeleteFileW(testFile.c_str());

    const JsonValue parsedRoot = JsonValue::parse(readContent);
    ASSERT_TRUE(parsedRoot.isObject());
    ASSERT_TRUE(parsedRoot["pendingEnable"].isArray());
    ASSERT_TRUE(parsedRoot["pendingRevert"].isArray());

    std::unordered_set<std::string> restoredEnable;
    for (const auto& item : parsedRoot["pendingEnable"].arrayValue) {
        if (item.isString()) {
            restoredEnable.insert(item.stringValue);
        }
    }

    std::unordered_set<std::string> restoredRevert;
    for (const auto& item : parsedRoot["pendingRevert"].arrayValue) {
        if (item.isString()) {
            restoredRevert.insert(item.stringValue);
        }
    }

    ASSERT_EQ(restoredEnable.size(), pendingEnable.size());
    ASSERT_EQ(restoredRevert.size(), pendingRevert.size());
    for (const auto& id : pendingEnable) {
        ASSERT_TRUE(restoredEnable.count(id) > 0);
    }
    for (const auto& id : pendingRevert) {
        ASSERT_TRUE(restoredRevert.count(id) > 0);
    }
}
