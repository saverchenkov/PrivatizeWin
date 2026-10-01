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
        L"PrivatizeWin - Windows Privacy & Telemetry Silencer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1140, 760,
        nullptr, nullptr, hInstance, nullptr
    );
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CREATE) {
        s_pMainWnd = std::make_unique<MainWindow>(hWnd);
        s_pMainWnd->OnCreate();
        return 0;
    }

    if (s_pMainWnd) {
        switch (uMsg) {
        case WM_GETMINMAXINFO: {
            auto* pMMI = reinterpret_cast<MINMAXINFO*>(lParam);
            pMMI->ptMinTrackSize.x = 760;
            pMMI->ptMinTrackSize.y = 480;
            return 0;
        }
        case WM_SIZE:
            s_pMainWnd->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = reinterpret_cast<HDC>(wParam);
            const bool isDark = DarkMode::IsDarkModeActive();
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, isDark ? RGB(200, 205, 215) : RGB(40, 45, 55));
            static HBRUSH s_hBrBarLight = CreateSolidBrush(RGB(246, 248, 250));
            static HBRUSH s_hBrBarDark = CreateSolidBrush(RGB(38, 40, 44));
            return reinterpret_cast<INT_PTR>(isDark ? s_hBrBarDark : s_hBrBarLight);
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rcClient{};
            GetClientRect(hWnd, &rcClient);
            const int w = rcClient.right - rcClient.left;
            const int h = rcClient.bottom - rcClient.top;

            const bool isDark = DarkMode::IsDarkModeActive();
            const COLORREF colBar = isDark ? RGB(38, 40, 44) : RGB(246, 248, 250);
            const COLORREF colLine = isDark ? RGB(55, 58, 64) : RGB(220, 224, 230);
            const COLORREF colBg = isDark ? RGB(28, 28, 30) : RGB(240, 242, 245);

            // Fill base background
            HBRUSH hBrBg = CreateSolidBrush(colBg);
            FillRect(hdc, &rcClient, hBrBg);
            DeleteObject(hBrBg);

            // Top toolbar bar background (Height: 46px)
            RECT rcTopBar{ 0, 0, w, 46 };
            HBRUSH hBrBar = CreateSolidBrush(colBar);
            FillRect(hdc, &rcTopBar, hBrBar);
            DeleteObject(hBrBar);

            // Top toolbar separator line
            HPEN hPen = CreatePen(PS_SOLID, 1, colLine);
            HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
            MoveToEx(hdc, 0, 46, nullptr);
            LineTo(hdc, w, 46);

            // Separator line between list and details pane
            const int detailsTop = h - 24 - 170;
            if (detailsTop > 50) {
                MoveToEx(hdc, 10, detailsTop - 4, nullptr);
                LineTo(hdc, w - 10, detailsTop - 4);
            }

            SelectObject(hdc, oldPen);
            DeleteObject(hPen);

            return 1;
        }
        case WM_COMMAND:
            s_pMainWnd->OnCommand(LOWORD(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_NOTIFY: {
            auto* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
            if (pnmhdr->idFrom == IDC_LIST_TWEAKS && pnmhdr->code == NM_CUSTOMDRAW) {
                return s_pMainWnd->OnCustomDraw(pnmhdr);
            }
            s_pMainWnd->OnNotify(pnmhdr);
            return 0;
        }
        case WM_CONTEXTMENU: {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            s_pMainWnd->OnContextMenu(reinterpret_cast<HWND>(wParam), x, y);
            return 0;
        }
        case WM_DESTROY:
            s_pMainWnd.reset();
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {}

MainWindow::~MainWindow() {
    if (m_hFontRegular) DeleteObject(m_hFontRegular);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontCode) DeleteObject(m_hFontCode);
    if (m_hRowImageList) ImageList_Destroy(m_hRowImageList);
    if (m_hRichEditLib) FreeLibrary(m_hRichEditLib);
}

void MainWindow::InitializeFonts() {
    HDC hdc = GetDC(m_hWnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(m_hWnd, hdc);
    if (dpi <= 0) dpi = 96;

    auto makeFont = [dpi](int ptSize, int weight, const wchar_t* face) {
        return CreateFontW(
            -MulDiv(ptSize, dpi, 72), 0, 0, 0, weight,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, face
        );
    };

    m_hFontRegular = makeFont(9, FW_NORMAL, L"Segoe UI");
    m_hFontBold    = makeFont(9, FW_SEMIBOLD, L"Segoe UI");
    m_hFontTitle   = makeFont(11, FW_SEMIBOLD, L"Segoe UI");
    m_hFontCode    = makeFont(9, FW_NORMAL, L"Consolas");
}

void MainWindow::OnCreate() {
    DarkMode::ApplyToWindow(m_hWnd);

    m_hRichEditLib = LoadLibraryW(L"msftedit.dll");

    const HICON hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_APPICON));
    if (hIcon) {
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    }

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    InitializeFonts();
    InitializeControls();

    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    PopulateListView(L"", FilterMode::All);
    UpdateStatusBar();
}

