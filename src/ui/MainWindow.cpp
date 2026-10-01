#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "MainWindow.h"
#include "DarkMode.h"
#include "ScheduleDialog.h"
#include "DetailsView.h"
#include "../../res/resource.h"
#include "../core/TweakRegistry.h"
#include "../core/TemplateManager.h"
#include "../core/RestorePoint.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <sstream>
#include <algorithm>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace PrivatizeWin {

static std::unique_ptr<MainWindow> s_pMainWnd = nullptr;

bool MainWindow::RegisterClass(HINSTANCE hInstance) {
    DetailsView::RegisterClass(hInstance);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PrivatizeWin_MainWindow";
    wc.hbrBackground = nullptr; // Handled in WM_ERASEBKGND
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

            const bool isDark = DarkMode::IsDarkModeActive();
            const COLORREF colBar = isDark ? RGB(38, 40, 44) : RGB(246, 248, 250);
            const COLORREF colLine = isDark ? RGB(55, 58, 64) : RGB(220, 224, 230);
            const COLORREF colBg = isDark ? RGB(28, 28, 30) : RGB(240, 242, 245);
            const COLORREF colSplitter = isDark ? RGB(50, 52, 58) : RGB(218, 222, 228);

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

            // Subtle splitter guidelines
            if (s_pMainWnd) {
                HPEN hPenSplitter = CreatePen(PS_SOLID, 1, colSplitter);
                SelectObject(hdc, hPenSplitter);
                MoveToEx(hdc, s_pMainWnd->m_splitterX, 48, nullptr);
                LineTo(hdc, s_pMainWnd->m_splitterX, s_pMainWnd->m_splitterY - 2);

                MoveToEx(hdc, 10, s_pMainWnd->m_splitterY + 1, nullptr);
                LineTo(hdc, w - 10, s_pMainWnd->m_splitterY + 1);

                DeleteObject(hPenSplitter);
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
        case WM_SETCURSOR: {
            POINT pt{};
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            RECT rcClient{};
            GetClientRect(hWnd, &rcClient);

            if (s_pMainWnd->m_dragMode == SplitterDragMode::Vertical) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
            if (s_pMainWnd->m_dragMode == SplitterDragMode::Horizontal) {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                return TRUE;
            }

            // Hover over vertical splitter
            if (pt.x >= s_pMainWnd->m_splitterX - 4 && pt.x <= s_pMainWnd->m_splitterX + 6 &&
                pt.y >= 48 && pt.y < s_pMainWnd->m_splitterY) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }

            // Hover over horizontal splitter
            if (pt.y >= s_pMainWnd->m_splitterY - 4 && pt.y <= s_pMainWnd->m_splitterY + 6 &&
                pt.y >= 48 && pt.x >= 10 && pt.x <= rcClient.right - 10) {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                return TRUE;
            }
            break;
        }
        case WM_LBUTTONDOWN: {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            RECT rcClient{};
            GetClientRect(hWnd, &rcClient);

            if (x >= s_pMainWnd->m_splitterX - 4 && x <= s_pMainWnd->m_splitterX + 4 &&
                y >= 48 && y < s_pMainWnd->m_splitterY) {
                s_pMainWnd->m_dragMode = SplitterDragMode::Vertical;
                SetCapture(hWnd);
                return 0;
            }
            if (y >= s_pMainWnd->m_splitterY - 4 && y <= s_pMainWnd->m_splitterY + 4 &&
                y >= 48 && x >= 10 && x <= rcClient.right - 10) {
                s_pMainWnd->m_dragMode = SplitterDragMode::Horizontal;
                SetCapture(hWnd);
                return 0;
            }
            break;
        }
        case WM_MOUSEMOVE: {
            if (s_pMainWnd->m_dragMode != SplitterDragMode::None) {
                const int x = GET_X_LPARAM(lParam);
                const int y = GET_Y_LPARAM(lParam);
                RECT rcClient{};
                GetClientRect(hWnd, &rcClient);
                const int width = rcClient.right - rcClient.left;
                const int height = rcClient.bottom - rcClient.top;

                if (s_pMainWnd->m_dragMode == SplitterDragMode::Vertical) {
                    s_pMainWnd->m_splitterX = (std::clamp)(x, 180, width - 260);
                } else if (s_pMainWnd->m_dragMode == SplitterDragMode::Horizontal) {
                    s_pMainWnd->m_splitterY = (std::clamp)(y, 160, height - 140);
                }
                s_pMainWnd->OnSize(width, height);
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            break;
        }
        case WM_LBUTTONUP: {
            if (s_pMainWnd->m_dragMode != SplitterDragMode::None) {
                s_pMainWnd->m_dragMode = SplitterDragMode::None;
                ReleaseCapture();
                return 0;
            }
            break;
        }
        case WM_CAPTURECHANGED: {
            if (s_pMainWnd) {
                s_pMainWnd->m_dragMode = SplitterDragMode::None;
            }
            return 0;
        }
        case WM_CANCELMODE: {
            if (s_pMainWnd && s_pMainWnd->m_dragMode != SplitterDragMode::None) {
                s_pMainWnd->m_dragMode = SplitterDragMode::None;
                ReleaseCapture();
            }
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

static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
    static bool s_isDraggingSelection = false;
    static int s_anchorItem = -1;

    switch (uMsg) {
    case WM_LBUTTONDOWN: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        LVHITTESTINFO hti{};
        hti.pt.x = x;
        hti.pt.y = y;
        ListView_SubItemHitTest(hWnd, &hti);

        // If clicking on a checkbox state icon, let default handle check toggle
        if (hti.flags & LVHT_ONITEMSTATEICON) {
            s_isDraggingSelection = false;
            s_anchorItem = -1;
            break;
        }

        if (hti.iItem != -1) {
            s_isDraggingSelection = true;
            s_anchorItem = hti.iItem;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (s_isDraggingSelection && (wParam & MK_LBUTTON)) {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            LVHITTESTINFO hti{};
            hti.pt.x = x;
            hti.pt.y = y;
            ListView_SubItemHitTest(hWnd, &hti);

            if (hti.iItem != -1 && s_anchorItem != -1) {
                const int start = (std::min)(s_anchorItem, hti.iItem);
                const int end = (std::max)(s_anchorItem, hti.iItem);
                const int count = ListView_GetItemCount(hWnd);

                SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
                for (int i = 0; i < count; ++i) {
                    const UINT state = (i >= start && i <= end) ? LVIS_SELECTED : 0;
                    ListView_SetItemState(hWnd, i, state, LVIS_SELECTED);
                }
                ListView_SetItemState(hWnd, hti.iItem, LVIS_FOCUSED, LVIS_FOCUSED);
                SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hWnd, nullptr, FALSE);

                RECT rcClient{};
                GetClientRect(hWnd, &rcClient);
                if (y < 20) {
                    SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                } else if (y > rcClient.bottom - 20) {
                    SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                }

                // Prevent SysListView32 default drag tracking that can snap cursor to (0, 0)
                return 0;
            }
        }
        break;
    }
    case WM_LBUTTONUP: {
        s_isDraggingSelection = false;
        s_anchorItem = -1;
        break;
    }
    case WM_CAPTURECHANGED:
    case WM_CANCELMODE: {
        s_isDraggingSelection = false;
        s_anchorItem = -1;
        break;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {}

MainWindow::~MainWindow() {
    if (m_hListView) {
        RemoveWindowSubclass(m_hListView, ListViewSubclassProc, 1001);
    }
    if (m_hFontRegular) DeleteObject(m_hFontRegular);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontCode) DeleteObject(m_hFontCode);
    if (m_hFontBadge) DeleteObject(m_hFontBadge);
    if (m_hTreeImageList) ImageList_Destroy(m_hTreeImageList);
    if (m_hRowImageList) ImageList_Destroy(m_hRowImageList);
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
    m_hFontBadge   = makeFont(8, FW_BOLD, L"Segoe UI");
}

void MainWindow::InitializeTreeIcons() {
    m_hTreeImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 10, 5);
    if (!m_hTreeImageList) return;

    auto addStock = [this](SHSTOCKICONID sid) {
        SHSTOCKICONINFO sii{ sizeof(sii) };
        if (SUCCEEDED(SHGetStockIconInfo(sid, SHGSI_ICON | SHGSI_SMALLICON, &sii))) {
            ImageList_AddIcon(m_hTreeImageList, sii.hIcon);
            DestroyIcon(sii.hIcon);
        }
    };

    addStock(SIID_APPLICATION);        // 0: All Settings
    addStock(SIID_HELP);               // 1: AI & Copilot
    addStock(SIID_DOCASSOC);           // 2: Activity History / Docs
    addStock(SIID_LOCK);               // 3: App Permissions / Privacy
    addStock(SIID_FIND);               // 4: Cortana & Search
    addStock(SIID_MEDIACOMPACTFLASH);  // 5: Gaming & Xbox
    addStock(SIID_WORLD);              // 6: Location / Edge
    addStock(SIID_DESKTOPPC);          // 7: Desktop / Lock Screen
    addStock(SIID_FOLDER);             // 8: Explorer / Misc
    addStock(SIID_DEVICECELLPHONE);    // 9: Mobile Devices
    addStock(SIID_SHIELD);             // 10: Security & Network
    addStock(SIID_SETTINGS);           // 11: Sync / Windows Update
    addStock(SIID_DRIVEFIXED);         // 12: Telemetry & Diagnostics
}

void MainWindow::OnCreate() {
    DarkMode::ApplyToWindow(m_hWnd);

    const HICON hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_APPICON));
    if (hIcon) {
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    }

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    InitializeFonts();
    InitializeTreeIcons();
    InitializeControls();

    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    PopulateCategories();
    PopulateListView(L"All Settings", L"", FilterMode::All);
    UpdateStatusBar();
}

