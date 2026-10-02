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
