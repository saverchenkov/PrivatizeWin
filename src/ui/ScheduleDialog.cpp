#include "ScheduleDialog.h"
#include "DarkMode.h"
#include "../core/TaskScheduler.h"
#include "../core/Localization.h"
#include <commctrl.h>
#include <uxtheme.h>
#include <string>
#include <memory>

namespace PrivatizeWin {

struct ScheduleDlgState {
    HWND hFreqCombo = nullptr;
    HWND hTplCombo = nullptr;
    HWND hUserCombo = nullptr;
    HWND hStatusLabel = nullptr;
    HWND hBtnInstall = nullptr;
    HWND hBtnRemove = nullptr;
    HFONT hFont = nullptr;
    HBRUSH hBgBrush = nullptr;
    std::wstring customTemplatePath;
    std::wstring customUserList;
};

static int ScaleDpi(int val, UINT dpi) {
    return MulDiv(val, static_cast<int>(dpi), 96);
}

static void RefreshScheduleDialogState(ScheduleDlgState* st) {
    if (!st) return;

    ScheduledTaskConfig cfg;
    const bool installed = TaskScheduler::GetTaskConfig(cfg);

    if (installed) {
        std::wstring freqStr;
        if (cfg.frequency == L"logon") freqStr = Loc("sched_logon");
        else if (cfg.frequency == L"weekly") freqStr = Loc("sched_weekly");
        else freqStr = Loc("sched_daily");

        std::wstring statusStr = LocFmt("sched_status_active", { freqStr });
        SetWindowTextW(st->hStatusLabel, statusStr.c_str());

        SetWindowTextW(st->hBtnInstall, Loc("sched_btn_update").c_str());
        EnableWindow(st->hBtnRemove, TRUE);

        // Select frequency
        if (cfg.frequency == L"logon") SendMessageW(st->hFreqCombo, CB_SETCURSEL, 1, 0);
        else if (cfg.frequency == L"weekly") SendMessageW(st->hFreqCombo, CB_SETCURSEL, 2, 0);
        else SendMessageW(st->hFreqCombo, CB_SETCURSEL, 0, 0);

        // Select template (preserving custom JSON path if present)
        if (cfg.templateName == L"recommended" || cfg.templateName.empty()) {
            SendMessageW(st->hTplCombo, CB_SETCURSEL, 0, 0);
        } else if (cfg.templateName == L"strict") {
            SendMessageW(st->hTplCombo, CB_SETCURSEL, 1, 0);
        } else if (cfg.templateName == L"minimal") {
            SendMessageW(st->hTplCombo, CB_SETCURSEL, 2, 0);
        } else {
            st->customTemplatePath = cfg.templateName;
            const LRESULT count = SendMessageW(st->hTplCombo, CB_GETCOUNT, 0, 0);
            if (count > 3) {
                SendMessageW(st->hTplCombo, CB_DELETESTRING, 3, 0);
            }
            SendMessageW(st->hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(st->customTemplatePath.c_str()));
            SendMessageW(st->hTplCombo, CB_SETCURSEL, 3, 0);
        }

        // Select user mode (preserving custom user list if present)
        if (cfg.userMode == L"all" || cfg.userMode.empty()) {
            SendMessageW(st->hUserCombo, CB_SETCURSEL, 0, 0);
        } else if (cfg.userMode == L"current") {
            SendMessageW(st->hUserCombo, CB_SETCURSEL, 1, 0);
        } else if (cfg.userMode == L"none") {
            SendMessageW(st->hUserCombo, CB_SETCURSEL, 2, 0);
        } else {
            st->customUserList = cfg.userMode;
            const LRESULT count = SendMessageW(st->hUserCombo, CB_GETCOUNT, 0, 0);
            if (count > 3) {
                SendMessageW(st->hUserCombo, CB_DELETESTRING, 3, 0);
            }
            SendMessageW(st->hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(st->customUserList.c_str()));
            SendMessageW(st->hUserCombo, CB_SETCURSEL, 3, 0);
        }
    } else {
        SetWindowTextW(st->hStatusLabel, Loc("sched_status_off").c_str());
        SetWindowTextW(st->hBtnInstall, Loc("sched_btn_create").c_str());
        EnableWindow(st->hBtnRemove, FALSE);
    }
}

static LRESULT CALLBACK ScheduleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto* st = reinterpret_cast<ScheduleDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_CREATE: {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        st = static_cast<ScheduleDlgState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));

        DarkMode::ApplyToWindow(hWnd);
        SetWindowTextW(hWnd, Loc("sched_title").c_str());

        UINT dpi = GetDpiForWindow(hWnd);
        if (dpi == 0) dpi = 96;

        NONCLIENTMETRICSW ncm{ sizeof(ncm) };
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            ncm.lfMessageFont.lfHeight = -ScaleDpi(9, dpi);
            st->hFont = CreateFontIndirectW(&ncm.lfMessageFont);
        }
        if (!st->hFont) {
            st->hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        }

        if (DarkMode::IsDarkModeActive()) {
            DarkMode::ApplyToWindow(hWnd);
            st->hBgBrush = CreateSolidBrush(RGB(32, 32, 32));
        }

        // Description label
        HWND hLbl = CreateWindowW(L"STATIC",
            Loc("sched_desc").c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            ScaleDpi(20, dpi), ScaleDpi(15, dpi), ScaleDpi(425, dpi), ScaleDpi(38, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(hLbl, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        // Schedule Label & Combo
        HWND hLblFreq = CreateWindowW(L"STATIC", Loc("sched_freq").c_str(), WS_CHILD | WS_VISIBLE,
            ScaleDpi(20, dpi), ScaleDpi(66, dpi), ScaleDpi(120, dpi), ScaleDpi(22, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(hLblFreq, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        st->hFreqCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
            ScaleDpi(145, dpi), ScaleDpi(63, dpi), ScaleDpi(295, dpi), ScaleDpi(200, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(st->hFreqCombo, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);
        SendMessageW(st->hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_daily").c_str()));
        SendMessageW(st->hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_logon").c_str()));
        SendMessageW(st->hFreqCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_weekly").c_str()));
        SendMessageW(st->hFreqCombo, CB_SETCURSEL, 0, 0);

        // Preset Label & Combo
        HWND hLblTpl = CreateWindowW(L"STATIC", Loc("sched_preset").c_str(), WS_CHILD | WS_VISIBLE,
            ScaleDpi(20, dpi), ScaleDpi(106, dpi), ScaleDpi(120, dpi), ScaleDpi(22, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(hLblTpl, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        st->hTplCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
            ScaleDpi(145, dpi), ScaleDpi(103, dpi), ScaleDpi(295, dpi), ScaleDpi(200, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(st->hTplCombo, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);
        SendMessageW(st->hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_recommended").c_str()));
        SendMessageW(st->hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_strict").c_str()));
        SendMessageW(st->hTplCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_minimal").c_str()));
        SendMessageW(st->hTplCombo, CB_SETCURSEL, 0, 0);

        // Apply To Label & Combo
        HWND hLblUsers = CreateWindowW(L"STATIC", Loc("sched_users").c_str(), WS_CHILD | WS_VISIBLE,
            ScaleDpi(20, dpi), ScaleDpi(146, dpi), ScaleDpi(120, dpi), ScaleDpi(22, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(hLblUsers, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        st->hUserCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
            ScaleDpi(145, dpi), ScaleDpi(143, dpi), ScaleDpi(295, dpi), ScaleDpi(200, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(st->hUserCombo, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);
        SendMessageW(st->hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_all_users").c_str()));
        SendMessageW(st->hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_current_user").c_str()));
        SendMessageW(st->hUserCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("sched_machine_only").c_str()));
        SendMessageW(st->hUserCombo, CB_SETCURSEL, 0, 0);

        // Status Label
        st->hStatusLabel = CreateWindowW(L"STATIC", Loc("sched_status_checking").c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
            ScaleDpi(20, dpi), ScaleDpi(188, dpi), ScaleDpi(420, dpi), ScaleDpi(24, dpi),
            hWnd, nullptr, nullptr, nullptr);
        SendMessageW(st->hStatusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        // Action Buttons
        st->hBtnInstall = CreateWindowW(WC_BUTTONW, Loc("sched_btn_create").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            ScaleDpi(40, dpi), ScaleDpi(226, dpi), ScaleDpi(130, dpi), ScaleDpi(30, dpi),
            hWnd, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        SendMessageW(st->hBtnInstall, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        st->hBtnRemove = CreateWindowW(WC_BUTTONW, Loc("sched_btn_remove").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            ScaleDpi(180, dpi), ScaleDpi(226, dpi), ScaleDpi(130, dpi), ScaleDpi(30, dpi),
            hWnd, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
        SendMessageW(st->hBtnRemove, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        HWND hBtnClose = CreateWindowW(WC_BUTTONW, Loc("sched_btn_close").c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            ScaleDpi(320, dpi), ScaleDpi(226, dpi), ScaleDpi(120, dpi), ScaleDpi(30, dpi),
            hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        SendMessageW(hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(st->hFont), TRUE);

        if (DarkMode::IsDarkModeActive()) {
            SetWindowTheme(st->hFreqCombo, L"DarkMode_CFD", nullptr);
            SetWindowTheme(st->hTplCombo, L"DarkMode_CFD", nullptr);
            SetWindowTheme(st->hUserCombo, L"DarkMode_CFD", nullptr);
            SetWindowTheme(st->hBtnInstall, L"DarkMode_Explorer", nullptr);
            SetWindowTheme(st->hBtnRemove, L"DarkMode_Explorer", nullptr);
            SetWindowTheme(hBtnClose, L"DarkMode_Explorer", nullptr);
        }

        RefreshScheduleDialogState(st);
        return 0;
    }
    case WM_ERASEBKGND: {
        if (st && st->hBgBrush) {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rc{};
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, st->hBgBrush);
            return 1;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        if (st && st->hBgBrush) {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(235, 235, 235));
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(st->hBgBrush);
        }
        break;
    }
    case WM_COMMAND: {
        if (!st) return 0;
        const int id = LOWORD(wParam);
        if (id == 1001) { // Create / Update
            ScheduledTaskConfig cfg;
            const int freqSel = static_cast<int>(SendMessageW(st->hFreqCombo, CB_GETCURSEL, 0, 0));
            if (freqSel == 1) cfg.frequency = L"logon";
            else if (freqSel == 2) cfg.frequency = L"weekly";
            else cfg.frequency = L"daily";

            const int tplSel = static_cast<int>(SendMessageW(st->hTplCombo, CB_GETCURSEL, 0, 0));
            if (tplSel == 1) cfg.templateName = L"strict";
            else if (tplSel == 2) cfg.templateName = L"minimal";
            else if (tplSel == 3 && !st->customTemplatePath.empty()) cfg.templateName = st->customTemplatePath;
            else cfg.templateName = L"recommended";

            const int usrSel = static_cast<int>(SendMessageW(st->hUserCombo, CB_GETCURSEL, 0, 0));
            if (usrSel == 1) cfg.userMode = L"current";
            else if (usrSel == 2) cfg.userMode = L"none";
            else if (usrSel == 3 && !st->customUserList.empty()) cfg.userMode = st->customUserList;
            else cfg.userMode = L"all";

            const bool ok = TaskScheduler::InstallTask(cfg);
            if (ok) {
                RefreshScheduleDialogState(st);
                MessageBoxW(hWnd, Loc("sched_msg_created").c_str(), Loc("sched_title").c_str(), MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, Loc("sched_msg_create_fail").c_str(), Loc("sched_title").c_str(), MB_OK | MB_ICONERROR);
            }
        } else if (id == 1002) { // Remove
            const bool ok = TaskScheduler::UninstallTask();
            if (ok) {
                RefreshScheduleDialogState(st);
                MessageBoxW(hWnd, Loc("sched_msg_removed").c_str(), Loc("sched_title").c_str(), MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, Loc("sched_msg_remove_fail").c_str(), Loc("sched_title").c_str(), MB_OK | MB_ICONWARNING);
            }
        } else if (id == IDCANCEL) {
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        if (st) {
            if (st->hFont && st->hFont != GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(st->hFont);
                st->hFont = nullptr;
            }
            if (st->hBgBrush) {
                DeleteObject(st->hBgBrush);
                st->hBgBrush = nullptr;
            }
        }
        return 0;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
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

    UINT dpi = GetDpiForWindow(hParent);
    if (dpi == 0) dpi = 96;

    const int dlgW = ScaleDpi(475, dpi);
    const int dlgH = ScaleDpi(310, dpi);

    RECT rcParent{};
    GetWindowRect(hParent, &rcParent);
    const int x = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    const int y = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    auto state = std::make_unique<ScheduleDlgState>();

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        Loc("sched_title").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH,
        hParent, nullptr, GetModuleHandle(nullptr), state.get()
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

