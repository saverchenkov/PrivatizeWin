#include "../TestFramework.h"
#include "../../src/core/SimpleJson.h"
#include "../../src/core/TweakRegistry.h"
#include "../../src/core/PendingHandoff.h"
#include <windows.h>
#include <unordered_set>
#include <string>

using namespace PrivatizeWin;

TEST_CASE(Unit_PendingState, RoundTripSerializationAndParsing) {
    Test::TestTempDirectory tempDir;
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

    // 2. Write to temporary file in isolated directory
    const std::wstring testFile = tempDir.GetFilePath(L"PrivatizeWin_Test_Pending.json");

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

TEST_CASE(Unit_PendingState, PendingHandoff_RoundTripAndCleanDeletion) {
    PendingStatePlan plan;
    plan.pendingEnable = { "TEL_DIAGTRACK", "AI_RECALL" };
    plan.pendingRevert = { "PRIV_AD_ID_USER" };

    std::wstring token;
    const std::wstring outPath = PendingHandoff::SaveHandoff(plan, &token);
    ASSERT_FALSE(outPath.empty());
    ASSERT_FALSE(token.empty());
    ASSERT_NE(GetFileAttributesW(outPath.c_str()), INVALID_FILE_ATTRIBUTES);

    // Wrong token must be rejected and must NOT delete the file
    PendingStatePlan dummyPlan;
    const bool wrongTokenOk = PendingHandoff::ConsumeHandoff(outPath, dummyPlan, L"invalid_token_12345");
    ASSERT_FALSE(wrongTokenOk);
    ASSERT_NE(GetFileAttributesW(outPath.c_str()), INVALID_FILE_ATTRIBUTES);

    // Correct token succeeds
    PendingStatePlan loadedPlan;
    const bool loadOk = PendingHandoff::ConsumeHandoff(outPath, loadedPlan, token);
    ASSERT_TRUE(loadOk);
    ASSERT_EQ(loadedPlan.pendingEnable.size(), plan.pendingEnable.size());
    ASSERT_EQ(loadedPlan.pendingRevert.size(), plan.pendingRevert.size());

    // After successful validated read via ConsumeHandoff, file must be cleanly deleted
    ASSERT_EQ(GetFileAttributesW(outPath.c_str()), INVALID_FILE_ATTRIBUTES);
}

TEST_CASE(Unit_PendingState, PendingHandoff_UnrelatedFileSurvivesRejection) {
    Test::TestTempDirectory tempDir;
    const std::wstring unrelatedFile = tempDir.GetFilePath(L"unrelated_user_document.txt");

    // Create an unrelated file
    HANDLE hFile = CreateFileW(unrelatedFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    const char text[] = "Do not delete this critical document!";
    DWORD written = 0;
    WriteFile(hFile, text, sizeof(text), &written, nullptr);
    CloseHandle(hFile);

    // Attempt to consume via handoff mechanism
    PendingStatePlan dummyPlan;
    const bool loadOk = PendingHandoff::ConsumeHandoff(unrelatedFile, dummyPlan);
    ASSERT_FALSE(loadOk);

    // CRITICAL: Unrelated file MUST survive and NOT be deleted
    ASSERT_NE(GetFileAttributesW(unrelatedFile.c_str()), INVALID_FILE_ATTRIBUTES);
}

TEST_CASE(Unit_PendingState, PendingHandoff_MalformedAndOverlappingRejected) {
    Test::TestTempDirectory tempDir;
    const std::wstring testHandoff = tempDir.GetFilePath(L"privatizewin_test_malformed.tmp");

    // Construct a payload with overlapping IDs
    std::string badPayload = "PRIVATIZEWIN_HANDOFF_V1\n{\"version\":1,\"pendingEnable\": [\"OVERLAP_01\"], \"pendingRevert\": [\"OVERLAP_01\"]}";

    HANDLE hFile = CreateFileW(testHandoff.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, badPayload.data(), static_cast<DWORD>(badPayload.size()), &written, nullptr);
    CloseHandle(hFile);

    PendingStatePlan dummyPlan;
    const bool loadOk = PendingHandoff::ConsumeHandoff(testHandoff, dummyPlan);
    ASSERT_FALSE(loadOk);

    // File with malformed schema / overlapping IDs must NOT be deleted by reader
    ASSERT_NE(GetFileAttributesW(testHandoff.c_str()), INVALID_FILE_ATTRIBUTES);
}
