#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "MainWindow.h"
#include "DarkMode.h"
#include "ScheduleDialog.h"
#include "../../res/resource.h"
#include "../core/TweakRegistry.h"
#include "../core/TemplateManager.h"
#include "../core/RestorePoint.h"
#include "../core/ProcessHelper.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <richedit.h>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace PrivatizeWin {

static std::unique_ptr<MainWindow> s_pMainWnd = nullptr;

// Subclass procedure for search box to support Esc to clear (Item 15)
static LRESULT CALLBACK SearchSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        SetWindowTextW(hWnd, L"");
        auto* pMain = reinterpret_cast<MainWindow*>(dwRefData);
        if (pMain) {
            HWND hList = FindWindowExW(GetParent(hWnd), nullptr, WC_LISTVIEWW, nullptr);
            if (hList) SetFocus(hList);
        }
        return 0;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

bool MainWindow::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PrivatizeWin_MainWindow";
    wc.hbrBackground = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.lpszMenuName = L"MAINMENU";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    return (RegisterClassExW(&wc) != 0);
}

HWND MainWindow::Create(HINSTANCE hInstance) {
    return CreateWindowExW(
        WS_EX_WINDOWEDGE,
        L"PrivatizeWin_MainWindow",
        L"PrivatizeWin \u2014 Windows Privacy Settings",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 720,
        nullptr, nullptr, hInstance, nullptr
    );
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        pThis = new MainWindow(hWnd);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (uMsg) {
        case WM_CREATE:
            pThis->OnCreate();
            return 0;
        case WM_SIZE:
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_GETMINMAXINFO: {
            auto* pMMI = reinterpret_cast<MINMAXINFO*>(lParam);
            pMMI->ptMinTrackSize.x = 860;
            pMMI->ptMinTrackSize.y = 560;
            return 0;
        }
        case WM_COMMAND:
            pThis->OnCommand(LOWORD(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_NOTIFY:
            pThis->OnNotify(reinterpret_cast<NMHDR*>(lParam));
            return 0;
        case WM_CONTEXTMENU:
            pThis->OnContextMenu(reinterpret_cast<HWND>(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_KEYDOWN:
            pThis->OnKeyDown(wParam);
            return 0;
        case WM_LBUTTONDOWN:
            pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            pThis->OnLButtonUp();
            return 0;
        case WM_MOUSEMOVE:
            pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_SETCURSOR: {
            HWND hTarget = reinterpret_cast<HWND>(wParam);
            if (hTarget == hWnd) {
                POINT pt{};
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                if (pt.y >= pThis->m_splitterY - 3 && pt.y <= pThis->m_splitterY + 5) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    return TRUE;
                }
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

            // Draw a subtle separator line for the splitter bar (Item 13)
            RECT rcSplitter = { 10, pThis->m_splitterY, rc.right - 10, pThis->m_splitterY + 1 };
            FillRect(hdc, &rcSplitter, GetSysColorBrush(COLOR_3DSHADOW));

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            pThis->SavePreferences();
            PostQuitMessage(0);
            return 0;
        case WM_NCDESTROY:
            delete pThis;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            return 0;
        default:
            break;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {
    m_hRichEditLib = LoadLibraryW(L"msftedit.dll");
}

MainWindow::~MainWindow() {
    if (m_hFontRegular) DeleteObject(m_hFontRegular);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontCode) DeleteObject(m_hFontCode);
    if (m_hRichEditLib) FreeLibrary(m_hRichEditLib);
}

void MainWindow::InitializeFonts() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    m_hFontRegular = CreateFontIndirectW(&ncm.lfMessageFont);

    LOGFONTW lfBold = ncm.lfMessageFont;
    lfBold.lfWeight = FW_BOLD;
    m_hFontBold = CreateFontIndirectW(&lfBold);

    LOGFONTW lfTitle = ncm.lfMessageFont;
    lfTitle.lfHeight = static_cast<LONG>(lfTitle.lfHeight * 1.25);
    lfTitle.lfWeight = FW_SEMIBOLD;
    m_hFontTitle = CreateFontIndirectW(&lfTitle);

    LOGFONTW lfCode = ncm.lfMessageFont;
    wcscpy_s(lfCode.lfFaceName, L"Consolas");
    lfCode.lfHeight = static_cast<LONG>(lfCode.lfHeight * 0.95);
    m_hFontCode = CreateFontIndirectW(&lfCode);
}

void MainWindow::LoadPreferences() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PrivatizeWin", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSplitter = 0;
        DWORD dwSize = sizeof(dwSplitter);
        if (RegQueryValueExW(hKey, L"SplitterY", nullptr, nullptr, reinterpret_cast<LPBYTE>(&dwSplitter), &dwSize) == ERROR_SUCCESS) {
            if (dwSplitter >= 180 && dwSplitter <= 1000) {
                m_splitterY = static_cast<int>(dwSplitter);
            }
        }

        // Load column widths (Item 12)
        if (m_hListView) {
            for (int col = 0; col < 4; ++col) {
                std::wstring valName = L"ColWidth" + std::to_wstring(col);
                DWORD dwWidth = 0;
                dwSize = sizeof(dwWidth);
                if (RegQueryValueExW(hKey, valName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&dwWidth), &dwSize) == ERROR_SUCCESS) {
                    if (dwWidth >= 40 && dwWidth <= 1200) {
                        ListView_SetColumnWidth(m_hListView, col, dwWidth);
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }
}

void MainWindow::SavePreferences() {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PrivatizeWin", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD dwSplitter = static_cast<DWORD>(m_splitterY);
        RegSetValueExW(hKey, L"SplitterY", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwSplitter), sizeof(dwSplitter));

        // Save column widths (Item 12)
        if (m_hListView) {
            for (int col = 0; col < 4; ++col) {
                std::wstring valName = L"ColWidth" + std::to_wstring(col);
                DWORD dwWidth = static_cast<DWORD>(ListView_GetColumnWidth(m_hListView, col));
                RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwWidth), sizeof(dwWidth));
            }
        }
        RegCloseKey(hKey);
    }
}

void MainWindow::OnCreate() {
    DarkMode::ApplyToWindow(m_hWnd);

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    InitializeFonts();
    InitializeControls();

    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    LoadPreferences();

    // Truthful window title (Item 18 & elevation indicator)
    std::wstring title = L"PrivatizeWin \u2014 Windows Privacy Settings";
    if (IsRunningAsAdmin()) {
        title += L" [Administrator]";
    } else {
        title += L" [Standard User - Read Only]";
    }
    SetWindowTextW(m_hWnd, title.c_str());

    PopulateListView(L"", FilterMode::All);
    UpdateStatusBar();
}

void MainWindow::InitializeControls() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 1. Search Box (Item 15: Filter tweaks Ctrl+F)
    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        10, 10, 160, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_SEARCH_EDIT), hInst, nullptr);
    SendMessage(m_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter tweaks (Ctrl+F)..."));
    SetWindowSubclass(m_hSearchEdit, SearchSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

    // 2. Filter Dropdown (Item 15: Labeled by what it filters)
    m_hFilterCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        176, 10, 130, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_FILTER_COMBO), hInst, nullptr);
    SendMessage(m_hFilterCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All settings"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Not applied only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Applied only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended only"));
    SendMessage(m_hFilterCombo, CB_SETCURSEL, 0, 0);

    // 3. Match / Hidden Count Label (Item 3)
    m_hLblMatchCount = CreateWindowW(WC_STATICW, L"260 shown",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        312, 13, 140, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_LBL_MATCH_COUNT), hInst, nullptr);
    SendMessage(m_hLblMatchCount, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 4. Explicit Selection Menu Button (Item 2)
    m_hBtnSelectMenu = CreateWindowW(WC_BUTTONW, L"Select \u25BC",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        458, 10, 85, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SELECT_MENU), hInst, nullptr);
    SendMessage(m_hBtnSelectMenu, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 5. Preset Dropdown & Explicit "Select Preset" Button (Item 2)
    m_hTemplateCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        550, 10, 130, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_TPL_COMBO), hInst, nullptr);
    SendMessage(m_hTemplateCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strict Privacy"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal"));
    SendMessage(m_hTemplateCombo, CB_SETCURSEL, 0, 0);

    m_hBtnSelectPreset = CreateWindowW(WC_BUTTONW, L"Select Preset",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        686, 10, 95, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SELECT_PRESET), hInst, nullptr);
    SendMessage(m_hBtnSelectPreset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 6. Action Buttons (Item 3: Apply Selected (N), Item 6: Restore Defaults (N)...)
    m_hBtnApply = CreateWindowW(WC_BUTTONW, L"Apply Selected (0)",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_DISABLED,
        788, 10, 140, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_APPLY), hInst, nullptr);
    SendMessage(m_hBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hBtnRevert = CreateWindowW(WC_BUTTONW, L"Restore Defaults\u2026",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP | WS_DISABLED,
        934, 10, 130, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REVERT), hInst, nullptr);
    SendMessage(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 7. Grouped ListView (SysListView32)
    m_hListView = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 46, 1040, 360,
        m_hWnd,
        reinterpret_cast<HMENU>(IDC_LIST_TWEAKS),
        hInst,
        nullptr
    );
    SendMessage(m_hListView, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // Modern list view styles with checkboxes for selection (Item 1)
    ListView_SetExtendedListViewStyle(m_hListView,
        LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Columns: Setting (flex), Status (105), Impact (90), Scope (130) (Item 12 & 18)
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = const_cast<LPWSTR>(L"Setting");
    col.cx = 640;
    col.iSubItem = 0;
    ListView_InsertColumn(m_hListView, 0, &col);

    col.pszText = const_cast<LPWSTR>(L"Status");
    col.cx = 105;
    col.iSubItem = 1;
    ListView_InsertColumn(m_hListView, 1, &col);

    col.pszText = const_cast<LPWSTR>(L"Impact");
    col.cx = 90;
    col.iSubItem = 2;
    ListView_InsertColumn(m_hListView, 2, &col);

    col.pszText = const_cast<LPWSTR>(L"Scope");
    col.cx = 130;
    col.iSubItem = 3;
    ListView_InsertColumn(m_hListView, 3, &col);

    // 8. Inspector Details Pane (Native RichEdit 5.0)
    m_hDetailsEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, 440, 1040, 200,
        m_hWnd,
        reinterpret_cast<HMENU>(IDC_EDIT_DETAILS),
        hInst,
        nullptr
    );
    SendMessage(m_hDetailsEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hDetailsEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(8, 8));

    // Per-Setting Actions (Item 11: Apply Setting / Restore Setting)
    m_hBtnToggleTweak = CreateWindowW(WC_BUTTONW, L"Apply Setting",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        10, 412, 130, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_DETAILS_BTN_TOGGLE), hInst, nullptr);
    SendMessage(m_hBtnToggleTweak, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hBtnCopyTweak = CreateWindowW(WC_BUTTONW, L"Copy Technical Details",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        146, 412, 160, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_DETAILS_BTN_COPY), hInst, nullptr);
    SendMessage(m_hBtnCopyTweak, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 9. Status Bar (Item 14: Factual counts)
    m_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), hInst, nullptr);
    SendMessage(m_hStatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    int parts[3] = { 380, 560, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));

    // Progress bar (only visible during operations)
    m_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        WS_CHILD | PBS_SMOOTH,
        382, 2, 174, 18, m_hStatusBar, reinterpret_cast<HMENU>(IDC_STATUS_PROGRESS), hInst, nullptr);
}