void MainWindow::InitializeControls() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 1. Search Box
    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        10, 10, 180, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_SEARCH_EDIT), hInst, nullptr);
    SendMessage(m_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter tweaks (Ctrl+F)..."));

    // 2. Filter Dropdown
    m_hFilterCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        196, 10, 135, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_FILTER_COMBO), hInst, nullptr);
    SendMessage(m_hFilterCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All Settings"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unprotected Only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Protected Only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended Only"));
    SendMessage(m_hFilterCombo, CB_SETCURSEL, 0, 0);

    // 3. Match Count Label
    m_hLblMatchCount = CreateWindowW(WC_STATICW, L"260 matches",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        338, 13, 95, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_LBL_MATCH_COUNT), hInst, nullptr);
    SendMessage(m_hLblMatchCount, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 4. Template Combo
    m_hTemplateCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        450, 10, 140, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_TPL_COMBO), hInst, nullptr);
    SendMessage(m_hTemplateCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended (Safe)"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strict Privacy"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal Telemetry"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Revert to Defaults"));
    SendMessage(m_hTemplateCombo, CB_SETCURSEL, 0, 0);

    // 5. Action Buttons (Right-aligned)
    m_hChkSelectAll = CreateWindowW(WC_BUTTONW, L"Check All",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        600, 13, 78, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_CHK_CHECK_ALL), hInst, nullptr);
    SendMessage(m_hChkSelectAll, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnRefresh = CreateWindowW(WC_BUTTONW, L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        684, 10, 64, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REFRESH), hInst, nullptr);
    SendMessage(m_hBtnRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnRevert = CreateWindowW(WC_BUTTONW, L"Revert Defaults",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        754, 10, 104, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REVERT), hInst, nullptr);
    SendMessage(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnApply = CreateWindowW(WC_BUTTONW, L"Apply Changes",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        864, 10, 112, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_APPLY), hInst, nullptr);
    SendMessage(m_hBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    // 6. Unified Full-Width ListView with Native Grouping
    m_hListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        10, 48, 1120, 450, m_hWnd, reinterpret_cast<HMENU>(IDC_LIST_TWEAKS), hInst, nullptr);
    SendMessage(m_hListView, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Set 26px row height via image list
    m_hRowImageList = ImageList_Create(1, 26, ILC_COLOR32, 1, 1);
    if (m_hRowImageList) {
        ListView_SetImageList(m_hListView, m_hRowImageList, LVSIL_SMALL);
    }
    SetWindowTheme(m_hListView, L"Explorer", nullptr);
    DarkMode::ApplyToControl(m_hListView);

    // ListView Columns
    LVCOLUMNW lvc{};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"Privacy Setting / Tweak");
    lvc.cx = 600;
    ListView_InsertColumn(m_hListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Current Status");
    lvc.cx = 135;
    ListView_InsertColumn(m_hListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Safety Level");
    lvc.cx = 115;
    ListView_InsertColumn(m_hListView, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.pszText = const_cast<LPWSTR>(L"Scope");
    lvc.cx = 85;
    ListView_InsertColumn(m_hListView, 3, &lvc);

    // 7. Details Inspector Actions & Native RichEdit Inspector
    m_hBtnToggleTweak = CreateWindowW(WC_BUTTONW, L"Enable Protection",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 140, 24, m_hWnd, reinterpret_cast<HMENU>(IDC_DETAILS_BTN_TOGGLE), hInst, nullptr);
    SendMessage(m_hBtnToggleTweak, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnCopyTweak = CreateWindowW(WC_BUTTONW, L"Copy Details",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 95, 24, m_hWnd, reinterpret_cast<HMENU>(IDC_DETAILS_BTN_COPY), hInst, nullptr);
    SendMessage(m_hBtnCopyTweak, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hDetailsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, 520, 1120, 140, m_hWnd, reinterpret_cast<HMENU>(IDC_EDIT_DETAILS), hInst, nullptr);
    SendMessage(m_hDetailsEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hDetailsEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));

    if (DarkMode::IsDarkModeActive()) {
        SendMessage(m_hDetailsEdit, EM_SETBKGNDCOLOR, 0, RGB(32, 32, 34));
    }

    // 8. Status Bar & Embedded Progress Bar
    m_hStatusBar = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), hInst, nullptr);
    int sbParts[] = { 220, 360, 800, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(sbParts));
    SendMessage(m_hStatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        0, 0, 0, 0, m_hStatusBar, reinterpret_cast<HMENU>(IDC_STATUS_PROGRESS), hInst, nullptr);
}