void MainWindow::InitializeControls() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 1. Search Box (Direct child of m_hWnd)
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
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Safe (Recommended)"));
    SendMessage(m_hFilterCombo, CB_SETCURSEL, 0, 0);

    // 3. Match Count Label
    m_hLblMatchCount = CreateWindowW(L"STATIC", L"260 of 260 shown",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        338, 13, 95, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_LBL_MATCH_COUNT), hInst, nullptr);
    SendMessage(m_hLblMatchCount, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 4. Template Selector
    m_hTemplateCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        440, 10, 150, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_TPL_COMBO), hInst, nullptr);
    SendMessage(m_hTemplateCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended (Safe)"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strict Privacy"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal Telemetry"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Factory Defaults"));
    SendMessage(m_hTemplateCombo, CB_SETCURSEL, 0, 0);

    // 5. Check All Checkbox
    m_hChkSelectAll = CreateWindowW(WC_BUTTONW, L"Check All",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        598, 13, 80, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_CHK_CHECK_ALL), hInst, nullptr);
    SendMessage(m_hChkSelectAll, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 6. Action Buttons
    m_hBtnRefresh = CreateWindowW(WC_BUTTONW, L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        684, 10, 66, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REFRESH), hInst, nullptr);
    SendMessage(m_hBtnRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnRevert = CreateWindowW(WC_BUTTONW, L"Revert Defaults",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        756, 10, 106, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REVERT), hInst, nullptr);
    SendMessage(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    m_hBtnApply = CreateWindowW(WC_BUTTONW, L"Apply Changes",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        868, 10, 115, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_APPLY), hInst, nullptr);
    SendMessage(m_hBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    // Category TreeView (Default width 285px to avoid text clipping)
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        10, 48, m_splitterX - 15, m_splitterY - 50, m_hWnd, reinterpret_cast<HMENU>(IDC_TREE_CATEGORIES), hInst, nullptr);
    SendMessage(m_hTreeView, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hTreeView, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
    if (m_hTreeImageList) {
        TreeView_SetImageList(m_hTreeView, m_hTreeImageList, TVSIL_NORMAL);
    }
    SetWindowTheme(m_hTreeView, L"Explorer", nullptr);
    DarkMode::ApplyToControl(m_hTreeView);

    // Main ListView (Multi-Select Enabled)
    m_hListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        m_splitterX + 5, 48, 1140 - m_splitterX - 15, m_splitterY - 50, m_hWnd, reinterpret_cast<HMENU>(IDC_LIST_TWEAKS), hInst, nullptr);
    SendMessage(m_hListView, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    SetWindowSubclass(m_hListView, ListViewSubclassProc, 1001, 0);

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
    lvc.cx = 450;
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

    // Modern Details View (Card Layout Inspector)
    m_hDetailsView = DetailsView::Create(m_hWnd, hInst, IDC_DETAILS_VIEW,
        10, m_splitterY + 4, 1140 - 20, 200);

    auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
    if (pDV) {
        pDV->SetFonts(m_hFontRegular, m_hFontBold, m_hFontTitle, m_hFontCode, m_hFontBadge);
        pDV->SetDarkMode(DarkMode::IsDarkModeActive());
    }

    // Status Bar
    m_hStatusBar = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), hInst, nullptr);
    int sbParts[] = { 230, 380, 720, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(sbParts));
    SendMessage(m_hStatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // Embedded Progress Bar in Status Bar (Part 1)
    m_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        0, 0, 0, 0, m_hStatusBar, reinterpret_cast<HMENU>(IDC_STATUS_PROGRESS), hInst, nullptr);
}