void MainWindow::OnSize(int width, int height) {
    if (width <= 0 || height <= 0) return;

    SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

    RECT rcStatus{};
    GetWindowRect(m_hStatusBar, &rcStatus);
    const int statusH = rcStatus.bottom - rcStatus.top;

    // Adjust parts based on width
    int part0 = std::max(360, width - 420);
    int part1 = part0 + 160;
    int parts[3] = { part0, part1, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));

    // Toolbar layout
    const int topMargin = 10;
    const int topH = 34;

    // Position top row controls
    SetWindowPos(m_hSearchEdit, nullptr, 10, topMargin, 160, 26, SWP_NOZORDER);
    SetWindowPos(m_hFilterCombo, nullptr, 176, topMargin, 130, 200, SWP_NOZORDER);
    SetWindowPos(m_hLblMatchCount, nullptr, 312, topMargin + 3, 135, 20, SWP_NOZORDER);
    SetWindowPos(m_hBtnSelectMenu, nullptr, 452, topMargin, 85, 26, SWP_NOZORDER);
    SetWindowPos(m_hTemplateCombo, nullptr, 543, topMargin, 125, 200, SWP_NOZORDER);
    SetWindowPos(m_hBtnSelectPreset, nullptr, 674, topMargin, 92, 26, SWP_NOZORDER);

    const int rightEdge = width - 10;
    const int btnRevertW = 135;
    const int btnApplyW = 145;
    SetWindowPos(m_hBtnRevert, nullptr, rightEdge - btnRevertW, topMargin, btnRevertW, 26, SWP_NOZORDER);
    SetWindowPos(m_hBtnApply, nullptr, rightEdge - btnRevertW - 8 - btnApplyW, topMargin, btnApplyW, 26, SWP_NOZORDER);

    // Calculate vertical layout using adjustable splitter (Item 13)
    const int minListH = 180;
    const int minDetailsH = 120;

    if (m_splitterY < minListH + topH) {
        m_splitterY = minListH + topH;
    } else if (m_splitterY > height - statusH - minDetailsH - 40) {
        m_splitterY = height - statusH - minDetailsH - 40;
    }

    const int listTop = topH + 12;
    const int listH = std::max(minListH, m_splitterY - listTop);
    SetWindowPos(m_hListView, nullptr, 10, listTop, width - 20, listH, SWP_NOZORDER);

    // Adjust list column 0 to fill available width
    if (m_hListView) {
        const int fixedCols = ListView_GetColumnWidth(m_hListView, 1) +
                              ListView_GetColumnWidth(m_hListView, 2) +
                              ListView_GetColumnWidth(m_hListView, 3);
        const int col0Width = std::max(280, width - 20 - fixedCols - GetSystemMetrics(SM_CXVSCROLL) - 4);
        ListView_SetColumnWidth(m_hListView, 0, col0Width);
    }

    // Details pane and buttons below splitter
    const int detailsBtnsTop = m_splitterY + 8;
    SetWindowPos(m_hBtnToggleTweak, nullptr, 10, detailsBtnsTop, 130, 26, SWP_NOZORDER);
    SetWindowPos(m_hBtnCopyTweak, nullptr, 146, detailsBtnsTop, 160, 26, SWP_NOZORDER);

    const int editTop = detailsBtnsTop + 32;
    const int editH = std::max(minDetailsH, height - statusH - editTop - 6);
    SetWindowPos(m_hDetailsEdit, nullptr, 10, editTop, width - 20, editH, SWP_NOZORDER);

    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void MainWindow::OnLButtonDown(int /*x*/, int y) {
    if (y >= m_splitterY - 3 && y <= m_splitterY + 5) {
        m_isDraggingSplitter = true;
        SetCapture(m_hWnd);
    }
}