void MainWindow::OnSize(int width, int height) {
    // 1. Responsive Toolbar Layout
    const int btnY = 10;
    const int applyW = 112;
    const int revertW = 104;
    const int refreshW = 64;
    const int chkAllW = 78;
    const int tplW = 140;

    int rightX = width - 12;

    rightX -= applyW;
    if (m_hBtnApply) MoveWindow(m_hBtnApply, rightX, btnY, applyW, 26, TRUE);

    rightX -= (revertW + 6);
    if (m_hBtnRevert) MoveWindow(m_hBtnRevert, rightX, btnY, revertW, 26, TRUE);

    rightX -= (refreshW + 6);
    if (m_hBtnRefresh) MoveWindow(m_hBtnRefresh, rightX, btnY, refreshW, 26, TRUE);

    rightX -= (chkAllW + 8);
    if (m_hChkSelectAll) MoveWindow(m_hChkSelectAll, rightX, btnY + 3, chkAllW, 20, TRUE);

    const bool showTpl = (width >= 940);
    if (m_hTemplateCombo) {
        if (showTpl) {
            rightX -= (tplW + 8);
            MoveWindow(m_hTemplateCombo, rightX, btnY, tplW, 200, TRUE);
            ShowWindow(m_hTemplateCombo, SW_SHOW);
        } else {
            ShowWindow(m_hTemplateCombo, SW_HIDE);
        }
    }

    const int maxLeftW = (std::max)(120, rightX - 12 - 10);
    const bool showMatch = (width >= 1060 && maxLeftW >= 420);
    if (m_hLblMatchCount) {
        ShowWindow(m_hLblMatchCount, showMatch ? SW_SHOW : SW_HIDE);
    }

    int searchW = 180;
    int filterW = 130;
    const int matchW = 90;

    if (showMatch) {
        int spaceForSearchFilter = maxLeftW - matchW - 14;
        searchW = (std::min)(220, (std::max)(120, spaceForSearchFilter * 58 / 100));
        filterW = (std::min)(150, (std::max)(100, spaceForSearchFilter - searchW));
        if (m_hSearchEdit) MoveWindow(m_hSearchEdit, 10, btnY, searchW, 26, TRUE);
        if (m_hFilterCombo) MoveWindow(m_hFilterCombo, 10 + searchW + 6, btnY, filterW, 200, TRUE);
        MoveWindow(m_hLblMatchCount, 10 + searchW + 6 + filterW + 8, btnY + 3, matchW, 20, TRUE);
    } else {
        int spaceForSearchFilter = maxLeftW;
        if (spaceForSearchFilter > 320) {
            searchW = 180;
            filterW = 130;
        } else {
            searchW = (std::max)(110, spaceForSearchFilter * 58 / 100);
            filterW = (std::max)(95, spaceForSearchFilter - searchW - 6);
        }
        if (m_hSearchEdit) MoveWindow(m_hSearchEdit, 10, btnY, searchW, 26, TRUE);
        if (m_hFilterCombo) MoveWindow(m_hFilterCombo, 10 + searchW + 6, btnY, filterW, 200, TRUE);
    }

    // 2. Status Bar and Embedded Progress Bar
    int sbHeight = 24;
    if (m_hStatusBar) {
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        RECT rcSb{};
        GetWindowRect(m_hStatusBar, &rcSb);
        sbHeight = rcSb.bottom - rcSb.top;

        const int p0 = (std::min)(210, (std::max)(160, width / 4));
        const int p1 = p0 + 130;
        const int adminWidth = 175;
        const int p2 = (std::max)(p1 + 100, width - adminWidth);
        int parts[] = { p0, p1, p2, -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(parts));

        RECT rcPart1{};
        SendMessage(m_hStatusBar, SB_GETRECT, 1, reinterpret_cast<LPARAM>(&rcPart1));
        if (m_hProgressBar) {
            MoveWindow(m_hProgressBar, rcPart1.left + 2, rcPart1.top + 3, (rcPart1.right - rcPart1.left) - 4, (rcPart1.bottom - rcPart1.top) - 6, TRUE);
        }
    }

    // 3. Main Panes: Full-Width Grouped ListView (Top) + Details Inspector (Bottom)
    const int topY = 48;
    const int detailsH = 175;
    const int listH = (std::max)(150, height - topY - detailsH - sbHeight - 14);
    const int listW = width - 20;

    if (m_hListView) {
        MoveWindow(m_hListView, 10, topY, listW, listH, TRUE);

        const int fixedCols = 135 + 115 + 85 + 25;
        const int col0Width = (std::max)(160, listW - fixedCols);
        ListView_SetColumnWidth(m_hListView, 0, col0Width);
        ShowScrollBar(m_hListView, SB_HORZ, FALSE);
    }

    // Details Inspector Layout
    const int detailsTop = topY + listH + 8;
    const int btnTop = detailsTop + 2;

    if (m_hBtnToggleTweak) {
        MoveWindow(m_hBtnToggleTweak, width - 250, btnTop, 140, 24, TRUE);
    }
    if (m_hBtnCopyTweak) {
        MoveWindow(m_hBtnCopyTweak, width - 105, btnTop, 95, 24, TRUE);
    }
    if (m_hDetailsEdit) {
        const int editTop = detailsTop + 28;
        const int editH = (std::max)(60, height - editTop - sbHeight - 6);
        MoveWindow(m_hDetailsEdit, 10, editTop, width - 20, editH, TRUE);
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
    m_checkedStates.clear();

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
        const bool isProtected = (st == SettingStatus::Protected);

        if (filterMode == FilterMode::RecommendedOnly && t.safety != SafetyLevel::Safe) continue;
        if (filterMode == FilterMode::UnprotectedOnly && isProtected) continue;
        if (filterMode == FilterMode::ProtectedOnly && !isProtected) continue;

        m_displayedTweaks.push_back(t);
        m_checkedStates.push_back(isProtected);

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

        std::wstring stStr = isProtected ? L"● Protected" : L"○ Default";
        ListView_SetItemText(m_hListView, itemIndex, 1, const_cast<LPWSTR>(stStr.c_str()));

        std::wstring safeStr = (t.safety == SafetyLevel::Safe) ? L"Safe" :
                               ((t.safety == SafetyLevel::Normal) ? L"Normal" : L"Advanced");
        ListView_SetItemText(m_hListView, itemIndex, 2, const_cast<LPWSTR>(safeStr.c_str()));

        std::wstring scopeStr = (t.scope == TargetScope::Machine) ? L"Machine" :
                                ((t.scope == TargetScope::User) ? L"User" : L"Machine/User");
        ListView_SetItemText(m_hListView, itemIndex, 3, const_cast<LPWSTR>(scopeStr.c_str()));

        ListView_SetCheckState(m_hListView, itemIndex, isProtected);
        itemIndex++;
    }

    SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hListView, nullptr, TRUE);

    if (!m_displayedTweaks.empty()) {
        ListView_SetItemState(m_hListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        UpdateDetailsPane(0);
    } else {
        UpdateDetailsPane(-1);
    }

    UpdateSelectAllCheckboxState();
    UpdateStatusBar();
}