void MainWindow::OnSize(int width, int height) {
    // 1. Responsive Toolbar Layout (Prevents Any Overlap Under Any Dimension)
    const int btnY = 10;
    const int applyW = 112;
    const int revertW = 104;
    const int refreshW = 64;
    const int chkAllW = 78;
    const int tplW = 140;

    int rightX = width - 12;

    // Fixed essential action buttons positioned from right edge
    rightX -= applyW;
    if (m_hBtnApply) MoveWindow(m_hBtnApply, rightX, btnY, applyW, 26, TRUE);

    rightX -= (revertW + 6);
    if (m_hBtnRevert) MoveWindow(m_hBtnRevert, rightX, btnY, revertW, 26, TRUE);

    rightX -= (refreshW + 6);
    if (m_hBtnRefresh) MoveWindow(m_hBtnRefresh, rightX, btnY, refreshW, 26, TRUE);

    rightX -= (chkAllW + 8);
    if (m_hChkSelectAll) MoveWindow(m_hChkSelectAll, rightX, btnY + 3, chkAllW, 20, TRUE);

    // Responsive collapsing:
    // If width >= 940: Show template combo on the right
    // If width < 940: Hide template combo (accessible in Templates menu) to prevent cramped toolbar
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

    // Left toolbar controls: Search box, Filter dropdown, Match count
    // Guaranteed boundary: left controls must end at least 12px before rightX
    const int maxLeftW = (std::max)(120, rightX - 12 - 10); // 10 is left margin

    // Breakpoint for match count label
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

        // Dynamic status bar parts:
        // Part 0: Summary count (~210px)
        // Part 1: Progress bar (~130px)
        // Part 2: Category text (stretches to fill middle)
        // Part 3: Administrator status (docked to right edge)
        const int p0 = (std::min)(210, (std::max)(160, width / 4));
        const int p1 = p0 + 130;
        const int adminWidth = 175;
        const int p2 = (std::max)(p1 + 100, width - adminWidth);
        int parts[] = { p0, p1, p2, -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(parts));

        // Position progress bar in Part 1
        RECT rcPart1{};
        SendMessage(m_hStatusBar, SB_GETRECT, 1, reinterpret_cast<LPARAM>(&rcPart1));
        if (m_hProgressBar) {
            MoveWindow(m_hProgressBar, rcPart1.left + 2, rcPart1.top + 3, (rcPart1.right - rcPart1.left) - 4, (rcPart1.bottom - rcPart1.top) - 6, TRUE);
        }
    }

    // 3. Main Splitters and Panes
    const int topY = 48;
    const int contentHeight = height - topY - sbHeight - 6;
    if (m_splitterY > contentHeight - 100) m_splitterY = (std::max)(160, contentHeight - 120);
    if (m_splitterY < 160) m_splitterY = 160;
    if (m_splitterX > width - 260) m_splitterX = (std::max)(180, width - 280);
    if (m_splitterX < 180) m_splitterX = 180;

    const int listHeight = m_splitterY - topY - 2;

    // TreeView (Left)
    MoveWindow(m_hTreeView, 10, topY, m_splitterX - 15, listHeight, TRUE);

    // ListView (Right)
    const int listX = m_splitterX + 5;
    const int listWidth = width - listX - 10;
    MoveWindow(m_hListView, listX, topY, listWidth, listHeight, TRUE);

    // Auto-stretch column 0 to eliminate empty right gap and prevent horizontal scrollbar
    const int fixedCols = 135 + 115 + 85 + 25; // Status + Safety + Scope + scrollbar reserve
    const int col0Width = (std::max)(140, listWidth - fixedCols);
    ListView_SetColumnWidth(m_hListView, 0, col0Width);
    ShowScrollBar(m_hListView, SB_HORZ, FALSE);

    // Details View (Bottom)
    const int detailsTop = m_splitterY + 4;
    const int detailsHeight = height - detailsTop - sbHeight - 4;
    if (detailsHeight > 0 && m_hDetailsView) {
        MoveWindow(m_hDetailsView, 10, detailsTop, width - 20, detailsHeight, TRUE);
    }
}