void MainWindow::OnLButtonUp() {
    if (m_isDraggingSplitter) {
        m_isDraggingSplitter = false;
        ReleaseCapture();
        SavePreferences();
    }
}

void MainWindow::OnMouseMove(int /*x*/, int y) {
    if (m_isDraggingSplitter) {
        RECT rc;
        GetClientRect(m_hWnd, &rc);
        const int minTop = 220;
        const int maxBottom = rc.bottom - 180;
        if (y >= minTop && y <= maxBottom) {
            m_splitterY = y;
            OnSize(rc.right - rc.left, rc.bottom - rc.top);
        }
    }
}

void MainWindow::PopulateListView(std::wstring_view searchFilter, FilterMode filterMode) {
    SendMessage(m_hListView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(m_hListView);
    ListView_RemoveAllGroups(m_hListView);

    const auto& catalog = TweakRegistry::Instance().GetAllTweaks();
    const auto categories = TweakRegistry::Instance().GetCategories();

    // 1. Insert Category Groups
    ListView_EnableGroupView(m_hListView, TRUE);

    std::unordered_map<std::wstring, int> categoryToGroupId;
    int groupId = 1;
    for (const auto& cat : categories) {
        if (cat.name == L"All Settings") continue;

        LVGROUP group{};
        group.cbSize = sizeof(group);
        group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
        group.stateMask = LVGS_COLLAPSIBLE;
        group.state = LVGS_COLLAPSIBLE;
        group.iGroupId = groupId;
        group.pszHeader = const_cast<LPWSTR>(cat.name.c_str());

        ListView_InsertGroup(m_hListView, -1, &group);
        categoryToGroupId[cat.name] = groupId;
        groupId++;
    }

    // 2. Populate Tweaks into their Groups
    m_displayedTweaks.clear();

    std::wstring lowerSearch(searchFilter);
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::towlower);

    int itemIndex = 0;
    for (const auto& t : catalog) {
        // Filter logic
        if (!lowerSearch.empty()) {
            std::wstring lowerTitle = t.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::towlower);
            std::wstring lowerDesc = t.description;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::towlower);
            std::wstring wideId(t.id.begin(), t.id.end());
            std::transform(wideId.begin(), wideId.end(), wideId.begin(), ::towlower);
            if (lowerTitle.find(lowerSearch) == std::wstring::npos &&
                lowerDesc.find(lowerSearch) == std::wstring::npos &&
                wideId.find(lowerSearch) == std::wstring::npos) {
                continue;
            }
        }

        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        const bool isApplied = (st == SettingStatus::Applied);

        if (filterMode == FilterMode::RecommendedOnly && !t.isRecommended) continue;
        if (filterMode == FilterMode::NotAppliedOnly && isApplied) continue;
        if (filterMode == FilterMode::AppliedOnly && !isApplied) continue;

        m_displayedTweaks.push_back(t);

        LVITEMW lvi{};
        lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM;
        lvi.iItem = itemIndex;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(t.title.c_str());
        lvi.lParam = itemIndex;

        auto it = categoryToGroupId.find(t.category);
        if (it != categoryToGroupId.end()) {
            lvi.iGroupId = it->second;
        }

        ListView_InsertItem(m_hListView, &lvi);

        // Status text (Item 4: Applied, Not applied, Partial, Unknown, Not applicable)
        std::wstring stStr;
        switch (st) {
        case SettingStatus::Applied:       stStr = L"\u25CF Applied"; break;
        case SettingStatus::NotApplied:    stStr = L"\u25CB Not applied"; break;
        case SettingStatus::Partial:       stStr = L"\u25D0 Partial"; break;
        case SettingStatus::Unknown:       stStr = L"? Unknown"; break;
        case SettingStatus::NotApplicable: stStr = L"\u2014 Not applicable"; break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 1, const_cast<LPWSTR>(stStr.c_str()));

        // Impact text (Item 5: Low, Moderate, High)
        std::wstring impactStr;
        switch (t.impactLevel) {
        case ImpactLevel::Low:      impactStr = L"Low"; break;
        case ImpactLevel::Moderate: impactStr = L"Moderate"; break;
        case ImpactLevel::High:     impactStr = L"High"; break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 2, const_cast<LPWSTR>(impactStr.c_str()));

        // Scope text (Item 12: User, Machine, or Machine & user)
        std::wstring scopeStr;
        switch (t.scope) {
        case TargetScope::Machine: scopeStr = L"Machine"; break;
        case TargetScope::User:    scopeStr = L"User"; break;
        case TargetScope::Both:    scopeStr = L"Machine & user"; break;
        case TargetScope::Service: scopeStr = L"Machine (Service)"; break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 3, const_cast<LPWSTR>(scopeStr.c_str()));

        // Checkbox represents SELECTION for bulk action (Item 1 & 15: stable identity)
        const bool isSelected = (m_selectedTweakIds.count(t.id) > 0);
        ListView_SetCheckState(m_hListView, itemIndex, isSelected ? TRUE : FALSE);

        itemIndex++;
    }

    SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hListView, nullptr, TRUE);

    // Sync Details Pane reliably (Item 9)
    if (!m_displayedTweaks.empty()) {
        ListView_SetItemState(m_hListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        UpdateDetailsPane(0);
    } else {
        SetWindowTextW(m_hDetailsEdit, L"No settings match the current filter.");
        EnableWindow(m_hBtnToggleTweak, FALSE);
        EnableWindow(m_hBtnCopyTweak, FALSE);
    }

    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::UpdateSelectionCounts() {
    const size_t totalSelected = m_selectedTweakIds.size();

    size_t visibleSelected = 0;
    for (const auto& t : m_displayedTweaks) {
        if (m_selectedTweakIds.count(t.id) > 0) {
            visibleSelected++;
        }
    }
    const size_t hiddenSelected = totalSelected - visibleSelected;

    // Item 3: Update Apply button text and state
    std::wstring applyText = L"Apply Selected (" + std::to_wstring(totalSelected) + L")";
    SetWindowTextW(m_hBtnApply, applyText.c_str());
    EnableWindow(m_hBtnApply, totalSelected > 0);

    // Item 6: Update Restore Defaults button text and state
    std::wstring revertText = totalSelected > 0 ? (L"Restore Defaults (" + std::to_wstring(totalSelected) + L")\u2026") : L"Restore Defaults\u2026";
    SetWindowTextW(m_hBtnRevert, revertText.c_str());
    EnableWindow(m_hBtnRevert, totalSelected > 0);

    // Item 3: Update match / hidden count label
    std::wstring countStr = std::to_wstring(m_displayedTweaks.size()) + L" shown";
    if (hiddenSelected > 0) {
        countStr += L" \u00B7 " + std::to_wstring(hiddenSelected) + L" hidden";
    }
    SetWindowTextW(m_hLblMatchCount, countStr.c_str());
}

void MainWindow::UpdateDetailsPane(int selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(m_displayedTweaks.size())) {
        SetWindowTextW(m_hDetailsEdit, L"");
        EnableWindow(m_hBtnToggleTweak, FALSE);
        EnableWindow(m_hBtnCopyTweak, FALSE);
        return;
    }

    EnableWindow(m_hBtnToggleTweak, TRUE);
    EnableWindow(m_hBtnCopyTweak, TRUE);

    const auto& t = m_displayedTweaks[selectedIndex];
    const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});

    // Item 11: Consistent per-setting action labels
    if (st == SettingStatus::Applied) {
        SetWindowTextW(m_hBtnToggleTweak, L"Restore Setting");
    } else {
        SetWindowTextW(m_hBtnToggleTweak, L"Apply Setting");
    }

    // Item 10: Reordered and shortened details content in sentence-case
    std::wstringstream ss;
    ss << t.title << L"\r\n";

    // State, impact, scope metadata line
    std::wstring stStr;
    switch (st) {
    case SettingStatus::Applied:       stStr = L"Applied"; break;
    case SettingStatus::NotApplied:    stStr = L"Not applied"; break;
    case SettingStatus::Partial:       stStr = L"Partial"; break;
    case SettingStatus::Unknown:       stStr = L"Unknown"; break;
    case SettingStatus::NotApplicable: stStr = L"Not applicable"; break;
    }

    std::wstring impactStr;
    switch (t.impactLevel) {
    case ImpactLevel::Low:      impactStr = L"Low"; break;
    case ImpactLevel::Moderate: impactStr = L"Moderate"; break;
    case ImpactLevel::High:     impactStr = L"High"; break;
    }

    std::wstring scopeStr;
    switch (t.scope) {
    case TargetScope::Machine: scopeStr = L"Machine"; break;
    case TargetScope::User:    scopeStr = L"User"; break;
    case TargetScope::Both:    scopeStr = L"Machine & user"; break;
    case TargetScope::Service: scopeStr = L"Machine (Service)"; break;
    }

    ss << L"State: " << stStr
       << L"  |  Impact: " << impactStr
       << L"  |  Scope: " << scopeStr
       << (t.isRecommended ? L"  |  Recommended: Yes" : L"  |  Recommended: No")
       << L"  |  Category: " << t.category << L"\r\n\r\n";

    // 1. What applying it does (practical explanation first)
    ss << L"What applying it does:\r\n" << t.description << L"\r\n\r\n";

    // 2. Features it may affect (specific consequence & tradeoff)
    if (!t.impact.empty()) {
        ss << L"Features it may affect:\r\n" << t.impact << L"\r\n\r\n";
    }

    // 3. Restart/sign-out requirement
    ss << L"Restart requirement:\r\n";
    if (t.requiresReboot) {
        ss << L"System restart required for changes to take full effect.\r\n\r\n";
    } else if (t.requiresSignOut) {
        ss << L"User sign-out required for changes to take full effect.\r\n\r\n";
    } else {
        ss << L"None. Takes effect immediately or on next process launch.\r\n\r\n";
    }

    // 4. Technical information below practical explanation
    if (!t.regActions.empty() || !t.serviceActions.empty()) {
        ss << L"Technical information:\r\n";
        for (const auto& reg : t.regActions) {
            ss << L"  [" << (reg.scope == TargetScope::Machine ? L"HKLM\\" : L"HKCU\\") << reg.subKey << L"]\r\n";
            if (reg.type == RegType::Dword) {
                ss << L"    " << reg.valueName << L" = " << reg.dwordProtected << L" (REG_DWORD)\r\n";
            } else {
                ss << L"    " << reg.valueName << L" = \"" << reg.strProtected << L"\" (REG_SZ)\r\n";
            }
        }
        for (const auto& svc : t.serviceActions) {
            ss << L"  [Service] " << svc.serviceName << L" -> Startup: "
               << (svc.startupTypeProtected == 4 ? L"Disabled (4)" : std::to_wstring(svc.startupTypeProtected)) << L"\r\n";
        }
    }

    SetWindowTextW(m_hDetailsEdit, ss.str().c_str());
}