LRESULT MainWindow::OnCustomDraw(NMHDR* pnmhdr) {
    auto* pcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmhdr);
    switch (pcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        if (!DarkMode::IsDarkModeActive()) {
            if ((pcd->nmcd.dwItemSpec % 2) == 1) {
                pcd->clrTextBk = RGB(248, 250, 252);
            } else {
                pcd->clrTextBk = RGB(255, 255, 255);
            }
        }
        return CDRF_NOTIFYSUBITEMDRAW;
    }

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
        const int item = static_cast<int>(pcd->nmcd.dwItemSpec);
        const int subItem = pcd->iSubItem;

        if (item >= 0 && item < static_cast<int>(m_displayedTweaks.size())) {
            const auto& t = m_displayedTweaks[item];

            if (subItem == 1) { // Current Status
                const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
                if (st == SettingStatus::Protected) {
                    pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(108, 203, 135) : RGB(16, 124, 65);
                } else {
                    pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(160, 160, 160) : RGB(115, 120, 130);
                }
            } else if (subItem == 2) { // Safety Level
                if (t.safety == SafetyLevel::Safe) {
                    pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(108, 203, 135) : RGB(16, 124, 65);
                } else if (t.safety == SafetyLevel::Normal) {
                    pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(240, 180, 50) : RGB(175, 95, 0);
                } else {
                    pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(250, 100, 95) : RGB(196, 43, 28);
                }
            } else if (subItem == 3) { // Scope
                pcd->clrText = DarkMode::IsDarkModeActive() ? RGB(140, 185, 240) : RGB(60, 90, 130);
            }
        }
        return CDRF_NEWFONT;
    }
    }

    return CDRF_DODEFAULT;
}

