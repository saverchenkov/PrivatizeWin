#include "ScheduleDialog.h"
#include "DarkMode.h"
#include "../core/TaskScheduler.h"
#include "../core/Localization.h"
#include <commctrl.h>
#include <string>

namespace PrivatizeWin {

static HWND s_hFreqCombo = nullptr;
static HWND s_hTplCombo = nullptr;
static HWND s_hUserCombo = nullptr;
static HWND s_hStatusLabel = nullptr;
static HWND s_hBtnInstall = nullptr;
static HWND s_hBtnRemove = nullptr;

static void RefreshScheduleDialogState(HWND /*hWnd*/) {
    ScheduledTaskConfig cfg;
    const bool installed = TaskScheduler::GetTaskConfig(cfg);

    if (installed) {
        std::wstring freqStr;
        if (cfg.frequency == L"logon") freqStr = Loc("sched_logon");
        else if (cfg.frequency == L"weekly") freqStr = Loc("sched_weekly");
        else freqStr = Loc("sched_daily");

        std::wstring statusStr = LocFmt("sched_status_active", { freqStr });
        SetWindowTextW(s_hStatusLabel, statusStr.c_str());

        SetWindowTextW(s_hBtnInstall, Loc("sched_btn_update").c_str());
        EnableWindow(s_hBtnRemove, TRUE);

        // Select combos
        if (cfg.frequency == L"logon") SendMessage(s_hFreqCombo, CB_SETCURSEL, 1, 0);
        else if (cfg.frequency == L"weekly") SendMessage(s_hFreqCombo, CB_SETCURSEL, 2, 0);
        else SendMessage(s_hFreqCombo, CB_SETCURSEL, 0, 0);

        if (cfg.templateName == L"strict") SendMessage(s_hTplCombo, CB_SETCURSEL, 1, 0);
        else if (cfg.templateName == L"minimal") SendMessage(s_hTplCombo, CB_SETCURSEL, 2, 0);
        else SendMessage(s_hTplCombo, CB_SETCURSEL, 0, 0);

        if (cfg.userMode == L"current") SendMessage(s_hUserCombo, CB_SETCURSEL, 1, 0);
        else if (cfg.userMode == L"none") SendMessage(s_hUserCombo, CB_SETCURSEL, 2, 0);
        else SendMessage(s_hUserCombo, CB_SETCURSEL, 0, 0);
    } else {
        SetWindowTextW(s_hStatusLabel, Loc("sched_status_off").c_str());
        SetWindowTextW(s_hBtnInstall, Loc("sched_btn_create").c_str());
        EnableWindow(s_hBtnRemove, FALSE);
    }
}

static LRESULT CALLBACK ScheduleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        DarkMode::ApplyToWindow(hWnd);
        SetWindowTextW(hWnd, Loc("sched_title").c_str());