void MainWindow::PopulateCategories() {
    TreeView_DeleteAllItems(m_hTreeView);
    const auto categories = TweakRegistry::Instance().GetCategories();

    auto getCategoryIcon = [](std::wstring_view name) -> int {
        if (name == L"All Settings") return 0;
        if (name == L"AI & Copilot") return 1;
        if (name == L"Activity History & Clipboard") return 2;
        if (name == L"App Permissions & Hardware Access") return 3;
        if (name == L"Cortana & Search") return 4;
        if (name == L"Gaming & Xbox") return 5;
        if (name == L"Location & Sensors") return 6;
        if (name == L"Lock Screen & Desktop") return 7;
        if (name == L"Microsoft Edge") return 6;
        if (name == L"Miscellaneous") return 8;
        if (name == L"Mobile Devices & Phone Link") return 9;
        if (name == L"Office & Outlook") return 2;
        if (name == L"Privacy & Tracking") return 3;
        if (name == L"Security & Network") return 10;
        if (name == L"Synchronization") return 11;
        if (name == L"Taskbar & Start Menu") return 0;
        if (name == L"Telemetry & Diagnostics") return 12;
        if (name == L"Windows Explorer") return 8;
        if (name == L"Windows Update") return 11;
        return 0;
    };

    for (const auto& cat : categories) {
        std::wstring label = cat.name + L" (" + std::to_wstring(cat.totalCount) + L")";
        const int icon = getCategoryIcon(cat.name);

        TVINSERTSTRUCTW tvis{};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = const_cast<LPWSTR>(label.c_str());
        tvis.item.iImage = icon;
        tvis.item.iSelectedImage = icon;
        TreeView_InsertItem(m_hTreeView, &tvis);
    }
}