void MainWindow::UpdateSelectAllCheckboxState() {
    if (!m_hChkSelectAll) return;
    const int total = static_cast<int>(m_displayedTweaks.size());
    if (total == 0) {
        SendMessage(m_hChkSelectAll, BM_SETCHECK, BST_UNCHECKED, 0);
        return;
    }

    int checkedCount = 0;
    for (int i = 0; i < total; ++i) {
        if (ListView_GetCheckState(m_hListView, i)) {
            checkedCount++;
        }
    }

    if (checkedCount == total) {
        SendMessage(m_hChkSelectAll, BM_SETCHECK, BST_CHECKED, 0);
    } else if (checkedCount == 0) {
        SendMessage(m_hChkSelectAll, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        SendMessage(m_hChkSelectAll, BM_SETCHECK, BST_INDETERMINATE, 0);
    }
}

void MainWindow::UpdateDetailsPane(int selectedIndex) {
    if (!m_hDetailsEdit) return;

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(m_displayedTweaks.size())) {
        SetWindowTextW(m_hDetailsEdit, L"Select any privacy tweak from the list to view its description, privacy impact, and technical registry specifications.");
        if (m_hBtnToggleTweak) EnableWindow(m_hBtnToggleTweak, FALSE);
        if (m_hBtnCopyTweak) EnableWindow(m_hBtnCopyTweak, FALSE);
        return;
    }

    if (m_hBtnToggleTweak) EnableWindow(m_hBtnToggleTweak, TRUE);
    if (m_hBtnCopyTweak) EnableWindow(m_hBtnCopyTweak, TRUE);

    const auto& t = m_displayedTweaks[selectedIndex];
    const bool isChecked = (ListView_GetCheckState(m_hListView, selectedIndex) != 0);

    if (m_hBtnToggleTweak) {
        SetWindowTextW(m_hBtnToggleTweak, isChecked ? L"Protected (Active)" : L"Enable Protection");
    }

    std::wstringstream ss;
    ss << t.title << L"\r\n";
    ss << L"Status: " << (isChecked ? L"Protected (Recommended)" : L"Default (Enabled)")
       << L"  |  Safety: " << (t.safety == SafetyLevel::Safe ? L"Safe (Recommended)" : (t.safety == SafetyLevel::Normal ? L"Normal" : L"Advanced"))
       << L"  |  Scope: " << (t.scope == TargetScope::Machine ? L"Machine (HKLM)" : (t.scope == TargetScope::User ? L"User (HKCU)" : L"Machine & User"))
       << L"  |  Category: " << t.category << L"\r\n\r\n";

    ss << L"DESCRIPTION:\r\n" << t.description << L"\r\n\r\n";

    if (!t.impact.empty()) {
        ss << L"PRIVACY IMPACT & RATIONALE:\r\n" << t.impact << L"\r\n\r\n";
    }

    if (!t.regActions.empty() || !t.serviceActions.empty()) {
        ss << L"TECHNICAL SPECIFICATIONS (REGISTRY & SERVICES):\r\n";
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

void MainWindow::ToggleCurrentTweakFromDetails() {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel == -1) sel = 0;
    if (sel >= 0 && sel < static_cast<int>(m_displayedTweaks.size())) {
        const BOOL cur = ListView_GetCheckState(m_hListView, sel);
        const BOOL nextState = !cur;
        ListView_SetCheckState(m_hListView, sel, nextState);
        UpdateDetailsPane(sel);
        UpdateSelectAllCheckboxState();
    }
}

void MainWindow::UpdateStatusBar() {
    const auto& all = TweakRegistry::Instance().GetAllTweaks();
    const int total = static_cast<int>(all.size());
    int protectedCount = 0;
    for (const auto& t : all) {
        if (TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {}) == SettingStatus::Protected) {
            protectedCount++;
        }
    }

    const int percent = (total > 0) ? ((protectedCount * 100) / total) : 0;
    std::wstring part1 = L"Privatized: " + std::to_wstring(protectedCount) + L" / " + std::to_wstring(total) + L" (" + std::to_wstring(percent) + L"%)";
    SendMessage(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(part1.c_str()));

    if (m_hProgressBar) {
        SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, total);
        SendMessage(m_hProgressBar, PBM_SETPOS, protectedCount, 0);
    }

    std::wstring part2 = L"Total Tweaks: " + std::to_wstring(m_displayedTweaks.size()) + L" displayed across categories";
    SendMessage(m_hStatusBar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(part2.c_str()));

    SendMessage(m_hStatusBar, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(L"Administrator (Elevated)"));
}

