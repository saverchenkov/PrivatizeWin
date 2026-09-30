#include "TaskScheduler.h"
#include "SmartHandle.h"
#include <taskschd.h>
#include <comdef.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

namespace PrivatizeWin {

std::wstring TaskScheduler::GetExecutablePath() {
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::wstring(path);
}

// RAII COM Initializer (Rule R.1)
class ComScope {
public:
    explicit ComScope(DWORD dwCoInit = COINIT_MULTITHREADED) noexcept {
        m_hr = CoInitializeEx(nullptr, dwCoInit);
    }
    ~ComScope() noexcept {
        if (SUCCEEDED(m_hr)) {
            CoUninitialize();
        }
    }
    ComScope(const ComScope&) = delete;
    ComScope& operator=(const ComScope&) = delete;
    [[nodiscard]] bool isOk() const noexcept { return SUCCEEDED(m_hr); }
private:
    HRESULT m_hr;
};

bool TaskScheduler::IsTaskInstalled(std::wstring_view taskName) {
    ComScope com;
    if (!com.isOk()) return false;

    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
    if (FAILED(hr)) return false;

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    ITaskFolder* pRootFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    std::wstring taskNameStr(taskName);
    IRegisteredTask* pRegisteredTask = nullptr;
    hr = pRootFolder->GetTask(_bstr_t(taskNameStr.c_str()), &pRegisteredTask);
    const bool exists = SUCCEEDED(hr) && (pRegisteredTask != nullptr);

    if (pRegisteredTask) pRegisteredTask->Release();
    pRootFolder->Release();
    pService->Release();
    return exists;
}

bool TaskScheduler::InstallTask(const ScheduledTaskConfig& config) {
    const std::wstring exePath = GetExecutablePath();
    const std::wstring args = L"--apply-template " + config.templateName + L" --quiet --users " + config.userMode;

    std::wstring cmd = L"schtasks.exe /Create /TN \"" + config.taskName + L"\" /TR \"\\\"" + exePath + L"\\\" " + args + L"\" /RL HIGHEST /F ";

    if (config.frequency == L"logon") {
        cmd += L"/SC ONLOGON";
    } else if (config.frequency == L"weekly") {
        cmd += L"/SC WEEKLY /D SUN /ST 12:00";
    } else {
        cmd += L"/SC DAILY /ST 12:00";
    }

    std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back(0);

    STARTUPINFOW si{ sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};

    if (CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        UniqueHandle hProc(pi.hProcess);
        UniqueHandle hThread(pi.hThread);

        WaitForSingleObject(hProc.get(), 5000);
        DWORD exitCode = 0;
        GetExitCodeProcess(hProc.get(), &exitCode);
        return (exitCode == 0);
    }

    return false;
}

bool TaskScheduler::UninstallTask(std::wstring_view taskName) {
    std::wstring taskNameStr(taskName);
    std::wstring cmd = L"schtasks.exe /Delete /TN \"" + taskNameStr + L"\" /F";
    std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back(0);

    STARTUPINFOW si{ sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};

    if (CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        UniqueHandle hProc(pi.hProcess);
        UniqueHandle hThread(pi.hThread);

        WaitForSingleObject(hProc.get(), 5000);
        DWORD exitCode = 0;
        GetExitCodeProcess(hProc.get(), &exitCode);
        return (exitCode == 0);
    }

    return false;
}

} // namespace PrivatizeWin
