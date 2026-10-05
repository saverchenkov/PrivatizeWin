#include "../TestFramework.h"
#include "../../src/core/SimpleJson.h"
#include "../../src/core/SmartHandle.h"
#include <windows.h>
#include <vector>
#include <string>

using namespace PrivatizeWin;

static std::wstring GetPrivatizeWinExePath() {
#ifdef PRIVATIZEWIN_EXE_PATH
    std::string cmakeTargetExe = PRIVATIZEWIN_EXE_PATH;
    std::wstring cmakeWPath(cmakeTargetExe.begin(), cmakeTargetExe.end());
    if (GetFileAttributesW(cmakeWPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return cmakeWPath;
    }
#endif

    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring pathStr(exePath);
    const size_t lastSlash = pathStr.find_last_of(L"\\/");
    std::wstring dir = (lastSlash != std::wstring::npos) ? pathStr.substr(0, lastSlash) : L"";
    std::wstring parentDir = (dir.find_last_of(L"\\/") != std::wstring::npos) ? dir.substr(0, dir.find_last_of(L"\\/")) : dir;
    std::wstring grandParentDir = (parentDir.find_last_of(L"\\/") != std::wstring::npos) ? parentDir.substr(0, parentDir.find_last_of(L"\\/")) : parentDir;

    const std::vector<std::wstring> names = {
        L"PrivatizeWin_x65.exe",
        L"PrivatizeWin_x64.exe",
        L"PrivatizeWin_arm.exe",
        L"PrivatizeWin_arm64.exe",
        L"PrivatizeWin.exe"
    };

    const std::vector<std::wstring> searchDirs = {
        dir,
        parentDir,
        grandParentDir,
        grandParentDir + L"\\Release",
        grandParentDir + L"\\Debug",
        parentDir + L"\\Release"
    };

    for (const auto& d : searchDirs) {
        for (const auto& name : names) {
            std::wstring candidate = d + L"\\" + name;
            if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return candidate;
            }
        }
    }

    return dir + L"\\PrivatizeWin.exe";
}