void MainWindow::ToggleSelectedTweakFromDetails() {
    const int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= static_cast<int>(m_displayedTweaks.size())) return;

    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            L"Administrator privileges are required to modify system settings.\n\n"
            L"Would you like to restart PrivatizeWin as Administrator now?",
            L"PrivatizeWin \u2014 Elevation Required",
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        return;
    }

    const auto& t = m_displayedTweaks[sel];
    const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
    const bool shouldApply = (st != SettingStatus::Applied);

    const bool ok = TweakRegistry::Instance().ApplyTweak(t.id, shouldApply, UserSelectionMode::AllUsers, {});
    if (ok) {
        // Re-audit setting and update UI
        const SettingStatus newSt = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        std::wstring stStr;
        switch (newSt) {
        case SettingStatus::Applied:       stStr = L"\u25CF Applied"; break;
        case SettingStatus::NotApplied:    stStr = L"\u25CB Not applied"; break;
        case SettingStatus::Partial:       stStr = L"\u25D0 Partial"; break;
        case SettingStatus::Unknown:       stStr = L"? Unknown"; break;
        case SettingStatus::NotApplicable: stStr = L"\u2014 Not applicable"; break;
        }
        ListView_SetItemText(m_hListView, sel, 1, const_cast<LPWSTR>(stStr.c_str()));
        UpdateDetailsPane(sel);
        UpdateStatusBar();
    } else {
        MessageBoxW(m_hWnd, (L"Failed to change setting: " + t.title).c_str(), L"PrivatizeWin Error", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::UpdateStatusBar() {
    const auto& all = TweakRegistry::Instance().GetAllTweaks();
    int appliedCount = 0;
    for (const auto& t : all) {
        if (TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {}) == SettingStatus::Applied) {
            appliedCount++;
        }
    }

    // Item 14: Factual counts in status bar ("40 applied · 12 selected · 260 shown")
    const size_t totalSelected = m_selectedTweakIds.size();
    size_t visibleSelected = 0;
    for (const auto& t : m_displayedTweaks) {
        if (m_selectedTweakIds.count(t.id) > 0) visibleSelected++;
    }
    const size_t hiddenSelected = totalSelected - visibleSelected;

    std::wstring part0 = std::to_wstring(appliedCount) + L" applied \u00B7 " +
                         std::to_wstring(totalSelected) + L" selected";
    if (hiddenSelected > 0) {
        part0 += L" (" + std::to_wstring(hiddenSelected) + L" hidden)";
    }
    part0 += L" \u00B7 " + std::to_wstring(m_displayedTweaks.size()) + L" shown";
    SendMessage(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(part0.c_str()));

    // Elevation Status
    const bool isAdmin = IsRunningAsAdmin();
    std::wstring part2 = isAdmin ? L"Administrator (Elevated)" : L"Standard User (Limited - Read Only)";
    SendMessage(m_hStatusBar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(part2.c_str()));
}

void MainWindow::SelectRecommended() {
    m_selectedTweakIds.clear();
    const auto& catalog = TweakRegistry::Instance().GetAllTweaks();
    for (const auto& t : catalog) {
        if (t.isRecommended) {
            m_selectedTweakIds.insert(t.id);
        }
    }
    // Update checkboxes for displayed items
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const bool isSel = (m_selectedTweakIds.count(m_displayedTweaks[i].id) > 0);
        ListView_SetCheckState(m_hListView, i, isSel ? TRUE : FALSE);
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::SelectAllShown() {
    for (const auto& t : m_displayedTweaks) {
        m_selectedTweakIds.insert(t.id);
    }
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        ListView_SetCheckState(m_hListView, i, TRUE);
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::InvertShownSelection() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& id = m_displayedTweaks[i].id;
        if (m_selectedTweakIds.count(id) > 0) {
            m_selectedTweakIds.erase(id);
            ListView_SetCheckState(m_hListView, i, FALSE);
        } else {
            m_selectedTweakIds.insert(id);
            ListView_SetCheckState(m_hListView, i, TRUE);
        }
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::ClearSelection() {
    m_selectedTweakIds.clear();
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        ListView_SetCheckState(m_hListView, i, FALSE);
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::SelectPreset(std::string_view templateName) {
    const auto tpl = TemplateManager::Instance().GetTemplate(templateName);
    if (!tpl.has_value()) return;

    // Item 2: Preset selection replaces previous selection
    m_selectedTweakIds.clear();
    for (const auto& [id, shouldEnable] : tpl->tweakStates) {
        if (shouldEnable) {
            m_selectedTweakIds.insert(id);
        }
    }

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const bool isSel = (m_selectedTweakIds.count(m_displayedTweaks[i].id) > 0);
        ListView_SetCheckState(m_hListView, i, isSel ? TRUE : FALSE);
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::ShowSelectMenu() {
    RECT rcBtn{};
    GetWindowRect(m_hBtnSelectMenu, &rcBtn);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_SEL_RECOMMENDED, L"Select Recommended");
    AppendMenuW(hMenu, MF_STRING, IDM_SEL_ALL_SHOWN, L"Select All Shown");
    AppendMenuW(hMenu, MF_STRING, IDM_SEL_INVERT_SHOWN, L"Invert Shown Selection");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_SEL_CLEAR, L"Clear Selection");

    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, rcBtn.left, rcBtn.bottom, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
}

void MainWindow::ApplySelectedTweaks() {
    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            L"Administrator privileges are required to apply registry and service modifications.\n\n"
            L"Would you like to restart PrivatizeWin as Administrator now?",
            L"PrivatizeWin \u2014 Elevation Required",
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        return;
    }

    if (m_selectedTweakIds.empty()) return;

    // Item 7: Provide one concise review before bulk writes
    std::vector<const Tweak*> moderateHighTweaks;
    bool anyReboot = false;
    bool anySignOut = false;

    for (const auto& id : m_selectedTweakIds) {
        const Tweak* t = TweakRegistry::Instance().GetTweakById(id);
        if (t) {
            if (t->impactLevel == ImpactLevel::Moderate || t->impactLevel == ImpactLevel::High) {
                moderateHighTweaks.push_back(t);
            }
            if (t->requiresReboot) anyReboot = true;
            if (t->requiresSignOut) anySignOut = true;
        }
    }

    std::wstringstream review;
    review << L"You are about to apply " << m_selectedTweakIds.size() << L" selected privacy setting(s).\n\n";

    if (!moderateHighTweaks.empty()) {
        review << L"Moderate / High Impact Settings Included (" << moderateHighTweaks.size() << L"):\n";
        const size_t limit = std::min<size_t>(moderateHighTweaks.size(), 8);
        for (size_t i = 0; i < limit; ++i) {
            const auto* t = moderateHighTweaks[i];
            review << L"  \u2022 " << t->title << L" (" << (t->impactLevel == ImpactLevel::High ? L"High" : L"Moderate") << L" impact)\n";
        }
        if (moderateHighTweaks.size() > limit) {
            review << L"  ... and " << (moderateHighTweaks.size() - limit) << L" more.\n";
        }
        review << L"\n";
    }

    if (anyReboot) {
        review << L"Note: A system restart is required for some settings to take full effect.\n\n";
    } else if (anySignOut) {
        review << L"Note: A user sign-out is required for some settings to take full effect.\n\n";
    }

    review << L"Do you wish to proceed?";

    const int choice = MessageBoxW(m_hWnd, review.str().c_str(), L"Apply Selected Settings", MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) return;

    // Item 8: Execute bulk write and report changed, already in state, failed, skipped
    ShowWindow(m_hProgressBar, SW_SHOW);
    SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, static_cast<LPARAM>(m_selectedTweakIds.size()));
    SendMessage(m_hProgressBar, PBM_SETPOS, 0, 0);

    int changedCount = 0;
    int alreadyCount = 0;
    int failedCount = 0;
    std::vector<std::wstring> failedTitles;

    int progress = 0;
    for (const auto& id : m_selectedTweakIds) {
        const SettingStatus before = TweakRegistry::Instance().AuditTweak(id, UserSelectionMode::AllUsers, {});
        if (before == SettingStatus::Applied) {
            alreadyCount++;
        } else {
            const bool ok = TweakRegistry::Instance().ApplyTweak(id, true, UserSelectionMode::AllUsers, {});
            const SettingStatus after = TweakRegistry::Instance().AuditTweak(id, UserSelectionMode::AllUsers, {});
            if (ok && after == SettingStatus::Applied) {
                changedCount++;
            } else {
                failedCount++;
                const Tweak* t = TweakRegistry::Instance().GetTweakById(id);
                failedTitles.push_back(t ? t->title : std::wstring(id.begin(), id.end()));
            }
        }
        progress++;
        SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
    }

    ShowWindow(m_hProgressBar, SW_HIDE);
    RefreshAuditState();

    // Summary results dialog
    std::wstringstream resMsg;
    resMsg << L"Operation completed.\n\n"
           << L"  \u2022 Changed: " << changedCount << L"\n"
           << L"  \u2022 Already in requested state: " << alreadyCount << L"\n"
           << L"  \u2022 Failed: " << failedCount << L"\n"
           << L"  \u2022 Skipped: 0\n";

    if (!failedTitles.empty()) {
        resMsg << L"\nFailed Settings:\n";
        for (const auto& title : failedTitles) {
            resMsg << L"  \u2022 " << title << L"\n";
        }
    }

    MessageBoxW(m_hWnd, resMsg.str().c_str(), L"PrivatizeWin", MB_OK | (failedCount == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void MainWindow::RestoreSelectedDefaults() {
    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            L"Administrator privileges are required to restore Windows default settings.\n\n"
            L"Would you like to restart PrivatizeWin as Administrator now?",
            L"PrivatizeWin \u2014 Elevation Required",
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        return;
    }

    if (m_selectedTweakIds.empty()) return;

    // Item 6: Accurate scope confirmation
    std::wstring prompt = L"Are you sure you want to restore Windows default values for the " +
                          std::to_wstring(m_selectedTweakIds.size()) + L" selected setting(s)?";
    const int choice = MessageBoxW(m_hWnd, prompt.c_str(), L"Restore Windows Defaults", MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) return;

    ShowWindow(m_hProgressBar, SW_SHOW);
    SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, static_cast<LPARAM>(m_selectedTweakIds.size()));
    SendMessage(m_hProgressBar, PBM_SETPOS, 0, 0);

    int changedCount = 0;
    int alreadyCount = 0;
    int failedCount = 0;

    int progress = 0;
    for (const auto& id : m_selectedTweakIds) {
        const SettingStatus before = TweakRegistry::Instance().AuditTweak(id, UserSelectionMode::AllUsers, {});
        if (before == SettingStatus::NotApplied) {
            alreadyCount++;
        } else {
            const bool ok = TweakRegistry::Instance().ApplyTweak(id, false, UserSelectionMode::AllUsers, {});
            const SettingStatus after = TweakRegistry::Instance().AuditTweak(id, UserSelectionMode::AllUsers, {});
            if (ok && after == SettingStatus::NotApplied) {
                changedCount++;
            } else {
                failedCount++;
            }
        }
        progress++;
        SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
    }

    ShowWindow(m_hProgressBar, SW_HIDE);
    RefreshAuditState();

    std::wstring resMsg = L"Restoration completed.\n\n"
                          L"  \u2022 Changed: " + std::to_wstring(changedCount) + L"\n" +
                          L"  \u2022 Already default: " + std::to_wstring(alreadyCount) + L"\n" +
                          L"  \u2022 Failed: " + std::to_wstring(failedCount);
    MessageBoxW(m_hWnd, resMsg.c_str(), L"PrivatizeWin", MB_OK | (failedCount == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void MainWindow::RestoreAllDefaults() {
    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            L"Administrator privileges are required to restore Windows default settings.\n\n"
            L"Would you like to restart PrivatizeWin as Administrator now?",
            L"PrivatizeWin \u2014 Elevation Required",
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        return;
    }

    // Item 6: Global restore in Actions menu
    const int choice = MessageBoxW(m_hWnd,
        L"Are you sure you want to restore Windows default values for ALL settings on this computer?\n\n"
        L"This will reset all privacy and telemetry tweaks across every category to out-of-the-box Windows defaults.",
        L"Confirm Restore All Defaults",
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (choice != IDYES) return;

    const auto& catalog = TweakRegistry::Instance().GetAllTweaks();

    ShowWindow(m_hProgressBar, SW_SHOW);
    SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, static_cast<LPARAM>(catalog.size()));
    SendMessage(m_hProgressBar, PBM_SETPOS, 0, 0);

    int changed = 0;
    int progress = 0;
    for (const auto& t : catalog) {
        if (TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {}) == SettingStatus::Applied) {
            if (TweakRegistry::Instance().ApplyTweak(t.id, false, UserSelectionMode::AllUsers, {})) {
                changed++;
            }
        }
        progress++;
        SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
    }

    ShowWindow(m_hProgressBar, SW_HIDE);
    RefreshAuditState();

    MessageBoxW(m_hWnd, (L"Restored " + std::to_wstring(changed) + L" settings to default.").c_str(), L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::RefreshAuditState() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});

        std::wstring stStr;
        switch (st) {
        case SettingStatus::Applied:       stStr = L"\u25CF Applied"; break;
        case SettingStatus::NotApplied:    stStr = L"\u25CB Not applied"; break;
        case SettingStatus::Partial:       stStr = L"\u25D0 Partial"; break;
        case SettingStatus::Unknown:       stStr = L"? Unknown"; break;
        case SettingStatus::NotApplicable: stStr = L"\u2014 Not applicable"; break;
        }
        ListView_SetItemText(m_hListView, i, 1, const_cast<LPWSTR>(stStr.c_str()));
    }

    UpdateSelectionCounts();
    UpdateStatusBar();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::OnContextMenu(HWND hWnd, int x, int y) {
    if (hWnd != m_hListView && GetParent(hWnd) != m_hListView) return;

    if (x == -1 && y == -1) {
        const int focus = ListView_GetNextItem(m_hListView, -1, LVNI_FOCUSED);
        if (focus != -1) {
            RECT rcItem{};
            ListView_GetItemRect(m_hListView, focus, &rcItem, LVIR_BOUNDS);
            POINT pt{ rcItem.left + 50, rcItem.bottom };
            ClientToScreen(m_hListView, &pt);
            x = pt.x;
            y = pt.y;
        }
    }

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_PROTECT_SELECTED, L"Select for Application");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_DEFAULT_SELECTED, L"Deselect");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_ID, L"Copy Setting ID");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_DETAILS, L"Copy Technical Details");

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
}

