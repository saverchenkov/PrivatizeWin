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

// RAII Task Service and Root Folder Connection
class TaskServiceConnection {
public:
    TaskServiceConnection() {
        if (!m_com.isOk()) return;
        HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&m_pService));
        if (FAILED(hr) || !m_pService) return;

        hr = m_pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            m_pService->Release();
            m_pService = nullptr;
            return;
        }

        hr = m_pService->GetFolder(_bstr_t(L"\\"), &m_pRootFolder);
        if (FAILED(hr)) {
            m_pRootFolder = nullptr;
            m_pService->Release();
            m_pService = nullptr;
        }
    }

    ~TaskServiceConnection() {
        if (m_pRootFolder) {
            m_pRootFolder->Release();
            m_pRootFolder = nullptr;
        }
        if (m_pService) {
            m_pService->Release();
            m_pService = nullptr;
        }
    }

    TaskServiceConnection(const TaskServiceConnection&) = delete;
    TaskServiceConnection& operator=(const TaskServiceConnection&) = delete;

    [[nodiscard]] bool isValid() const noexcept { return m_pService != nullptr && m_pRootFolder != nullptr; }
    [[nodiscard]] ITaskService* service() const noexcept { return m_pService; }
    [[nodiscard]] ITaskFolder* folder() const noexcept { return m_pRootFolder; }

private:
    ComScope m_com;
    ITaskService* m_pService{ nullptr };
    ITaskFolder* m_pRootFolder{ nullptr };
};

bool TaskScheduler::IsTaskInstalled(std::wstring_view taskName) {
    TaskServiceConnection conn;
    if (!conn.isValid()) return false;

    const std::wstring taskNameStr(taskName);
    IRegisteredTask* pRegisteredTask = nullptr;
    const HRESULT hr = conn.folder()->GetTask(_bstr_t(taskNameStr.c_str()), &pRegisteredTask);
    const bool exists = SUCCEEDED(hr) && (pRegisteredTask != nullptr);

    if (pRegisteredTask) pRegisteredTask->Release();
    return exists;
}