        const auto hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        // Group / Instructions (Item 16)
        HWND hLbl = CreateWindowW(L"STATIC",
            Loc("sched_desc").c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 15, 440, 36, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLbl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        // Schedule Label & Combo (Item 16)
        HWND hLblFreq = CreateWindowW(L"STATIC", Loc("sched_freq").c_str(), WS_CHILD | WS_VISIBLE, 20, 65, 120, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblFreq, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hFreqCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 150, 63, 290, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hFreqCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_daily").c_str()));
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_logon").c_str()));
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_weekly").c_str()));
        SendMessage(s_hFreqCombo, CB_SETCURSEL, 0, 0);

        // Preset Label & Combo (Item 16)
        HWND hLblTpl = CreateWindowW(L"STATIC", Loc("sched_preset").c_str(), WS_CHILD | WS_VISIBLE, 20, 105, 120, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblTpl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hTplCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 150, 103, 290, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hTplCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_recommended").c_str()));
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_strict").c_str()));
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_minimal").c_str()));
        SendMessage(s_hTplCombo, CB_SETCURSEL, 0, 0);

        // Apply To (Item 16)
        HWND hLblUsers = CreateWindowW(L"STATIC", Loc("sched_users").c_str(), WS_CHILD | WS_VISIBLE, 20, 145, 120, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblUsers, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hUserCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 150, 143, 290, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hUserCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_all_users").c_str()));
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_current_user").c_str()));
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_machine_only").c_str()));
        SendMessage(s_hUserCombo, CB_SETCURSEL, 0, 0);

        // Status Label (Item 16)
        s_hStatusLabel = CreateWindowW(L"STATIC", Loc("sched_status_checking").c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 190, 420, 25, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hStatusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        // Buttons (Item 17)
        s_hBtnInstall = CreateWindowW(WC_BUTTONW, Loc("sched_btn_create").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 50, 230, 130, 30, hWnd, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        SendMessage(s_hBtnInstall, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hBtnRemove = CreateWindowW(WC_BUTTONW, Loc("sched_btn_remove").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 190, 230, 130, 30, hWnd, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
        SendMessage(s_hBtnRemove, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        HWND hBtnClose = CreateWindowW(WC_BUTTONW, Loc("sched_btn_close").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 330, 230, 100, 30, hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        SendMessage(hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        RefreshScheduleDialogState(hWnd);
        return 0;
    }
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == 1001) { // Create / Update
            ScheduledTaskConfig cfg;
            const int freqSel = static_cast<int>(SendMessage(s_hFreqCombo, CB_GETCURSEL, 0, 0));
            if (freqSel == 1) cfg.frequency = L"logon";
            else if (freqSel == 2) cfg.frequency = L"weekly";
            else cfg.frequency = L"daily";

            const int tplSel = static_cast<int>(SendMessage(s_hTplCombo, CB_GETCURSEL, 0, 0));
            if (tplSel == 1) cfg.templateName = L"strict";
            else if (tplSel == 2) cfg.templateName = L"minimal";
            else cfg.templateName = L"recommended";

            const int usrSel = static_cast<int>(SendMessage(s_hUserCombo, CB_GETCURSEL, 0, 0));
            if (usrSel == 1) cfg.userMode = L"current";
            else if (usrSel == 2) cfg.userMode = L"none";
            else cfg.userMode = L"all";

            const bool ok = TaskScheduler::InstallTask(cfg);
            if (ok) {
                RefreshScheduleDialogState(hWnd);
                MessageBoxW(hWnd, L"Scheduled Task configured successfully.\nSettings will be reapplied automatically according to the schedule.", Loc("sched_title").c_str(), MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, L"Failed to create scheduled task. Ensure PrivatizeWin is running as Administrator.", L"Error", MB_OK | MB_ICONERROR);
            }
        } else if (id == 1002) { // Remove
            const bool ok = TaskScheduler::UninstallTask();
            if (ok) {
                RefreshScheduleDialogState(hWnd);
                MessageBoxW(hWnd, L"Scheduled Task removed successfully.", Loc("sched_title").c_str(), MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, L"Could not remove task (it may not be currently installed, or administrator privileges are required).", L"Info", MB_OK | MB_ICONWARNING);
            }
        } else if (id == IDCANCEL) {
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

void ScheduleDialog::Show(HWND hParent) {
    const wchar_t CLASS_NAME[] = L"PrivatizeWin_ScheduleDlg";

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = ScheduleWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    RECT rcParent{};
    GetWindowRect(hParent, &rcParent);
    const int x = rcParent.left + (rcParent.right - rcParent.left - 480) / 2;
    const int y = rcParent.top + (rcParent.bottom - rcParent.top - 320) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        Loc("sched_title").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, 480, 320,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr
    );

    EnableWindow(hParent, FALSE);

    MSG msg{};
    while (IsWindow(hWnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (IsDialogMessageW(hWnd, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
}

} // namespace PrivatizeWin
