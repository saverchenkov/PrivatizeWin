#include "ScheduleDialog.h"
#include "DarkMode.h"
#include "../core/TaskScheduler.h"
#include <commctrl.h>
#include <string>

namespace PrivatizeWin {

static HWND s_hFreqCombo = nullptr;
static HWND s_hTplCombo = nullptr;
static HWND s_hUserCombo = nullptr;
static HWND s_hStatusLabel = nullptr;

static LRESULT CALLBACK ScheduleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        DarkMode::ApplyToWindow(hWnd);

        const auto hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        // Group / Instructions
        HWND hLbl = CreateWindowW(L"STATIC",
            L"Configure PrivatizeWin to automatically reapply your chosen privacy template in the background.\n"
            L"This ensures Windows Updates or system telemetry resets cannot undo your privacy settings.",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 15, 440, 40, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLbl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        // Frequency Label & Combo
        HWND hLblFreq = CreateWindowW(L"STATIC", L"Reapplication Frequency:", WS_CHILD | WS_VISIBLE, 20, 70, 180, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblFreq, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hFreqCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 200, 68, 220, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hFreqCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Daily (12:00 PM)"));
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"At User Logon"));
        SendMessage(s_hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weekly (Sunday 12:00 PM)"));
        SendMessage(s_hFreqCombo, CB_SETCURSEL, 0, 0);

        // Template Label & Combo
        HWND hLblTpl = CreateWindowW(L"STATIC", L"Privacy Template:", WS_CHILD | WS_VISIBLE, 20, 110, 180, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblTpl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hTplCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 200, 108, 220, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hTplCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"recommended"));
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"strict"));
        SendMessage(s_hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"minimal"));
        SendMessage(s_hTplCombo, CB_SETCURSEL, 0, 0);

        // Target Users
        HWND hLblUsers = CreateWindowW(L"STATIC", L"Target User Hives:", WS_CHILD | WS_VISIBLE, 20, 150, 180, 20, hWnd, nullptr, nullptr, nullptr);
        SendMessage(hLblUsers, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        s_hUserCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 200, 148, 220, 200, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hUserCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All User Profiles (Recommended)"));
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Current User Only"));
        SendMessage(s_hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Machine Only (HKLM)"));
        SendMessage(s_hUserCombo, CB_SETCURSEL, 0, 0);

        // Status Label
        s_hStatusLabel = CreateWindowW(L"STATIC", L"Current Status: Checking...", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 195, 400, 25, hWnd, nullptr, nullptr, nullptr);
        SendMessage(s_hStatusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        // Buttons
        HWND hBtnInstall = CreateWindowW(WC_BUTTONW, L"Enable Scheduled Task", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 60, 235, 170, 30, hWnd, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        SendMessage(hBtnInstall, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        HWND hBtnRemove = CreateWindowW(WC_BUTTONW, L"Remove Task", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 240, 235, 110, 30, hWnd, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
        SendMessage(hBtnRemove, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        HWND hBtnClose = CreateWindowW(WC_BUTTONW, L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 360, 235, 80, 30, hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        SendMessage(hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        const bool installed = TaskScheduler::IsTaskInstalled();
        SetWindowTextW(s_hStatusLabel, installed ? L"Current Status: Active (Running automatically in background)" : L"Current Status: Not Scheduled");
        return 0;
    }
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == 1001) { // Install
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
                SetWindowTextW(s_hStatusLabel, L"Current Status: Active (Task registered successfully)");
                MessageBoxW(hWnd, L"Scheduled Task 'PrivatizeWin Auto-Protect' successfully configured!\nYour privacy settings will be reapplied automatically.", L"PrivatizeWin Scheduler", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, L"Failed to create scheduled task. Please make sure PrivatizeWin is running as Administrator.", L"Error", MB_OK | MB_ICONERROR);
            }
        } else if (id == 1002) { // Remove
            const bool ok = TaskScheduler::UninstallTask();
            if (ok) {
                SetWindowTextW(s_hStatusLabel, L"Current Status: Not Scheduled (Task removed)");
                MessageBoxW(hWnd, L"Scheduled Task removed successfully.", L"PrivatizeWin Scheduler", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, L"Could not remove task (it may not be currently installed).", L"Info", MB_OK | MB_ICONINFORMATION);
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
        L"Automated Background Reapplication",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, 480, 320,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr
    );

    EnableWindow(hParent, FALSE);

    MSG msg{};
    while (IsWindow(hWnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
}

} // namespace PrivatizeWin