void MainWindow::OnContextMenu(HWND hWnd, int x, int y) {
    if (hWnd != m_hListView && GetParent(hWnd) != m_hListView) {
        return;
    }

    if (x == -1 && y == -1) {
        const int focus = ListView_GetNextItem(m_hListView, -1, LVNI_FOCUSED);
        if (focus != -1) {
            RECT rcItem{};
            ListView_GetItemRect(m_hListView, focus, &rcItem, LVIR_BOUNDS);
            POINT pt{ rcItem.left + 50, rcItem.bottom };
            ClientToScreen(m_hListView, &pt);
            x = pt.x;
            y = pt.y;
        } else {
            POINT pt{ 100, 100 };
            ClientToScreen(m_hListView, &pt);
            x = pt.x;
            y = pt.y;
        }
    }

    const int selectedCount = ListView_GetSelectedCount(m_hListView);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_PROTECT_SELECTED, L"Check Selected (Enable Protection)\tSpace");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_DEFAULT_SELECTED, L"Uncheck Selected (Set to Default)");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_INVERT_SELECTED, L"Invert Selected Check State");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_APPLY_SELECTED, L"Apply Selected Tweaks Immediately");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_SELECT_ALL, L"Select All Items\tCtrl+A");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_ID, L"Copy Tweak ID(s)");
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_DETAILS, L"Copy Tweak Details");

    if (selectedCount == 0) {
        EnableMenuItem(hMenu, IDM_CTX_PROTECT_SELECTED, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CTX_DEFAULT_SELECTED, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CTX_INVERT_SELECTED, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CTX_APPLY_SELECTED, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CTX_COPY_ID, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CTX_COPY_DETAILS, MF_BYCOMMAND | MF_GRAYED);
    }

    TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, x, y, m_hWnd, nullptr);
    DestroyMenu(hMenu);
}

void MainWindow::SetSelectedItemsChecked(bool checked) {
    int i = -1;
    while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
        ListView_SetCheckState(m_hListView, i, checked ? TRUE : FALSE);
    }
    UpdateSelectAllCheckboxState();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::InvertSelectedItemsChecked() {
    int i = -1;
    while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
        const BOOL cur = ListView_GetCheckState(m_hListView, i);
        ListView_SetCheckState(m_hListView, i, !cur);
    }
    UpdateSelectAllCheckboxState();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::SelectAllListItems() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        ListView_SetItemState(m_hListView, i, LVIS_SELECTED, LVIS_SELECTED);
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
    ss << L"Tweak: " << t.title << L" [" << std::wstring(t.id.begin(), t.id.end()) << L"]\r\n";
    ss << L"Category: " << t.category << L"\r\n";
    ss << L"Description: " << t.description << L"\r\n";
    ss << L"Impact: " << t.impact << L"\r\n";

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