bool TaskScheduler::GetTaskConfig(ScheduledTaskConfig& outConfig, std::wstring_view taskName) {
    TaskServiceConnection conn;
    if (!conn.isValid()) return false;

    const std::wstring taskNameStr(taskName);
    IRegisteredTask* pRegisteredTask = nullptr;
    const HRESULT hr = conn.folder()->GetTask(_bstr_t(taskNameStr.c_str()), &pRegisteredTask);
    if (FAILED(hr) || !pRegisteredTask) {
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
    return true;
}

bool TaskScheduler::InstallTask(const ScheduledTaskConfig& config) {
    TaskServiceConnection conn;
    if (!conn.isValid()) return false;

    ITaskDefinition* pDefinition = nullptr;
    HRESULT hr = conn.service()->NewTask(0, &pDefinition);
    if (FAILED(hr) || !pDefinition) return false;

    IRegistrationInfo* pRegInfo = nullptr;
    if (SUCCEEDED(pDefinition->get_RegistrationInfo(&pRegInfo)) && pRegInfo) {
        pRegInfo->put_Author(_bstr_t(L"PrivatizeWin"));
        pRegInfo->put_Description(_bstr_t(L"PrivatizeWin automated privacy protection reapplication task."));
        pRegInfo->Release();
    }

    IPrincipal* pPrincipal = nullptr;
    if (SUCCEEDED(pDefinition->get_Principal(&pPrincipal)) && pPrincipal) {
        pPrincipal->put_Id(_bstr_t(L"Author"));
        pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
        pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        pPrincipal->Release();
    }

    ITaskSettings* pSettings = nullptr;
    if (SUCCEEDED(pDefinition->get_Settings(&pSettings)) && pSettings) {
        pSettings->put_StartWhenAvailable(VARIANT_TRUE);
        pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT1H"));
        pSettings->Release();
    }

    ITriggerCollection* pTriggers = nullptr;
    if (SUCCEEDED(pDefinition->get_Triggers(&pTriggers)) && pTriggers) {
        ITrigger* pTrigger = nullptr;
        if (config.frequency == L"logon") {
            hr = pTriggers->Create(TASK_TRIGGER_LOGON, &pTrigger);
            if (SUCCEEDED(hr) && pTrigger) {
                pTrigger->put_Id(_bstr_t(L"LogonTrigger"));
                pTrigger->put_Enabled(VARIANT_TRUE);
                pTrigger->Release();
            }
        } else if (config.frequency == L"weekly") {
            hr = pTriggers->Create(TASK_TRIGGER_WEEKLY, &pTrigger);
            if (SUCCEEDED(hr) && pTrigger) {
                IWeeklyTrigger* pWeeklyTrigger = nullptr;
                if (SUCCEEDED(pTrigger->QueryInterface(IID_IWeeklyTrigger, reinterpret_cast<void**>(&pWeeklyTrigger))) && pWeeklyTrigger) {
                    pWeeklyTrigger->put_Id(_bstr_t(L"WeeklyTrigger"));
                    pWeeklyTrigger->put_Enabled(VARIANT_TRUE);
                    pWeeklyTrigger->put_StartBoundary(_bstr_t(L"2026-01-01T12:00:00"));
                    pWeeklyTrigger->put_DaysOfWeek(1); // Sunday
                    pWeeklyTrigger->Release();
                }
                pTrigger->Release();
            }
        } else {
            // Daily
            hr = pTriggers->Create(TASK_TRIGGER_DAILY, &pTrigger);
            if (SUCCEEDED(hr) && pTrigger) {
                IDailyTrigger* pDailyTrigger = nullptr;
                if (SUCCEEDED(pTrigger->QueryInterface(IID_IDailyTrigger, reinterpret_cast<void**>(&pDailyTrigger))) && pDailyTrigger) {
                    pDailyTrigger->put_Id(_bstr_t(L"DailyTrigger"));
                    pDailyTrigger->put_Enabled(VARIANT_TRUE);
                    pDailyTrigger->put_StartBoundary(_bstr_t(L"2026-01-01T12:00:00"));
                    pDailyTrigger->put_DaysInterval(1);
                    pDailyTrigger->Release();
                }
                pTrigger->Release();
            }
        }
        pTriggers->Release();
    }

    const std::wstring exePath = GetExecutablePath();
    const std::wstring args = L"--apply-template " + config.templateName + L" --quiet --users " + config.userMode;

    IActionCollection* pActions = nullptr;
    if (SUCCEEDED(pDefinition->get_Actions(&pActions)) && pActions) {
        IAction* pAction = nullptr;
        if (SUCCEEDED(pActions->Create(TASK_ACTION_EXEC, &pAction)) && pAction) {
            IExecAction* pExec = nullptr;
            if (SUCCEEDED(pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExec))) && pExec) {
                pExec->put_Path(_bstr_t(exePath.c_str()));
                pExec->put_Arguments(_bstr_t(args.c_str()));
                pExec->Release();
            }
            pAction->Release();
        }
        pActions->Release();
    }

    IRegisteredTask* pRegisteredTask = nullptr;
    hr = conn.folder()->RegisterTaskDefinition(
        _bstr_t(config.taskName.c_str()),
        pDefinition,
        TASK_CREATE_OR_UPDATE,
        _variant_t(),
        _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask
    );

    const bool success = SUCCEEDED(hr) && (pRegisteredTask != nullptr);
    if (pRegisteredTask) pRegisteredTask->Release();
    pDefinition->Release();
    return success;
}

bool TaskScheduler::UninstallTask(std::wstring_view taskName) {
    TaskServiceConnection conn;
    if (!conn.isValid()) return false;

    const std::wstring taskNameStr(taskName);
    const HRESULT hr = conn.folder()->DeleteTask(_bstr_t(taskNameStr.c_str()), 0);
    return SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

} // namespace PrivatizeWin