static std::pair<int, std::string> RunSubprocess(const std::wstring& cmd, DWORD timeoutMs = 15000) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr;
    HANDLE hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        return { -1, "" };
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    UniqueHandle hPipeRead(hRead);
    UniqueHandle hPipeWrite(hWrite);

    STARTUPINFOW si{ sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hPipeWrite.get();
    si.hStdError = hPipeWrite.get();
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(0);

    if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return { -1, "" };
    }

    UniqueHandle hProc(pi.hProcess);
    UniqueHandle hThread(pi.hThread);

    // Close write handle in parent so child owns the write end
    hPipeWrite.reset();

    std::string output;
    char buffer[4096];
    const DWORD startTick = GetTickCount();
    bool timedOut = false;

    while (true) {
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(hPipeRead.get(), nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
            // Pipe broken (child closed stdout/stderr)
            break;
        }

        if (bytesAvailable > 0) {
            DWORD bytesToRead = (std::min)(bytesAvailable, static_cast<DWORD>(sizeof(buffer) - 1));
            DWORD bytesRead = 0;
            if (ReadFile(hPipeRead.get(), buffer, bytesToRead, &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead] = 0;
                output.append(buffer, bytesRead);
            }
        } else {
            // Check if process has terminated
            const DWORD waitRes = WaitForSingleObject(hProc.get(), 20);
            if (waitRes == WAIT_OBJECT_0) {
                // Drain any remaining bytes in pipe
                if (PeekNamedPipe(hPipeRead.get(), nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0) {
                    DWORD bytesRead = 0;
                    if (ReadFile(hPipeRead.get(), buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                        buffer[bytesRead] = 0;
                        output.append(buffer, bytesRead);
                    }
                }
                break;
            }
        }

        if (GetTickCount() - startTick > timeoutMs) {
            timedOut = true;
            TerminateProcess(hProc.get(), 101);
            WaitForSingleObject(hProc.get(), 1000);
            break;
        }
    }

    if (timedOut) {
        return { -999, "ERROR: Subprocess timed out after " + std::to_string(timeoutMs) + " ms" };
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(hProc.get(), &exitCode);
    return { static_cast<int>(exitCode), output };
}

TEST_CASE(Integration_CliEndToEnd, ListTemplatesExecution) {
    const std::wstring appExe = GetPrivatizeWinExePath();
    const std::wstring cmd = L"\"" + appExe + L"\" --list-templates";
    const auto [exitCode, output] = RunSubprocess(cmd);

    ASSERT_EQ(exitCode, 0);
    ASSERT_TRUE(output.find("PrivatizeWin Available Templates") != std::string::npos);
    ASSERT_TRUE(output.find("recommended") != std::string::npos);
    ASSERT_TRUE(output.find("strict") != std::string::npos);
}

TEST_CASE(Integration_CliEndToEnd, StatusJsonOutput) {
    const std::wstring appExe = GetPrivatizeWinExePath();
    const std::wstring cmd = L"\"" + appExe + L"\" --status --output json";
    const auto [exitCode, output] = RunSubprocess(cmd);

    ASSERT_EQ(exitCode, 0);
    const JsonValue root = JsonValue::parse(output);
    ASSERT_TRUE(root.isObject());
    ASSERT_TRUE(root["tweaks"].isArray());
    ASSERT_TRUE(root["tweaks"].arrayValue.size() >= 20);
}

TEST_CASE(Integration_CliEndToEnd, DryRunTemplateExecution) {
    const std::wstring appExe = GetPrivatizeWinExePath();
    const std::wstring cmd = L"\"" + appExe + L"\" --apply-template minimal --dry-run";
    const auto [exitCode, output] = RunSubprocess(cmd);

    ASSERT_EQ(exitCode, 0);
    ASSERT_TRUE(output.find("[DRY RUN]") != std::string::npos);
    ASSERT_TRUE(output.find("Dry run completed without making any changes.") != std::string::npos);
}

TEST_CASE(Integration_CliEndToEnd, UsersNoneOnUserOnlyProfile) {
    Test::TestTempDirectory tempDir;
    const std::wstring profPath = tempDir.GetFilePath(L"user_only_profile.json");

    const std::string jsonContent = R"({
        "name": "User Only Profile",
        "tweaks": {
            "PRIV_AD_ID_USER": true
        }
    })";

    HANDLE hFile = CreateFileW(profPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, jsonContent.data(), static_cast<DWORD>(jsonContent.size()), &written, nullptr);
    CloseHandle(hFile);

    const std::wstring appExe = GetPrivatizeWinExePath();

    // 1. Dry run with --users none: should report 0 operations planned and succeed
    const std::wstring cmdDry = L"\"" + appExe + L"\" --apply-template \"" + profPath + L"\" --users none --dry-run";
    const auto [dryExit, dryOut] = RunSubprocess(cmdDry);
    ASSERT_EQ(dryExit, 0);
    ASSERT_TRUE(dryOut.find("0 operations planned") != std::string::npos || dryOut.find("No applicable operations") != std::string::npos || dryOut.find("[DRY RUN]") != std::string::npos);

    // 2. Execution with --users none: should NOT fail with exit code 1 or 2
    const std::wstring cmdExec = L"\"" + appExe + L"\" --apply-template \"" + profPath + L"\" --users none";
    const auto [execExit, execOut] = RunSubprocess(cmdExec);
    ASSERT_EQ(execExit, 0);
    ASSERT_TRUE(execOut.find("No applicable operations") != std::string::npos || execOut.find("Applied: 0") != std::string::npos);
}

TEST_CASE(Integration_CliEndToEnd, ConflictingProfileExitsWithCode4) {
    Test::TestTempDirectory tempDir;
    const std::wstring profPath = tempDir.GetFilePath(L"conflicting_profile.json");

    const std::string jsonContent = R"({
        "name": "Conflicting CEIP Profile",
        "tweaks": {
            "TEL_CEIP": true,
            "P027": false
        }
    })";

    HANDLE hFile = CreateFileW(profPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, jsonContent.data(), static_cast<DWORD>(jsonContent.size()), &written, nullptr);
    CloseHandle(hFile);

    const std::wstring appExe = GetPrivatizeWinExePath();
    const std::wstring cmd = L"\"" + appExe + L"\" --apply-template \"" + profPath + L"\"";
    const auto [exitCode, output] = RunSubprocess(cmd);

    // Conflict detection must return exit code 4 and report the contradictory tweaks
    ASSERT_EQ(exitCode, 4);
    ASSERT_TRUE(output.find("Conflict detected") != std::string::npos || output.find("contradictory") != std::string::npos);
}