void MainWindow::ApplySelectedItemsOnly() {
    const int count = ListView_GetSelectedCount(m_hListView);
    if (count == 0) return;

    int applied = 0;
    int i = -1;
    while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
        const bool checked = (ListView_GetCheckState(m_hListView, i) != 0);
        const bool ok = TweakRegistry::Instance().ApplyTweak(m_displayedTweaks[i].id, checked, UserSelectionMode::AllUsers, {});
        if (ok) applied++;
    }

    RefreshAuditState();
    MessageBoxW(m_hWnd, (std::to_wstring(applied) + L" selected setting(s) applied.").c_str(), L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::OnCommand(int id, HWND hCtrl) {
    switch (id) {
    case IDM_FILE_EXPORT:
        ExportConfiguration();
        break;
    case IDM_FILE_IMPORT:
        ImportConfiguration();
        break;
    case IDM_FILE_EXIT:
        DestroyWindow(m_hWnd);
        break;
    case IDM_TPL_RECOMMENDED:
        ApplyTemplate("recommended");
        break;
    case IDM_TPL_STRICT:
        ApplyTemplate("strict");
        break;
    case IDM_TPL_MINIMAL:
        ApplyTemplate("minimal");
        break;
    case IDM_TPL_DEFAULTS:
        RevertAllToDefaults();
        break;
    case IDM_ACT_APPLY:
    case IDC_BTN_APPLY:
        ApplyCurrentSelection();
        break;
    case IDM_ACT_INVERT:
        for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
            const BOOL cur = ListView_GetCheckState(m_hListView, i);
            ListView_SetCheckState(m_hListView, i, !cur);
        }
        UpdateSelectAllCheckboxState();
        break;
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
            L"PrivatizeWin v1.2\n"
            L"Open-source Windows Privacy & Telemetry Silencer\n"
            L"Minimalist, zero-footprint Sysinternals-grade utility.\n\n"
            L"Copyright (C) 2026 PrivatizeWin Project (MIT License)",
            L"About PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        break;
    case IDM_HELP_GITHUB:
        ShellExecuteW(nullptr, L"open", L"https://github.com/saverchenkov/PrivatizeWin", nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case IDC_DETAILS_BTN_TOGGLE:
        ToggleCurrentTweakFromDetails();
        break;
    case IDC_DETAILS_BTN_COPY:
        CopySelectedTweakDetails();
        break;
    case IDC_CHK_CHECK_ALL: {
        const LRESULT state = SendMessage(m_hChkSelectAll, BM_GETCHECK, 0, 0);
        const bool shouldCheck = (state == BST_CHECKED);
        for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
            ListView_SetCheckState(m_hListView, i, shouldCheck ? TRUE : FALSE);
        }
        int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
        if (cur != -1) {
            UpdateDetailsPane(cur);
        }
        break;
    }
    case IDM_CTX_PROTECT_SELECTED:
        SetSelectedItemsChecked(true);
        break;
    case IDM_CTX_DEFAULT_SELECTED:
        SetSelectedItemsChecked(false);
        break;
    case IDM_CTX_INVERT_SELECTED:
        InvertSelectedItemsChecked();
        break;
    case IDM_CTX_APPLY_SELECTED:
        ApplySelectedItemsOnly();
        break;
    case IDM_CTX_SELECT_ALL:
        SelectAllListItems();
        break;
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
            else if (sel == 1) m_filterMode = FilterMode::UnprotectedOnly;
            else if (sel == 2) m_filterMode = FilterMode::ProtectedOnly;
            else if (sel == 3) m_filterMode = FilterMode::RecommendedOnly;
            PopulateListView(m_currentFilter, m_filterMode);
        }
        break;
    case IDC_TPL_COMBO:
        if (HIWORD(reinterpret_cast<DWORD_PTR>(hCtrl)) == CBN_SELCHANGE) {
            const int sel = static_cast<int>(SendMessage(m_hTemplateCombo, CB_GETCURSEL, 0, 0));
            if (sel == 0) ApplyTemplate("recommended");
            else if (sel == 1) ApplyTemplate("strict");
            else if (sel == 2) ApplyTemplate("minimal");
            else if (sel == 3) RevertAllToDefaults();
        }
        break;
    case IDC_BTN_REVERT:
        RevertAllToDefaults();
        break;
    }
}