void MainWindow::OnKeyDown(WPARAM vk) {
    if (vk == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SetFocus(m_hSearchEdit);
        SendMessage(m_hSearchEdit, EM_SETSEL, 0, -1);
    } else if (vk == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SelectAllShown();
    }
}

void MainWindow::CopySelectedTweakIds() {
    std::wstringstream ss;
    int i = -1;
    while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
        const auto& t = m_displayedTweaks[i];
        ss << std::wstring(t.id.begin(), t.id.end()) << L"\r\n";
    }

    std::wstring text = ss.str();
    if (text.empty()) return;

    if (OpenClipboard(m_hWnd)) {
        EmptyClipboard();
        const size_t bytes = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            memcpy(GlobalLock(hMem), text.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

void MainWindow::CopySelectedTweakDetails() {
    const int focus = ListView_GetNextItem(m_hListView, -1, LVNI_FOCUSED);
    if (focus < 0 || focus >= static_cast<int>(m_displayedTweaks.size())) return;

    const auto& t = m_displayedTweaks[focus];
    std::wstringstream ss;
    ss << t.title << L" [" << std::wstring(t.id.begin(), t.id.end()) << L"]\r\n";
    ss << L"Category: " << t.category << L"\r\n";
    ss << L"Impact: " << (t.impactLevel == ImpactLevel::Low ? L"Low" : (t.impactLevel == ImpactLevel::Moderate ? L"Moderate" : L"High")) << L"\r\n";
    ss << L"Description: " << t.description << L"\r\n";
    ss << L"Consequences: " << t.impact << L"\r\n";

    std::wstring text = ss.str();
    if (OpenClipboard(m_hWnd)) {
        EmptyClipboard();
        const size_t bytes = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            memcpy(GlobalLock(hMem), text.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

void MainWindow::OnCommand(int id, HWND hCtrl) {
    switch (id) {
    case IDM_FILE_EXPORT:
        ExportConfiguration();
        break;
    case IDM_FILE_IMPORT:
        ImportConfiguration();
        break;
    case IDM_FILE_RESTART_ADMIN:
        if (IsRunningAsAdmin()) {
            MessageBoxW(m_hWnd, L"PrivatizeWin is already running with Administrator privileges.", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        } else {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        break;
    case IDM_FILE_EXIT:
        DestroyWindow(m_hWnd);
        break;
    case IDM_TPL_RECOMMENDED:
        SelectPreset("recommended");
        break;
    case IDM_TPL_STRICT:
        SelectPreset("strict");
        break;
    case IDM_TPL_MINIMAL:
        SelectPreset("minimal");
        break;
    case IDM_ACT_RESTORE_SELECTED:
    case IDC_BTN_REVERT:
        RestoreSelectedDefaults();
        break;
    case IDM_ACT_RESTORE_ALL:
        RestoreAllDefaults();
        break;
    case IDM_ACT_APPLY:
    case IDC_BTN_APPLY:
        ApplySelectedTweaks();
        break;
    case IDM_SEL_RECOMMENDED:
        SelectRecommended();
        break;
    case IDM_SEL_ALL_SHOWN:
        SelectAllShown();
        break;
    case IDM_SEL_INVERT_SHOWN:
        InvertShownSelection();
        break;
    case IDM_SEL_CLEAR:
        ClearSelection();
        break;
    case IDC_BTN_SELECT_MENU:
        ShowSelectMenu();
        break;
    case IDC_BTN_SELECT_PRESET: {
        const int sel = static_cast<int>(SendMessage(m_hTemplateCombo, CB_GETCURSEL, 0, 0));
        if (sel == 1) SelectPreset("strict");
        else if (sel == 2) SelectPreset("minimal");
        else SelectPreset("recommended");
        break;
    }
    case IDM_ACT_REFRESH:
    case IDC_BTN_REFRESH:
        RefreshAuditState();
        break;
    case IDM_ACT_RESTORE_PT:
        CreateSystemRestorePoint();
        break;
    case IDM_TOOLS_SCHEDULE:
        ScheduleDialog::Show(m_hWnd);
        break;
    case IDM_TOOLS_TASKSCHD:
        ShellExecuteW(nullptr, L"open", L"taskschd.msc", nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case IDM_HELP_ABOUT:
        MessageBoxW(m_hWnd,
            L"PrivatizeWin v1.3\n"
            L"Open-source Windows privacy and telemetry configuration tool.\n\n"
            L"MIT License",
            L"About PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        break;
    case IDM_HELP_GITHUB:
        ShellExecuteW(nullptr, L"open", L"https://github.com/saverchenkov/PrivatizeWin", nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case IDC_DETAILS_BTN_TOGGLE:
        ToggleSelectedTweakFromDetails();
        break;
    case IDC_DETAILS_BTN_COPY:
        CopySelectedTweakDetails();
        break;
    case IDM_CTX_PROTECT_SELECTED: {
        int i = -1;
        while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
            m_selectedTweakIds.insert(m_displayedTweaks[i].id);
            ListView_SetCheckState(m_hListView, i, TRUE);
        }
        UpdateSelectionCounts();
        break;
    }
    case IDM_CTX_DEFAULT_SELECTED: {
        int i = -1;
        while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
            m_selectedTweakIds.erase(m_displayedTweaks[i].id);
            ListView_SetCheckState(m_hListView, i, FALSE);
        }
        UpdateSelectionCounts();
        break;
    }
    case IDM_CTX_COPY_ID:
        CopySelectedTweakIds();
        break;
    case IDM_CTX_COPY_DETAILS:
        CopySelectedTweakDetails();
        break;
    case IDC_SEARCH_EDIT:
        if (HIWORD(reinterpret_cast<DWORD_PTR>(hCtrl)) == EN_CHANGE || (hCtrl == nullptr && GetFocus() == m_hSearchEdit)) {
            wchar_t buf[256]{};
            GetWindowTextW(m_hSearchEdit, buf, 256);
            m_currentFilter = buf;
            PopulateListView(m_currentFilter, m_filterMode);
        }
        break;
    case IDC_FILTER_COMBO:
        if (HIWORD(reinterpret_cast<DWORD_PTR>(hCtrl)) == CBN_SELCHANGE) {
            const int sel = static_cast<int>(SendMessage(m_hFilterCombo, CB_GETCURSEL, 0, 0));
            if (sel == 0) m_filterMode = FilterMode::All;
            else if (sel == 1) m_filterMode = FilterMode::NotAppliedOnly;
            else if (sel == 2) m_filterMode = FilterMode::AppliedOnly;
            else if (sel == 3) m_filterMode = FilterMode::RecommendedOnly;
            PopulateListView(m_currentFilter, m_filterMode);
        }
        break;
    }
}

void MainWindow::OnNotify(NMHDR* pnmhdr) {
    if (pnmhdr->idFrom == IDC_LIST_TWEAKS) {
        if (pnmhdr->code == LVN_ITEMCHANGED) {
            auto* pItem = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
            if (pItem->iItem >= 0 && pItem->iItem < static_cast<int>(m_displayedTweaks.size())) {
                // Selection change in ListView
                if ((pItem->uChanged & LVIF_STATE) && (pItem->uNewState & LVIS_SELECTED)) {
                    UpdateDetailsPane(pItem->iItem);
                }

                // Checkbox toggle (Item 1: selection for operation)
                if (pItem->uChanged & LVIF_STATE) {
                    const UINT oldCheck = pItem->uOldState & LVIS_STATEIMAGEMASK;
                    const UINT newCheck = pItem->uNewState & LVIS_STATEIMAGEMASK;
                    if (oldCheck != newCheck && newCheck != 0) {
                        const bool isChecked = (newCheck >> 12) == 2;
                        const auto& id = m_displayedTweaks[pItem->iItem].id;
                        if (isChecked) {
                            m_selectedTweakIds.insert(id);
                        } else {
                            m_selectedTweakIds.erase(id);
                        }
                        UpdateSelectionCounts();
                        UpdateStatusBar();
                    }
                }
            }
        } else if (pnmhdr->code == LVN_KEYDOWN) {
            auto* pnkd = reinterpret_cast<NMLVKEYDOWN*>(pnmhdr);
            if (pnkd->wVKey == VK_SPACE) {
                // Item 15: Space toggles checkbox of focused row
                const int focused = ListView_GetNextItem(m_hListView, -1, LVNI_FOCUSED);
                if (focused >= 0 && focused < static_cast<int>(m_displayedTweaks.size())) {
                    const BOOL cur = ListView_GetCheckState(m_hListView, focused);
                    ListView_SetCheckState(m_hListView, focused, !cur);
                }
            } else if (pnkd->wVKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                SelectAllShown();
            }
        } else if (pnmhdr->code == NM_RCLICK) {
            POINT pt{};
            GetCursorPos(&pt);
            OnContextMenu(m_hListView, pt.x, pt.y);
        }
    }
}

void MainWindow::ExportConfiguration() {
    wchar_t szFile[MAX_PATH] = L"PrivatizeWin_Config.json";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"json";

    if (GetSaveFileNameW(&ofn)) {
        TemplateProfile p{};
        p.name = "Exported Configuration";
        p.description = "User exported configuration profile";
        for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
            p.tweakStates[m_displayedTweaks[i].id] = (m_selectedTweakIds.count(m_displayedTweaks[i].id) > 0);
        }

        if (TemplateManager::Instance().SaveTemplateToFile(szFile, p)) {
            MessageBoxW(m_hWnd, L"Configuration exported successfully.", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Failed to export configuration to file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::ImportConfiguration() {
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        TemplateProfile p{};
        if (TemplateManager::Instance().LoadTemplateFromFile(szFile, p)) {
            m_selectedTweakIds.clear();
            for (const auto& [id, shouldCheck] : p.tweakStates) {
                if (shouldCheck) m_selectedTweakIds.insert(id);
            }

            for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
                const bool isSel = (m_selectedTweakIds.count(m_displayedTweaks[i].id) > 0);
                ListView_SetCheckState(m_hListView, i, isSel ? TRUE : FALSE);
            }
            UpdateSelectionCounts();

            int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
            if (cur != -1) {
                UpdateDetailsPane(cur);
            }

            MessageBoxW(m_hWnd, L"Configuration imported successfully. Review selections and click 'Apply Selected'.", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Failed to load or parse configuration file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::CreateSystemRestorePoint() {
    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            L"Administrator privileges are required to create a Windows System Restore Point.\n\n"
            L"Would you like to restart PrivatizeWin as Administrator now?",
            L"PrivatizeWin \u2014 Elevation Required",
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            if (RelaunchElevated(m_hWnd)) {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        return;
    }

    const int choice = MessageBoxW(m_hWnd,
        L"Do you want to create a Windows System Restore Point before applying tweaks?",
        L"System Restore Point",
        MB_YESNO | MB_ICONQUESTION);

    if (choice != IDYES) return;

    SetCursor(LoadCursor(nullptr, IDC_WAIT));
    int64_t seqNumber = 0;
    const bool success = RestorePoint::Create(L"PrivatizeWin Baseline Restore Point", seqNumber);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    if (success) {
        std::wstring msg = L"System Restore Point created successfully (Sequence #" + std::to_wstring(seqNumber) + L").";
        MessageBoxW(m_hWnd, msg.c_str(), L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(m_hWnd, L"Failed to create Restore Point. Ensure System Protection is enabled in Windows.", L"Restore Point", MB_OK | MB_ICONWARNING);
    }
}

} // namespace PrivatizeWin