void MainWindow::PopulateListView(std::wstring_view category, std::wstring_view filter, FilterMode mode) {
    ListView_DeleteAllItems(m_hListView);
    auto allInCat = TweakRegistry::Instance().GetTweaksByCategory(category);
    const size_t totalCatCount = allInCat.size();

    std::wstring lowerFilter(filter);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::towlower);

    std::vector<Tweak> filtered;
    filtered.reserve(allInCat.size());

    for (const auto& t : allInCat) {
        // 1. Text Search Filter
        if (!lowerFilter.empty()) {
            std::wstring lowerTitle = t.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::towlower);
            std::string lowerId = t.id;
            std::transform(lowerId.begin(), lowerId.end(), lowerId.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            std::wstring wId(lowerId.begin(), lowerId.end());

            if (lowerTitle.find(lowerFilter) == std::wstring::npos && wId.find(lowerFilter) == std::wstring::npos) {
                continue;
            }
        }

        // 2. Smart Mode Filter
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        if (mode == FilterMode::UnprotectedOnly && st == SettingStatus::Protected) {
            continue;
        }
        if (mode == FilterMode::ProtectedOnly && st != SettingStatus::Protected) {
            continue;
        }
        if (mode == FilterMode::SafeOnly && t.safety != SafetyLevel::Safe) {
            continue;
        }

        filtered.push_back(t);
    }

    m_displayedTweaks = std::move(filtered);
    m_checkedStates.assign(m_displayedTweaks.size(), false);

    // Update match count label
    if (m_hLblMatchCount) {
        std::wstring countStr = std::to_wstring(m_displayedTweaks.size()) + L" of " + std::to_wstring(totalCatCount) + L" shown";
        SetWindowTextW(m_hLblMatchCount, countStr.c_str());
    }

    SendMessage(m_hListView, WM_SETREDRAW, FALSE, 0);

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];

        LVITEMW lvi{};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(t.title.c_str());
        ListView_InsertItem(m_hListView, &lvi);

        // Status text with symbol
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        std::wstring stStr = (st == SettingStatus::Protected) ? L"● Protected" : L"○ Default";
        ListView_SetItemText(m_hListView, i, 1, const_cast<LPWSTR>(stStr.c_str()));

        // Safety text with icon
        std::wstring safeStr = L"✔ Safe";
        if (t.safety == SafetyLevel::Normal) safeStr = L"⚠ Normal";
        else if (t.safety == SafetyLevel::Advanced) safeStr = L"⚡ Advanced";
        ListView_SetItemText(m_hListView, i, 2, const_cast<LPWSTR>(safeStr.c_str()));

        // Scope
        std::wstring scopeStr = L"Machine";
        if (t.scope == TargetScope::User) scopeStr = L"User";
        else if (t.scope == TargetScope::Both) scopeStr = L"Both";
        else if (t.scope == TargetScope::Service) scopeStr = L"Service";
        ListView_SetItemText(m_hListView, i, 3, const_cast<LPWSTR>(scopeStr.c_str()));

        // Initial checkbox state matches audited protection
        const bool isProtected = (st == SettingStatus::Protected);
        m_checkedStates[i] = isProtected;
        ListView_SetCheckState(m_hListView, i, isProtected);
    }

    SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hListView, nullptr, TRUE);

    UpdateSelectAllCheckboxState();

    if (!m_displayedTweaks.empty()) {
        UpdateDetailsPane(0);
    } else {
        auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
        if (pDV) {
            pDV->SetTweak(nullptr, false);
        }
    }
}