void MainWindow::OnNotify(NMHDR* pnmhdr) {
    if (pnmhdr->idFrom == IDC_LIST_TWEAKS) {
        if (pnmhdr->code == LVN_ITEMCHANGED) {
            auto* pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmhdr);
            if (pnmv->uNewState & LVIS_SELECTED) {
                UpdateDetailsPane(pnmv->iItem);
            }
            if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) != (pnmv->uOldState & LVIS_STATEIMAGEMASK)) {
                UpdateSelectAllCheckboxState();
                int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                if (sel == pnmv->iItem) {
                    UpdateDetailsPane(sel);
                }
            }
        } else if (pnmhdr->code == LVN_KEYDOWN) {
            auto* pnkd = reinterpret_cast<NMLVKEYDOWN*>(pnmhdr);
            if (pnkd->wVKey == VK_SPACE) {
                const int selCount = ListView_GetSelectedCount(m_hListView);
                if (selCount > 1) {
                    const int first = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                    const BOOL target = !ListView_GetCheckState(m_hListView, first);
                    SetSelectedItemsChecked(target != 0);
                }
            } else if (pnkd->wVKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                SelectAllListItems();
            }
        } else if (pnmhdr->code == NM_RCLICK) {
            POINT pt{};
            GetCursorPos(&pt);
            OnContextMenu(m_hListView, pt.x, pt.y);
        }
    }
}

void MainWindow::ApplyTemplate(std::string_view templateName) {
    const auto tpl = TemplateManager::Instance().GetTemplate(templateName);
    if (!tpl.has_value()) return;

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        auto it = tpl->tweakStates.find(t.id);
        const bool shouldCheck = (it != tpl->tweakStates.end()) ? it->second : false;
        ListView_SetCheckState(m_hListView, i, shouldCheck);
    }
    UpdateSelectAllCheckboxState();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::ApplyCurrentSelection() {
    std::unordered_map<std::string, bool> desired;
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const bool checked = (ListView_GetCheckState(m_hListView, i) != 0);
        desired[m_displayedTweaks[i].id] = checked;
    }

    std::vector<std::string> appliedIds;
    std::vector<std::string> failedIds;

    for (const auto& [id, shouldProtect] : desired) {
        const bool ok = TweakRegistry::Instance().ApplyTweak(id, shouldProtect, UserSelectionMode::AllUsers, {});
        if (ok) {
            appliedIds.push_back(id);
        } else {
            failedIds.push_back(id);
        }
    }

    RefreshAuditState();

    std::wstring msg = L"Successfully applied " + std::to_wstring(appliedIds.size()) + L" tweak(s).";
    if (!failedIds.empty()) {
        msg += L"\nWarning: " + std::to_wstring(failedIds.size()) + L" tweak(s) could not be applied.";
    }

    MessageBoxW(m_hWnd, msg.c_str(), L"PrivatizeWin", MB_OK | (failedIds.empty() ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void MainWindow::RevertAllToDefaults() {
    const int choice = MessageBoxW(m_hWnd,
        L"Are you sure you want to revert all privacy settings to default Windows behavior?",
        L"Confirm Revert Defaults",
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (choice != IDYES) return;

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        ListView_SetCheckState(m_hListView, i, FALSE);
    }
    UpdateSelectAllCheckboxState();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }

    ApplyCurrentSelection();
}

void MainWindow::RefreshAuditState() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        const bool isProtected = (st == SettingStatus::Protected);
        m_checkedStates[i] = isProtected;
        ListView_SetCheckState(m_hListView, i, isProtected);

        std::wstring stStr = isProtected ? L"● Protected" : L"○ Default";
        ListView_SetItemText(m_hListView, i, 1, const_cast<LPWSTR>(stStr.c_str()));
    }
    UpdateSelectAllCheckboxState();
    UpdateStatusBar();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
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
            p.tweakStates[m_displayedTweaks[i].id] = (ListView_GetCheckState(m_hListView, i) != 0);
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
            for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
                auto it = p.tweakStates.find(m_displayedTweaks[i].id);
                if (it != p.tweakStates.end()) {
                    ListView_SetCheckState(m_hListView, i, it->second ? TRUE : FALSE);
                }
            }
            UpdateSelectAllCheckboxState();

            int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
            if (cur != -1) {
                UpdateDetailsPane(cur);
            }

            MessageBoxW(m_hWnd, L"Configuration imported successfully. Review settings and click 'Apply Changes'.", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Failed to load or parse configuration file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::CreateSystemRestorePoint() {
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
