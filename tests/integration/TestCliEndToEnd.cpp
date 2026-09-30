#include "../TestFramework.h"
#include "../../src/core/SimpleJson.h"
#include "../../src/core/SmartHandle.h"
#include <windows.h>
#include <vector>
#include <string>

using namespace PrivatizeWin;

static std::wstring GetPrivatizeWinExePath() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring pathStr(exePath);
    const size_t lastSlash = pathStr.find_last_of(L"\\/");
    std::wstring dir = (lastSlash != std::wstring::npos) ? pathStr.substr(0, lastSlash) : L"";

    // 1. Check same directory
    std::wstring candidate1 = dir + L"\\PrivatizeWin.exe";
    if (GetFileAttributesW(candidate1.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return candidate1;
    }

    // 2. Check parent directory (e.g. build/tests/ -> build/)
    const size_t parentSlash = dir.find_last_of(L"\\/");
    if (parentSlash != std::wstring::npos) {
        std::wstring candidate2 = dir.substr(0, parentSlash) + L"\\PrivatizeWin.exe";
        if (GetFileAttributesW(candidate2.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate2;
        }
    }

    return candidate1;
}

static std::pair<int, std::string> RunSubprocess(const std::wstring& cmd) {
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

    // Close write handle in parent so ReadFile encounters EOF when child exits
    hPipeWrite.reset();

    std::string output;
    char buffer[1024];
    DWORD bytesRead = 0;
    while (ReadFile(hPipeRead.get(), buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        output += buffer;
    }

    WaitForSingleObject(hProc.get(), 10000);
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