LRESULT MainWindow::OnCustomDraw(NMHDR* pnmhdr) {
    auto* pcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmhdr);
    switch (pcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        // Zebra striping
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
    auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
    if (!pDV) return;

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(m_displayedTweaks.size())) {
        pDV->SetTweak(nullptr, false);
        return;
    }

    const auto& t = m_displayedTweaks[selectedIndex];
    const bool isChecked = (ListView_GetCheckState(m_hListView, selectedIndex) != 0);
    pDV->SetTweak(&t, isChecked);
}

void MainWindow::ToggleCurrentTweakFromDetails() {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel == -1) sel = 0;
    if (sel >= 0 && sel < static_cast<int>(m_displayedTweaks.size())) {
        const BOOL cur = ListView_GetCheckState(m_hListView, sel);
        const BOOL nextState = !cur;
        ListView_SetCheckState(m_hListView, sel, nextState);

        auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
        if (pDV) {
            pDV->SetCheckedState(nextState != 0);
        }
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

    // Progress bar update
    if (m_hProgressBar) {
        SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, total);
        SendMessage(m_hProgressBar, PBM_SETPOS, protectedCount, 0);
    }

    std::wstring part2 = L"Category: " + m_currentCategory + L" (" + std::to_wstring(m_displayedTweaks.size()) + L" shown)";
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
        auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
        if (pDV) pDV->SetCheckedState(checked);
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
        auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
        if (pDV) pDV->SetCheckedState(ListView_GetCheckState(m_hListView, cur) != 0);
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
            L"PrivatizeWin v1.1\n"
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
    case IDC_CHK_CHECK_ALL: {
        const LRESULT state = SendMessage(m_hChkSelectAll, BM_GETCHECK, 0, 0);
        const bool shouldCheck = (state == BST_CHECKED);
        for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
            ListView_SetCheckState(m_hListView, i, shouldCheck ? TRUE : FALSE);
        }
        int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
        if (cur != -1) {
            auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
            if (pDV) pDV->SetCheckedState(shouldCheck);
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
            PopulateListView(m_currentCategory, m_currentFilter, m_filterMode);
        }
        break;
    case IDC_FILTER_COMBO:
        if (HIWORD(reinterpret_cast<DWORD_PTR>(hCtrl)) == CBN_SELCHANGE) {
            const int sel = static_cast<int>(SendMessage(m_hFilterCombo, CB_GETCURSEL, 0, 0));
            if (sel == 0) m_filterMode = FilterMode::All;
            else if (sel == 1) m_filterMode = FilterMode::UnprotectedOnly;
            else if (sel == 2) m_filterMode = FilterMode::ProtectedOnly;
            else if (sel == 3) m_filterMode = FilterMode::SafeOnly;
            else if (sel == 4) m_filterMode = FilterMode::PendingChanges;
            PopulateListView(m_currentCategory, m_currentFilter, m_filterMode);
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
    if (pnmhdr->idFrom == IDC_TREE_CATEGORIES) {
        if (pnmhdr->code == TVN_SELCHANGEDW) {
            auto* pnmtv = reinterpret_cast<LPNMTREEVIEWW>(pnmhdr);
            wchar_t textBuf[128]{};
            TVITEMW tvi{};
            tvi.hItem = pnmtv->itemNew.hItem;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = textBuf;
            tvi.cchTextMax = 128;
            TreeView_GetItem(m_hTreeView, &tvi);

            std::wstring cat(textBuf);
            const size_t paren = cat.find(L" (");
            if (paren != std::wstring::npos) {
                cat = cat.substr(0, paren);
            }
            m_currentCategory = std::move(cat);
            PopulateListView(m_currentCategory, m_currentFilter, m_filterMode);
            UpdateStatusBar();
        }
    } else if (pnmhdr->idFrom == IDC_LIST_TWEAKS) {
        if (pnmhdr->code == LVN_BEGINDRAG || pnmhdr->code == LVN_BEGINRDRAG) {
            ReleaseCapture();
            return;
        }
        if (pnmhdr->code == LVN_ITEMCHANGED) {
            auto* pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmhdr);
            if (pnmv->uNewState & LVIS_SELECTED) {
                UpdateDetailsPane(pnmv->iItem);
            }
            // Checkbox state changed
            if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) != (pnmv->uOldState & LVIS_STATEIMAGEMASK)) {
                UpdateSelectAllCheckboxState();
                int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                if (sel == pnmv->iItem) {
                    auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
                    if (pDV) pDV->SetCheckedState(ListView_GetCheckState(m_hListView, sel) != 0);
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
        auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
        if (pDV) pDV->SetCheckedState(ListView_GetCheckState(m_hListView, cur) != 0);
    }
}

