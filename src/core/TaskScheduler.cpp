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

bool TaskScheduler::GetTaskConfig(ScheduledTaskConfig& outConfig, std::wstring_view taskName) {
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
    if (FAILED(hr) || !pRegisteredTask) {
        pRootFolder->Release();
        pService->Release();
        return false;
    }

    outConfig.taskName = taskNameStr;
    outConfig.frequency = L"daily";
    outConfig.templateName = L"recommended";
    outConfig.userMode = L"all";

    ITaskDefinition* pDefinition = nullptr;
    if (SUCCEEDED(pRegisteredTask->get_Definition(&pDefinition)) && pDefinition) {
        IActionCollection* pActions = nullptr;
        if (SUCCEEDED(pDefinition->get_Actions(&pActions)) && pActions) {
            IAction* pAction = nullptr;
            if (SUCCEEDED(pActions->get_Item(1, &pAction)) && pAction) {
                IExecAction* pExec = nullptr;
                if (SUCCEEDED(pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExec))) && pExec) {
                    BSTR bstrArgs = nullptr;
                    if (SUCCEEDED(pExec->get_Arguments(&bstrArgs)) && bstrArgs) {
                        std::wstring args(bstrArgs);
                        SysFreeString(bstrArgs);
                        size_t pos = args.find(L"--apply-template ");
                        if (pos != std::wstring::npos) {
                            size_t start = pos + 17;
                            size_t end = args.find(L' ', start);
                            outConfig.templateName = args.substr(start, (end == std::wstring::npos) ? std::wstring::npos : (end - start));
                        }
                        pos = args.find(L"--users ");
                        if (pos != std::wstring::npos) {
                            size_t start = pos + 8;
                            size_t end = args.find(L' ', start);
                            outConfig.userMode = args.substr(start, (end == std::wstring::npos) ? std::wstring::npos : (end - start));
                        }
                    }
                    pExec->Release();
                }
                pAction->Release();
            }
            pActions->Release();
        }

        ITriggerCollection* pTriggers = nullptr;
        if (SUCCEEDED(pDefinition->get_Triggers(&pTriggers)) && pTriggers) {
            ITrigger* pTrigger = nullptr;
            if (SUCCEEDED(pTriggers->get_Item(1, &pTrigger)) && pTrigger) {
                TASK_TRIGGER_TYPE2 trigType;
                if (SUCCEEDED(pTrigger->get_Type(&trigType))) {
                    if (trigType == TASK_TRIGGER_LOGON) outConfig.frequency = L"logon";
                    else if (trigType == TASK_TRIGGER_WEEKLY) outConfig.frequency = L"weekly";
                    else outConfig.frequency = L"daily";
                }
                pTrigger->Release();
            }
            pTriggers->Release();
        }
        pDefinition->Release();
    }

    pRegisteredTask->Release();
    pRootFolder->Release();
    pService->Release();
    return true;
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