void MainWindow::ApplyCurrentSelection() {
    const int res = MessageBoxW(m_hWnd,
        L"Would you like to create a System Restore Point before applying the selected privacy settings?",
        L"PrivatizeWin Safety Confirmation", MB_YESNOCANCEL | MB_ICONQUESTION);

    if (res == IDCANCEL) return;

    if (res == IDYES) {
        CreateSystemRestorePoint();
    }

    int applied = 0;
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const bool checked = (ListView_GetCheckState(m_hListView, i) != 0);
        const bool ok = TweakRegistry::Instance().ApplyTweak(m_displayedTweaks[i].id, checked, UserSelectionMode::AllUsers, {});
        if (ok) applied++;
    }

    RefreshAuditState();
    MessageBoxW(m_hWnd, L"Selected privacy settings have been applied successfully!", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::RevertAllToDefaults() {
    const int res = MessageBoxW(m_hWnd,
        L"Are you sure you want to revert all privacy tweaks back to Windows factory defaults?",
        L"Revert to Defaults", MB_YESNO | MB_ICONWARNING);

    if (res != IDYES) return;

    const auto tpl = TemplateManager::Instance().GetTemplate("defaults");
    if (tpl.has_value()) {
        TemplateManager::Instance().ApplyTemplate(tpl.value(), UserSelectionMode::AllUsers, {});
    }

    RefreshAuditState();
    MessageBoxW(m_hWnd, L"All settings have been reverted to Windows defaults.", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::RefreshAuditState() {
    PopulateListView(m_currentCategory, m_currentFilter, m_filterMode);
    UpdateStatusBar();
}

void MainWindow::CreateSystemRestorePoint() {
    int64_t seq = 0;
    const bool ok = RestorePoint::Create(L"PrivatizeWin - Pre-Apply Configuration", seq);
    if (ok) {
        MessageBoxW(m_hWnd, L"System Restore Point created successfully!", L"PrivatizeWin", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(m_hWnd, L"Could not create System Restore Point.\nSystem Protection might be disabled on your Windows OS drive.", L"Warning", MB_OK | MB_ICONWARNING);
    }
}

void MainWindow::ExportConfiguration() {
    wchar_t szFile[MAX_PATH] = L"privatizewin_config.json";
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"json";

    if (GetSaveFileNameW(&ofn)) {
        TemplateProfile p;
        p.name = "exported_config";
        p.description = "Exported PrivatizeWin configuration";
        for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
            p.tweakStates[m_displayedTweaks[i].id] = (ListView_GetCheckState(m_hListView, i) != 0);
        }
        if (TemplateManager::Instance().SaveTemplateToFile(szFile, p)) {
            MessageBoxW(m_hWnd, L"Configuration successfully exported to JSON.", L"Export Configuration", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Failed to write configuration file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::ImportConfiguration() {
    wchar_t szFile[MAX_PATH]{};
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        TemplateProfile p;
        if (TemplateManager::Instance().LoadTemplateFromFile(szFile, p)) {
            for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
                auto it = p.tweakStates.find(m_displayedTweaks[i].id);
                if (it != p.tweakStates.end()) {
                    ListView_SetCheckState(m_hListView, i, it->second);
                }
            }
            UpdateSelectAllCheckboxState();
            int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
            if (cur != -1) {
                auto* pDV = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(m_hDetailsView, GWLP_USERDATA));
                if (pDV) pDV->SetCheckedState(ListView_GetCheckState(m_hListView, cur) != 0);
            }
            MessageBoxW(m_hWnd, L"Configuration successfully imported. Review the checkboxes and click 'Apply Changes'.", L"Import Configuration", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Invalid or corrupted JSON template file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

} // namespace PrivatizeWin
