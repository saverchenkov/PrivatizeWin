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
#include "../core/SimpleJson.h"
#include "../core/Localization.h"
#include "../core/PendingHandoff.h"
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

// Subclass procedure for search box to support Esc to clear and Enter to focus list
static LRESULT CALLBACK SearchSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            SetWindowTextW(hWnd, L"");
            auto* pMain = reinterpret_cast<MainWindow*>(dwRefData);
            if (pMain) {
                HWND hList = FindWindowExW(GetParent(hWnd), nullptr, WC_LISTVIEWW, nullptr);
                if (hList) SetFocus(hList);
            }
            return 0;
        }
        if (wParam == VK_RETURN) {
            auto* pMain = reinterpret_cast<MainWindow*>(dwRefData);
            if (pMain) {
                HWND hList = FindWindowExW(GetParent(hWnd), nullptr, WC_LISTVIEWW, nullptr);
                if (hList) SetFocus(hList);
            }
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Subclass procedure for details edit to support accessible Tab navigation out of read-only edit
static LRESULT CALLBACK DetailsSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
    if (uMsg == WM_GETDLGCODE) {
        // Do not consume TAB or ESCAPE; let dialog manager navigate focus
        return DefSubclassProc(hWnd, uMsg, wParam, lParam) & ~(DLGC_WANTTAB | DLGC_WANTALLKEYS);
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
    if (!RegisterClassExW(&wc)) return false;

    WNDCLASSEXW sc{ sizeof(sc) };
    sc.lpfnWndProc = MainWindow::SplitterWndProc;
    sc.hInstance = hInstance;
    sc.lpszClassName = L"PrivatizeWin_Splitter";
    sc.hbrBackground = nullptr;
    sc.hCursor = LoadCursor(nullptr, IDC_SIZENS);
    sc.style = CS_HREDRAW | CS_VREDRAW;
    return (RegisterClassExW(&sc) != 0);
}

HWND MainWindow::Create(HINSTANCE hInstance, std::wstring_view resumePendingFile, std::wstring_view resumePendingToken) {
    s_resumePendingFile = std::wstring(resumePendingFile);
    s_resumePendingToken = std::wstring(resumePendingToken);
    return CreateWindowExW(
        WS_EX_WINDOWEDGE,
        L"PrivatizeWin_MainWindow",
        L"PrivatizeWin \u2014 Windows Privacy Settings",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 720,
        nullptr, nullptr, hInstance, nullptr
    );
}

static bool s_isSplitterHovered = false;

LRESULT CALLBACK MainWindow::SplitterWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

        UINT dpi = 96;
        if (hWnd) {
            dpi = GetDpiForWindow(hWnd);
            if (dpi == 0) dpi = 96;
        }

        const int midX = (rc.left + rc.right) / 2;
        const int midY = (rc.top + rc.bottom) / 2;

        // 1. Subtle horizontal divider line across the bar
        HPEN hPenLine = CreatePen(PS_SOLID, 1, RGB(214, 216, 220));
        HGDIOBJ hOldPen = SelectObject(hdc, hPenLine);
        MoveToEx(hdc, rc.left, midY, nullptr);
        LineTo(hdc, rc.right, midY);

        // 2. Centered draggable grip handle (rounded pill)
        const int handleW = MulDiv(46, dpi, 96);
        const int handleH = MulDiv(4, dpi, 96);
        const int halfH = handleH / 2;
        RECT rcHandle = {
            midX - handleW / 2,
            midY - halfH,
            midX + handleW / 2,
            midY + halfH + (handleH % 2 == 0 ? 0 : 1)
        };

        COLORREF fillColor = s_isSplitterHovered ? RGB(90, 95, 105) : RGB(165, 170, 178);
        HBRUSH hHandleBrush = CreateSolidBrush(fillColor);
        HPEN hHandlePen = CreatePen(PS_SOLID, 1, fillColor);

        SelectObject(hdc, hHandleBrush);
        SelectObject(hdc, hHandlePen);
        const int cornerR = MulDiv(4, dpi, 96);
        RoundRect(hdc, rcHandle.left, rcHandle.top, rcHandle.right, rcHandle.bottom, cornerR, cornerR);

        // 3. Inner grip ribs for instant "draggable" recognition:
        COLORREF ribColor = s_isSplitterHovered ? RGB(255, 255, 255) : RGB(236, 238, 242);
        HPEN hRibPen = CreatePen(PS_SOLID, 1, ribColor);
        SelectObject(hdc, hRibPen);

        const int ribSpacing = MulDiv(7, dpi, 96);
        const int ribHalfH = std::max(1, MulDiv(1, dpi, 96));
        for (int i = -1; i <= 1; ++i) {
            int ribX = midX + i * ribSpacing;
            MoveToEx(hdc, ribX, midY - ribHalfH, nullptr);
            LineTo(hdc, ribX, midY + ribHalfH + 1);
        }

        // Clean up GDI objects
        SelectObject(hdc, hOldPen);
        DeleteObject(hRibPen);
        DeleteObject(hHandlePen);
        DeleteObject(hHandleBrush);
        DeleteObject(hPenLine);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (!s_isSplitterHovered) {
            s_isSplitterHovered = true;
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    }
    case WM_MOUSELEAVE: {
        s_isSplitterHovered = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        return TRUE;
    case WM_LBUTTONDOWN: {
        HWND hParent = GetParent(hWnd);
        if (hParent) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(hWnd, &pt);
            ScreenToClient(hParent, &pt);
            SendMessage(hParent, WM_LBUTTONDOWN, wParam, MAKELPARAM(pt.x, pt.y));
        }
        return 0;
    }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
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
            pMMI->ptMinTrackSize.x = 1060;
            pMMI->ptMinTrackSize.y = 560;
            return 0;
        }
        case WM_COMMAND:
            pThis->OnCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_NOTIFY: {
            auto* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
            if (pnmhdr->code == NM_CUSTOMDRAW && pnmhdr->idFrom == IDC_LIST_TWEAKS) {
                return pThis->OnCustomDraw(pnmhdr);
            }
            pThis->OnNotify(pnmhdr);
            return 0;
        }
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
            if (pThis && pThis->m_isDraggingSplitter) {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                return TRUE;
            }
            HWND hTarget = reinterpret_cast<HWND>(wParam);
            if (hTarget == hWnd && pThis) {
                POINT pt{};
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                UINT dpi = 96;
                if (pThis->m_hWnd) {
                    dpi = GetDpiForWindow(pThis->m_hWnd);
                    if (dpi == 0) dpi = 96;
                }
                const int splitterH = MulDiv(8, dpi, 96);
                if (pt.y >= pThis->m_splitterY - 2 && pt.y <= pThis->m_splitterY + splitterH + 2) {
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
    if (m_hStateImageList) {
        ImageList_Destroy(m_hStateImageList);
        m_hStateImageList = nullptr;
    }
    if (m_hListView) {
        RemoveWindowSubclass(m_hListView, ListViewSubclassProc, 1);
    }
    if (m_hDetailsEdit) {
        RemoveWindowSubclass(m_hDetailsEdit, DetailsSubclassProc, 2);
    }
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
            UpdateHeaderTooltips();
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

std::wstring MainWindow::SavePendingStateToTempFile(std::wstring* outToken) const {
    PendingStatePlan plan;
    plan.pendingEnable.assign(m_pendingEnableIds.begin(), m_pendingEnableIds.end());
    plan.pendingRevert.assign(m_pendingRevertIds.begin(), m_pendingRevertIds.end());
    return PendingHandoff::SaveHandoff(plan, outToken);
}

bool MainWindow::RestorePendingStateFromFile(const std::wstring& filePath, const std::wstring& token) {
    PendingStatePlan plan;
    if (!PendingHandoff::ConsumeHandoff(filePath, plan, token)) {
        return false;
    }

    m_pendingEnableIds.clear();
    m_pendingRevertIds.clear();

    for (const auto& id : plan.pendingEnable) {
        if (TweakRegistry::Instance().GetTweakById(id) != nullptr) {
            m_pendingEnableIds.insert(id);
        }
    }
    for (const auto& id : plan.pendingRevert) {
        if (TweakRegistry::Instance().GetTweakById(id) != nullptr) {
            m_pendingRevertIds.insert(id);
        }
    }
    return true;
}

bool MainWindow::ValidateStagedPlan(std::string* outError) const {
    std::string err;
    const bool ok = TweakRegistry::Instance().ValidatePendingPlan(m_pendingEnableIds, m_pendingRevertIds, err);
    if (!ok && outError) {
        *outError = std::move(err);
    }
    return ok;
}

void MainWindow::StageTweakState(std::string_view id, bool enable) {
    std::string sId(id);
    if (enable) {
        m_pendingRevertIds.erase(sId);
        m_pendingEnableIds.insert(sId);
    } else {
        m_pendingEnableIds.erase(sId);
        m_pendingRevertIds.insert(sId);
    }
}

bool MainWindow::RelaunchAsAdminWithPendingState() {
    const bool hasPending = (!m_pendingEnableIds.empty() || !m_pendingRevertIds.empty());
    std::wstring pendingFile;
    std::wstring token;
    if (hasPending) {
        pendingFile = SavePendingStateToTempFile(&token);
        if (pendingFile.empty()) {
            // Abort relaunch if saving non-empty plan failed so user selections are preserved
            MessageBoxW(m_hWnd, L"Failed to securely save pending selections for elevated restart. Relaunch aborted to prevent losing your pending changes.", L"PrivatizeWin", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    std::wstring extraArgs;
    if (!pendingFile.empty()) {
        extraArgs = L"--resume-pending \"" + pendingFile + L"\" --resume-token \"" + token + L"\"";
    }

    if (RelaunchElevated(m_hWnd, extraArgs)) {
        PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
        return true;
    }

    // If elevation was cancelled or failed, clean up the handoff file and preserve selections
    if (!pendingFile.empty()) {
        DeleteFileW(pendingFile.c_str());
    }
    return false;
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

    const bool resumeAttempted = !s_resumePendingFile.empty();
    bool restoredPending = false;
    if (resumeAttempted) {
        restoredPending = RestorePendingStateFromFile(s_resumePendingFile, s_resumePendingToken);
        s_resumePendingFile.clear();
        s_resumePendingToken.clear();
    }

    UpdateLocalization();

    if (restoredPending) {
        const size_t total = m_pendingEnableIds.size() + m_pendingRevertIds.size();
        if (total > 0) {
            std::wstring stMsg = LocFmt("restored_pending", total);
            SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(stMsg.c_str()));
        }
    } else if (resumeAttempted) {
        MessageBoxW(m_hWnd,
            L"Could not restore pending selections from the previous elevation handoff.\nPlease re-select the desired privacy settings.",
            Loc("app_title").c_str(),
            MB_OK | MB_ICONWARNING);
    }
}

void MainWindow::InitializeControls() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 1. Search Box (Item 15: Filter tweaks Ctrl+F)
    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        10, 10, 180, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_SEARCH_EDIT), hInst, nullptr);
    SendMessage(m_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter tweaks (Ctrl+F)"));
    SetWindowSubclass(m_hSearchEdit, SearchSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

    // 2. Filter Dropdown (Item 15: Labeled by what it filters)
    m_hFilterCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        196, 10, 140, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_FILTER_COMBO), hInst, nullptr);
    SendMessage(m_hFilterCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hFilterCombo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 20);
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All settings"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Not applied only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Applied only"));
    SendMessage(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended only"));
    SendMessage(m_hFilterCombo, CB_SETCURSEL, 0, 0);

    // 3. Match / Hidden Count Label (Item 3)
    m_hLblMatchCount = CreateWindowW(WC_STATICW, L"260 shown",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        342, 13, 80, 20, m_hWnd, reinterpret_cast<HMENU>(IDC_LBL_MATCH_COUNT), hInst, nullptr);
    SendMessage(m_hLblMatchCount, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 4. Preset Dropdown & "Apply Preset" Button
    m_hTemplateCombo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        432, 10, 140, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_TPL_COMBO), hInst, nullptr);
    SendMessage(m_hTemplateCombo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hTemplateCombo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 20);
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strict Privacy"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal"));
    SendMessage(m_hTemplateCombo, CB_SETCURSEL, 0, 0);

    m_hBtnSelectPreset = CreateWindowW(WC_BUTTONW, L"Apply Preset",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        578, 10, 95, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SELECT_PRESET), hInst, nullptr);
    SendMessage(m_hBtnSelectPreset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 5. "Apply Defaults" Button (placed to the right of "Apply Preset")
    m_hBtnRevert = CreateWindowW(WC_BUTTONW, L"Apply Defaults",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        679, 10, 105, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_REVERT), hInst, nullptr);
    SendMessage(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    // 6. Action Button (Apply Selected - positioned dynamically on the right)
    m_hBtnApply = CreateWindowW(WC_BUTTONW, L"Apply Selected",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        890, 10, 145, 26, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_APPLY), hInst, nullptr);
    SendMessage(m_hBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

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
    SetWindowSubclass(m_hListView, ListViewSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

    // Multi-state Checkboxes (Item 10)
    CreateStateImages();
    ListView_SetImageList(m_hListView, m_hStateImageList, LVSIL_STATE);

    // Modern list view styles without LVS_EX_CHECKBOXES so custom multi-state icons are active
    ListView_SetExtendedListViewStyle(m_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

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

    InitializeHeaderTooltips();

    // 8. Adjustable Splitter Bar (Item 13)
    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }
    const int splitterH = MulDiv(8, dpi, 96);

    m_hSplitterBar = CreateWindowExW(
        0,
        L"PrivatizeWin_Splitter",
        L"",
        WS_CHILD | WS_VISIBLE,
        10, m_splitterY, 1040, splitterH,
        m_hWnd,
        reinterpret_cast<HMENU>(IDC_SPLITTER_BAR),
        hInst,
        nullptr
    );

    // 9. Inspector Details Pane (Native RichEdit 5.0)
    m_hDetailsEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, m_splitterY + splitterH, 1040, 200,
        m_hWnd,
        reinterpret_cast<HMENU>(IDC_EDIT_DETAILS),
        hInst,
        nullptr
    );
    SendMessage(m_hDetailsEdit, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    SendMessage(m_hDetailsEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(8, 8));

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

void MainWindow::CreateStateImages() {
    if (m_hStateImageList) {
        ImageList_Destroy(m_hStateImageList);
        m_hStateImageList = nullptr;
    }

    const int iconSize = 16;
    m_hStateImageList = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 5, 1);
    if (!m_hStateImageList) return;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    const COLORREF clrMask = RGB(255, 0, 255); // Magenta transparency mask
    const bool isDark = DarkMode::IsDarkModeActive();

    auto addStateBitmap = [&](auto drawFunc) {
        HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, iconSize, iconSize);
        HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));

        RECT rcFull{ 0, 0, iconSize, iconSize };
        HBRUSH hMaskBrush = CreateSolidBrush(clrMask);
        FillRect(hdcMem, &rcFull, hMaskBrush);
        DeleteObject(hMaskBrush);

        drawFunc(hdcMem, isDark);

        SelectObject(hdcMem, hOldBmp);
        ImageList_AddMasked(m_hStateImageList, hBmp, clrMask);
        DeleteObject(hBmp);
    };

    // State 1: Unchecked [ ] (Empty outline box, Windows default / not applied)
    addStateBitmap([](HDC hdc, bool dark) {
        COLORREF clrBorder = dark ? RGB(160, 160, 160) : RGB(120, 120, 120);
        COLORREF clrBg = dark ? RGB(32, 32, 32) : RGB(255, 255, 255);
        HPEN hPen = CreatePen(PS_SOLID, 1, clrBorder);
        HBRUSH hBrush = CreateSolidBrush(clrBg);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

        RoundRect(hdc, 1, 1, 15, 15, 3, 3);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    });

    // State 2: AlreadyEnabled [✔] (Solid green with white checkmark)
    addStateBitmap([](HDC hdc, bool /*dark*/) {
        COLORREF clrGreen = RGB(16, 137, 62);
        COLORREF clrBorder = RGB(12, 110, 50);
        HPEN hPen = CreatePen(PS_SOLID, 1, clrBorder);
        HBRUSH hBrush = CreateSolidBrush(clrGreen);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

        RoundRect(hdc, 1, 1, 15, 15, 3, 3);

        HPEN hCheckPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        SelectObject(hdc, hCheckPen);
        MoveToEx(hdc, 4, 8, nullptr);
        LineTo(hdc, 7, 11);
        LineTo(hdc, 12, 5);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hCheckPen);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    });

    // State 3: PendingEnable [☑] (Solid accent blue with white checkmark)
    addStateBitmap([](HDC hdc, bool /*dark*/) {
        COLORREF clrBlue = RGB(0, 120, 215);
        COLORREF clrBorder = RGB(0, 95, 175);
        HPEN hPen = CreatePen(PS_SOLID, 1, clrBorder);
        HBRUSH hBrush = CreateSolidBrush(clrBlue);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

        RoundRect(hdc, 1, 1, 15, 15, 3, 3);

        HPEN hCheckPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        SelectObject(hdc, hCheckPen);
        MoveToEx(hdc, 4, 8, nullptr);
        LineTo(hdc, 7, 11);
        LineTo(hdc, 12, 5);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hCheckPen);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    });

    // State 4: PendingRevert [-] (Solid red with white horizontal minus bar)
    addStateBitmap([](HDC hdc, bool /*dark*/) {
        COLORREF clrRed = RGB(209, 52, 56);
        COLORREF clrBorder = RGB(165, 35, 40);
        HPEN hPen = CreatePen(PS_SOLID, 1, clrBorder);
        HBRUSH hBrush = CreateSolidBrush(clrRed);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

        RoundRect(hdc, 1, 1, 15, 15, 3, 3);

        RECT rcMinus{ 4, 7, 12, 9 };
        HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rcMinus, hWhiteBrush);
        DeleteObject(hWhiteBrush);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    });

    // State 5: Disabled (Faint grey outline box, cannot be checked/toggled)
    addStateBitmap([](HDC hdc, bool dark) {
        COLORREF clrBorder = dark ? RGB(75, 75, 75) : RGB(190, 190, 190);
        COLORREF clrBg = dark ? RGB(40, 40, 40) : RGB(235, 235, 235);
        HPEN hPen = CreatePen(PS_SOLID, 1, clrBorder);
        HBRUSH hBrush = CreateSolidBrush(clrBg);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

        RoundRect(hdc, 1, 1, 15, 15, 3, 3);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    });

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

void MainWindow::SetRowCheckboxState(int itemIndex, CheckboxState state) {
    if (!m_hListView || itemIndex < 0) return;
    LVITEMW lvi{};
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.state = INDEXTOSTATEIMAGEMASK(static_cast<UINT>(state));
    SendMessageW(m_hListView, LVM_SETITEMSTATE, static_cast<WPARAM>(itemIndex), reinterpret_cast<LPARAM>(&lvi));
}

CheckboxState MainWindow::GetRowCheckboxState(int itemIndex) const {
    if (!m_hListView || itemIndex < 0) return CheckboxState::None;
    const UINT state = ListView_GetItemState(m_hListView, itemIndex, LVIS_STATEIMAGEMASK);
    const UINT idx = (state & LVIS_STATEIMAGEMASK) >> 12;
    return static_cast<CheckboxState>(idx);
}

void MainWindow::ToggleRowCheckbox(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_displayedTweaks.size())) return;
    const auto& t = m_displayedTweaks[itemIndex];
    if (m_notApplicableIds.count(t.id) > 0) return;
    const SettingStatus auditSt = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
    if (auditSt == SettingStatus::NotApplicable) return;
    const CheckboxState cur = GetRowCheckboxState(itemIndex);
    if (cur == CheckboxState::Disabled) return;

    CheckboxState nextState = CheckboxState::Unchecked;

    if (cur == CheckboxState::Unchecked) {
        m_pendingEnableIds.insert(t.id);
        m_pendingRevertIds.erase(t.id);
        nextState = CheckboxState::PendingEnable;
    } else if (cur == CheckboxState::PendingEnable) {
        m_pendingEnableIds.erase(t.id);
        m_pendingRevertIds.erase(t.id);
        nextState = (auditSt == SettingStatus::Applied) ? CheckboxState::AlreadyEnabled : CheckboxState::Unchecked;
    } else if (cur == CheckboxState::AlreadyEnabled) {
        m_pendingRevertIds.insert(t.id);
        m_pendingEnableIds.erase(t.id);
        nextState = CheckboxState::PendingRevert;
    } else if (cur == CheckboxState::PendingRevert) {
        m_pendingRevertIds.erase(t.id);
        m_pendingEnableIds.erase(t.id);
        nextState = CheckboxState::AlreadyEnabled;
    } else {
        m_pendingEnableIds.insert(t.id);
        nextState = CheckboxState::PendingEnable;
    }

    SetRowCheckboxState(itemIndex, nextState);
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::OnSize(int width, int height) {
    if (width <= 0 || height <= 0) return;

    SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

    // Adjust parts based on width
    int part0 = std::max(360, width - 420);
    int part1 = part0 + 160;
    int parts[3] = { part0, part1, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));

    // Toolbar layout
    UpdateToolbarLayout(width);

    UpdateSplitterLayout();

    // Adjust list column 0 to fill available width
    if (m_hListView) {
        const int fixedCols = ListView_GetColumnWidth(m_hListView, 1) +
                              ListView_GetColumnWidth(m_hListView, 2) +
                              ListView_GetColumnWidth(m_hListView, 3);
        const int col0Width = std::max(280, width - 20 - fixedCols - GetSystemMetrics(SM_CXVSCROLL) - 4);
        ListView_SetColumnWidth(m_hListView, 0, col0Width);
        UpdateHeaderTooltips();
    }

    InvalidateRect(m_hWnd, nullptr, FALSE);

}

void MainWindow::UpdateSplitterLayout() {
    if (!m_hListView || !m_hDetailsEdit) return;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    RECT rcStatus{};
    if (m_hStatusBar) {
        GetWindowRect(m_hStatusBar, &rcStatus);
    }
    const int statusH = rcStatus.bottom - rcStatus.top;

    const int topH = 34;
    const int minListH = 180;
    const int minDetailsH = 80;

    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }
    const int splitterH = MulDiv(8, dpi, 96);

    if (m_splitterY < minListH + topH) {
        m_splitterY = minListH + topH;
    } else if (m_splitterY > height - statusH - minDetailsH - splitterH - 6) {
        m_splitterY = height - statusH - minDetailsH - splitterH - 6;
    }

    const int listTop = topH + 12;
    const int listH = std::max(minListH, m_splitterY - listTop);

    const int editTop = m_splitterY + splitterH;
    const int editH = std::max(minDetailsH, height - statusH - editTop - 6);

    HDWP hdwp = BeginDeferWindowPos(3);
    if (hdwp) {
        hdwp = DeferWindowPos(hdwp, m_hListView, nullptr, 10, listTop, width - 20, listH,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        if (m_hSplitterBar) {
            hdwp = DeferWindowPos(hdwp, m_hSplitterBar, nullptr, 10, m_splitterY, width - 20, splitterH,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        }
        hdwp = DeferWindowPos(hdwp, m_hDetailsEdit, nullptr, 10, editTop, width - 20, editH,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        if (hdwp) {
            EndDeferWindowPos(hdwp);
        }
    }

    // Completely erase and repaint any exposed parent window band around the splitter
    RECT rcBand = { 0, m_splitterY - 4, width, editTop + 4 };
    RedrawWindow(m_hWnd, &rcBand, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    if (m_hSplitterBar) {
        RedrawWindow(m_hSplitterBar, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void MainWindow::OnLButtonDown(int /*x*/, int y) {
    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }
    const int splitterH = MulDiv(8, dpi, 96);
    if (y >= m_splitterY - 3 && y <= m_splitterY + splitterH + 3) {
        m_isDraggingSplitter = true;
        SetCapture(m_hWnd);
    }
}

void MainWindow::OnLButtonUp() {
    if (m_isDraggingSplitter) {
        m_isDraggingSplitter = false;
        s_isSplitterHovered = false;
        if (m_hSplitterBar) {
            InvalidateRect(m_hSplitterBar, nullptr, FALSE);
        }
        ReleaseCapture();
        SavePreferences();

        RECT rc;
        GetClientRect(m_hWnd, &rc);
        OnSize(rc.right - rc.left, rc.bottom - rc.top);
    }
}

void MainWindow::OnMouseMove(int /*x*/, int y) {
    if (m_isDraggingSplitter) {
        RECT rc;
        GetClientRect(m_hWnd, &rc);
        RECT rcStatus{};
        if (m_hStatusBar) {
            GetWindowRect(m_hStatusBar, &rcStatus);
        }
        const int statusH = rcStatus.bottom - rcStatus.top;
        const int minTop = 220;
        const int maxBottom = rc.bottom - statusH - 90;
        if (y >= minTop && y <= maxBottom && y != m_splitterY) {
            m_splitterY = y;
            UpdateSplitterLayout();
        }
    }
}

void MainWindow::InitializeHeaderTooltips() {
    if (!m_hListView) return;
    HWND hHeader = ListView_GetHeader(m_hListView);
    if (!hHeader) return;

    if (!m_hHeaderTooltip) {
        m_hHeaderTooltip = CreateWindowExW(
            WS_EX_TOPMOST,
            TOOLTIPS_CLASSW,
            nullptr,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            m_hWnd,
            nullptr,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_hWnd, GWLP_HINSTANCE)),
            nullptr
        );
        if (!m_hHeaderTooltip) return;

        SendMessageW(m_hHeaderTooltip, TTM_SETMAXTIPWIDTH, 0, 380);
        SendMessageW(m_hHeaderTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 250);
        SendMessageW(m_hHeaderTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000);
        SendMessageW(m_hHeaderTooltip, TTM_SETDELAYTIME, TTDT_RESHOW, 100);

        if (DarkMode::IsDarkModeActive()) {
            DarkMode::ApplyToControl(m_hHeaderTooltip);
        }
    }

    // Column 0: Setting
    TOOLINFOW ti0{};
    ti0.cbSize = sizeof(ti0);
    ti0.uFlags = TTF_SUBCLASS;
    ti0.hwnd = hHeader;
    ti0.uId = 0;
    Header_GetItemRect(hHeader, 0, &ti0.rect);
    ti0.lpszText = const_cast<LPWSTR>(
        L"Setting:\r\n"
        L"Name and summary of the Windows privacy, telemetry, or security feature."
    );
    SendMessageW(m_hHeaderTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti0));

    // Column 1: Status
    TOOLINFOW ti1{};
    ti1.cbSize = sizeof(ti1);
    ti1.uFlags = TTF_SUBCLASS;
    ti1.hwnd = hHeader;
    ti1.uId = 1;
    Header_GetItemRect(hHeader, 1, &ti1.rect);
    ti1.lpszText = const_cast<LPWSTR>(
        L"Status & Multi-State Checkbox:\r\n"
        L"Reflects machine audit status and pending staging actions.\r\n\r\n"
        L"System State (Status Column):\r\n"
        L"  \u25CF Applied: Setting is 100% active and enforced on your machine.\r\n"
        L"  \u25CB Not applied: Setting is in Windows standard default state.\r\n"
        L"  ? Unknown: Key inaccessible or detection failed.\r\n\r\n"
        L"Checkbox State:\r\n"
        L"  \u2022 [  ] Unchecked: Setting is default / not applied.\r\n"
        L"  \u2022 [\u2714] Green Check: Setting is already applied in Windows.\r\n"
        L"  \u2022 [\u2611] Blue Check: Staged to be applied on next action.\r\n"
        L"  \u2022 [ - ] Red Minus: Staged to be restored to Windows default."
    );
    SendMessageW(m_hHeaderTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti1));

    // Column 2: Impact
    TOOLINFOW ti2{};
    ti2.cbSize = sizeof(ti2);
    ti2.uFlags = TTF_SUBCLASS;
    ti2.hwnd = hHeader;
    ti2.uId = 2;
    Header_GetItemRect(hHeader, 2, &ti2.rect);
    ti2.lpszText = const_cast<LPWSTR>(
        L"Impact:\r\n"
        L"Functional consequence of applying this tweak:\r\n\r\n"
        L"\u2022 Low: Safe optimization with minimal or no functional side effects.\r\n"
        L"\u2022 Moderate: Mild tradeoff (e.g. disables Bing search in Start, web suggestions, or feedback prompts).\r\n"
        L"\u2022 High: Functional restriction on hardware or convenience (e.g. restricts camera, microphone, or biometric login)."
    );
    SendMessageW(m_hHeaderTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti2));

    // Column 3: Scope
    TOOLINFOW ti3{};
    ti3.cbSize = sizeof(ti3);
    ti3.uFlags = TTF_SUBCLASS;
    ti3.hwnd = hHeader;
    ti3.uId = 3;
    Header_GetItemRect(hHeader, 3, &ti3.rect);
    ti3.lpszText = const_cast<LPWSTR>(
        L"Scope:\r\n"
        L"Execution boundary and permissions required for this tweak:\r\n\r\n"
        L"\u2022 User: Applies to current user profile (HKCU). Standard user, no admin elevation required.\r\n"
        L"\u2022 Machine: Applies system-wide to all users (HKLM / policies). Requires administrator privileges.\r\n"
        L"\u2022 Machine (Service): Governs Windows system services. Requires administrator privileges."
    );
    SendMessageW(m_hHeaderTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti3));
}

void MainWindow::UpdateHeaderTooltips() {
    if (!m_hListView || !m_hHeaderTooltip) return;
    HWND hHeader = ListView_GetHeader(m_hListView);
    if (!hHeader) return;

    for (int col = 0; col < 4; ++col) {
        TOOLINFOW ti{};
        ti.cbSize = sizeof(ti);
        ti.hwnd = hHeader;
        ti.uId = static_cast<UINT_PTR>(col);
        if (Header_GetItemRect(hHeader, col, &ti.rect)) {
            SendMessageW(m_hHeaderTooltip, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&ti));
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
        std::wstring locCat = Localization::Instance().GetCategory(cat.name);
        group.pszHeader = const_cast<LPWSTR>(locCat.c_str());

        ListView_InsertGroup(m_hListView, -1, &group);
        categoryToGroupId[cat.name] = groupId;
        groupId++;
    }

    // 2. Populate Tweaks into their Groups
    m_displayedTweaks.clear();
    m_notApplicableIds.clear();
    m_appliedCount = 0;

    std::wstring lowerSearch(searchFilter);
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::towlower);

    int itemIndex = 0;
    for (const auto& t : catalog) {
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        if (st == SettingStatus::NotApplicable) {
            m_notApplicableIds.insert(t.id);
            m_pendingEnableIds.erase(t.id);
            m_pendingRevertIds.erase(t.id);
        }
        const bool isApplied = (st == SettingStatus::Applied);
        if (isApplied) {
            m_appliedCount++;
        }

        std::wstring locTitle = Localization::Instance().GetTweakTitle(t.id, t.title);

        // Filter logic
        if (!lowerSearch.empty()) {
            std::wstring lowerTitle = t.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::towlower);
            std::wstring lowerLocTitle = locTitle;
            std::transform(lowerLocTitle.begin(), lowerLocTitle.end(), lowerLocTitle.begin(), ::towlower);
            std::wstring lowerDesc = t.description;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::towlower);
            std::wstring wideId(t.id.begin(), t.id.end());
            std::transform(wideId.begin(), wideId.end(), wideId.begin(), ::towlower);
            if (lowerTitle.find(lowerSearch) == std::wstring::npos &&
                lowerLocTitle.find(lowerSearch) == std::wstring::npos &&
                lowerDesc.find(lowerSearch) == std::wstring::npos &&
                wideId.find(lowerSearch) == std::wstring::npos) {
                continue;
            }
        }

        if (filterMode == FilterMode::RecommendedOnly && !t.isRecommended) continue;
        if (filterMode == FilterMode::NotAppliedOnly && isApplied) continue;
        if (filterMode == FilterMode::AppliedOnly && !isApplied) continue;

        m_displayedTweaks.push_back(t);

        LVITEMW lvi{};
        lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM;
        lvi.iItem = itemIndex;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(locTitle.c_str());
        lvi.lParam = itemIndex;

        auto it = categoryToGroupId.find(t.category);
        if (it != categoryToGroupId.end()) {
            lvi.iGroupId = it->second;
        }

        ListView_InsertItem(m_hListView, &lvi);

        // Status text (Item 4: Applied, Not applied, Unknown, Not applicable)
        std::wstring stStr;
        switch (st) {
        case SettingStatus::Applied:       stStr = L"\u25CF " + Loc("status_raw_applied"); break;
        case SettingStatus::NotApplied:    stStr = L"\u25CB " + Loc("status_raw_not_applied"); break;
        case SettingStatus::Partial:       stStr = L"\u25D0 " + Loc("status_raw_partial"); break;
        case SettingStatus::Custom:        stStr = L"\u25C6 " + Loc("status_raw_custom"); break;
        case SettingStatus::Unknown:       stStr = L"? " + Loc("status_raw_unknown"); break;
        case SettingStatus::NotApplicable: stStr = L"\u2014 " + Loc("status_raw_not_applicable"); break;
        default:                           stStr = L"\u25CB " + Loc("status_raw_not_applied"); break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 1, const_cast<LPWSTR>(stStr.c_str()));

        // Impact text (Item 5: Low, Moderate, High)
        std::wstring impactStr;
        switch (t.impactLevel) {
        case ImpactLevel::Low:      impactStr = Loc("impact_low"); break;
        case ImpactLevel::Moderate: impactStr = Loc("impact_moderate"); break;
        case ImpactLevel::High:     impactStr = Loc("impact_high"); break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 2, const_cast<LPWSTR>(impactStr.c_str()));

        // Scope text (Item 12: User, Machine, or Service)
        std::wstring scopeStr;
        switch (t.scope) {
        case TargetScope::Machine: scopeStr = Loc("scope_machine"); break;
        case TargetScope::User:    scopeStr = Loc("scope_user"); break;
        case TargetScope::Service: scopeStr = Loc("scope_service"); break;
        }
        ListView_SetItemText(m_hListView, itemIndex, 3, const_cast<LPWSTR>(scopeStr.c_str()));

        // Multi-state Checkbox (Item 10)
        CheckboxState cbState = CheckboxState::Unchecked;
        if (st == SettingStatus::NotApplicable) {
            cbState = CheckboxState::Disabled;
        } else if (m_pendingEnableIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingEnable;
        } else if (m_pendingRevertIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingRevert;
        } else if (st == SettingStatus::Applied) {
            cbState = CheckboxState::AlreadyEnabled;
        }
        SetRowCheckboxState(itemIndex, cbState);

        itemIndex++;
    }

    SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hListView, nullptr, TRUE);

    // Sync Details Pane reliably (Item 9)
    if (!m_displayedTweaks.empty()) {
        ListView_SetItemState(m_hListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        UpdateDetailsPane(0);
    } else {
        SetWindowTextW(m_hDetailsEdit, Loc("no_match_filter").c_str());
    }

    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::UpdateSelectionCounts() {
    const size_t pendingApply = m_pendingEnableIds.size();
    const size_t pendingRevert = m_pendingRevertIds.size();
    const size_t totalSelected = pendingApply + pendingRevert;

    size_t shownSelected = 0;
    for (const auto& t : m_displayedTweaks) {
        if (m_pendingEnableIds.count(t.id) > 0 || m_pendingRevertIds.count(t.id) > 0) {
            shownSelected++;
        }
    }
    const size_t hiddenSelected = (totalSelected >= shownSelected) ? (totalSelected - shownSelected) : 0;

    // Update main action button (Item 3)
    if (m_hBtnApply) {
        if (totalSelected > 0) {
            std::wstring btnText;
            if (pendingApply > 0 && pendingRevert > 0) {
                btnText = LocFmt("btn_apply_changes_n", totalSelected);
            } else if (pendingRevert > 0) {
                btnText = LocFmt("btn_restore_selected_n", pendingRevert);
            } else {
                btnText = LocFmt("btn_apply_selected_n", pendingApply);
            }
            SetWindowTextW(m_hBtnApply, btnText.c_str());
            EnableWindow(m_hBtnApply, TRUE);
        } else {
            SetWindowTextW(m_hBtnApply, Loc("btn_apply_selected").c_str());
            EnableWindow(m_hBtnApply, FALSE);
        }
    }

    // Update compact count label next to filter/search (Item 3)
    if (m_hLblMatchCount) {
        std::wstringstream ss;
        ss << LocFmt("match_shown", m_displayedTweaks.size());
        if (totalSelected > 0) {
            ss << L" \u00B7 " << LocFmt("match_selected", totalSelected);
            if (hiddenSelected > 0) {
                ss << L" " << LocFmt("match_hidden", hiddenSelected);
            }
        }
        SetWindowTextW(m_hLblMatchCount, ss.str().c_str());
    }

    UpdateToolbarLayout();
}

int MainWindow::GetApplyButtonWidth() const {
    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }
    const int baseMinW = MulDiv(145, dpi, 96);
    const int padH = MulDiv(36, dpi, 96);

    int w = baseMinW;
    if (m_hBtnApply) {
        wchar_t textBuf[128]{};
        GetWindowTextW(m_hBtnApply, textBuf, 128);
        if (textBuf[0] != L'\0') {
            HDC hdc = GetDC(m_hBtnApply);
            if (hdc) {
                HGDIOBJ hOld = SelectObject(hdc, m_hFontBold ? m_hFontBold : m_hFontRegular);
                SIZE sz{};
                GetTextExtentPoint32W(hdc, textBuf, static_cast<int>(wcslen(textBuf)), &sz);
                SelectObject(hdc, hOld);
                ReleaseDC(m_hBtnApply, hdc);
                w = std::max(baseMinW, static_cast<int>(sz.cx) + padH);
            }
        }
    }
    return w;
}

void MainWindow::UpdateActionButtonsLayout(int clientWidth) {
    if (!m_hBtnApply || !m_hWnd) return;
    if (clientWidth <= 0) {
        RECT rcClient{};
        GetClientRect(m_hWnd, &rcClient);
        clientWidth = rcClient.right - rcClient.left;
    }
    if (clientWidth <= 0) return;

    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }

    const int topMargin = MulDiv(10, dpi, 96);
    const int ctrlH = MulDiv(26, dpi, 96);
    const int gap = MulDiv(6, dpi, 96);
    const int rightEdge = clientWidth - 10;
    const int btnApplyW = GetApplyButtonWidth();

    int applyX = rightEdge - btnApplyW;
    if (m_hBtnRevert) {
        RECT rcDefaults{};
        GetWindowRect(m_hBtnRevert, &rcDefaults);
        POINT pt{ rcDefaults.right, rcDefaults.top };
        ScreenToClient(m_hWnd, &pt);
        if (applyX < pt.x + gap) {
            applyX = pt.x + gap;
        }
    }

    SetWindowPos(m_hBtnApply, nullptr, applyX, topMargin, btnApplyW, ctrlH, SWP_NOZORDER | SWP_NOACTIVATE);

    // Repaint toolbar background area around apply button to cleanly erase any vacated background
    RECT rcToolbar{ 0, topMargin - 2, clientWidth, topMargin + ctrlH + 4 };
    InvalidateRect(m_hWnd, &rcToolbar, TRUE);
    UpdateWindow(m_hBtnApply);
}

static int MeasureWindowTextWidth(HWND hCtrl, HFONT hFont, int padding = 20) {
    if (!hCtrl) return 60;
    wchar_t buf[256]{};
    GetWindowTextW(hCtrl, buf, 256);
    if (buf[0] == L'\0') return 60;
    HDC hdc = GetDC(hCtrl);
    if (!hdc) return 60;
    HGDIOBJ hOld = SelectObject(hdc, hFont ? hFont : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    SIZE sz{};
    GetTextExtentPoint32W(hdc, buf, static_cast<int>(wcslen(buf)), &sz);
    SelectObject(hdc, hOld);
    ReleaseDC(hCtrl, hdc);
    return static_cast<int>(sz.cx) + padding;
}

static int MeasureComboMaxTextWidth(HWND hCombo, HFONT hFont) {
    if (!hCombo) return 120;
    int count = static_cast<int>(SendMessageW(hCombo, CB_GETCOUNT, 0, 0));
    if (count <= 0) return 120;
    HDC hdc = GetDC(hCombo);
    if (!hdc) return 120;
    HGDIOBJ hOld = SelectObject(hdc, hFont ? hFont : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    int maxW = 0;
    for (int i = 0; i < count; ++i) {
        int len = static_cast<int>(SendMessageW(hCombo, CB_GETLBTEXTLEN, i, 0));
        if (len > 0 && len < 256) {
            std::wstring s(len, L'\0');
            SendMessageW(hCombo, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(s.data()));
            SIZE sz{};
            GetTextExtentPoint32W(hdc, s.c_str(), len, &sz);
            if (static_cast<int>(sz.cx) > maxW) maxW = static_cast<int>(sz.cx);
        }
    }
    SelectObject(hdc, hOld);
    ReleaseDC(hCombo, hdc);
    return maxW;
}

void MainWindow::UpdateToolbarLayout(int clientWidth) {
    if (!m_hWnd) return;
    if (clientWidth <= 0) {
        RECT rcClient{};
        GetClientRect(m_hWnd, &rcClient);
        clientWidth = rcClient.right - rcClient.left;
    }
    if (clientWidth <= 0) return;

    UINT dpi = 96;
    if (m_hWnd) {
        dpi = GetDpiForWindow(m_hWnd);
        if (dpi == 0) dpi = 96;
    }

    const int topMargin = MulDiv(10, dpi, 96);
    const int ctrlH = MulDiv(26, dpi, 96);
    const int gap = MulDiv(6, dpi, 96);

    // Measure combo boxes content width
    int filterContentW = MeasureComboMaxTextWidth(m_hFilterCombo, m_hFontRegular);
    int wFilter = std::max(MulDiv(140, dpi, 96), filterContentW + MulDiv(32, dpi, 96));
    SendMessageW(m_hFilterCombo, CB_SETDROPPEDWIDTH, std::max(wFilter, filterContentW + MulDiv(28, dpi, 96)), 0);

    int tplContentW = MeasureComboMaxTextWidth(m_hTemplateCombo, m_hFontRegular);
    int wPresetCombo = std::max(MulDiv(130, dpi, 96), tplContentW + MulDiv(32, dpi, 96));
    SendMessageW(m_hTemplateCombo, CB_SETDROPPEDWIDTH, std::max(wPresetCombo, tplContentW + MulDiv(28, dpi, 96)), 0);

    // Measure buttons
    int wPresetBtn = std::max(MulDiv(95, dpi, 96), MeasureWindowTextWidth(m_hBtnSelectPreset, m_hFontRegular, MulDiv(22, dpi, 96)));
    int wDefaultsBtn = std::max(MulDiv(105, dpi, 96), MeasureWindowTextWidth(m_hBtnRevert, m_hFontRegular, MulDiv(22, dpi, 96)));

    // Match count label
    int wCount = std::max(MulDiv(75, dpi, 96), MeasureWindowTextWidth(m_hLblMatchCount, m_hFontRegular, MulDiv(10, dpi, 96)));

    // Search box: base 170
    int wSearch = MulDiv(170, dpi, 96);

    const int applyBtnW = GetApplyButtonWidth();
    const int rightAvail = clientWidth - 10 - applyBtnW - gap;
    int totalLeftW = 10 + wSearch + gap + wFilter + gap + wCount + MulDiv(10, dpi, 96) + wPresetCombo + gap + wPresetBtn + gap + wDefaultsBtn;

    int excess = totalLeftW - rightAvail;
    if (excess > 0) {
        int shrinkSearch = std::min(wSearch - MulDiv(90, dpi, 96), excess);
        wSearch -= shrinkSearch;
        excess -= shrinkSearch;
    }
    if (excess > 0) {
        int shrinkFilter = std::min(wFilter - MulDiv(130, dpi, 96), excess);
        wFilter -= shrinkFilter;
        excess -= shrinkFilter;
    }
    if (excess > 0) {
        int shrinkPreset = std::min(wPresetCombo - MulDiv(120, dpi, 96), excess);
        wPresetCombo -= shrinkPreset;
        excess -= shrinkPreset;
    }

    int x = 10;
    SetWindowPos(m_hSearchEdit, nullptr, x, topMargin, wSearch, ctrlH, SWP_NOZORDER | SWP_NOACTIVATE);
    x += wSearch + gap;

    SetWindowPos(m_hFilterCombo, nullptr, x, topMargin, wFilter, MulDiv(200, dpi, 96), SWP_NOZORDER | SWP_NOACTIVATE);
    x += wFilter + gap;

    SetWindowPos(m_hLblMatchCount, nullptr, x, topMargin + MulDiv(3, dpi, 96), wCount, MulDiv(20, dpi, 96), SWP_NOZORDER | SWP_NOACTIVATE);
    x += wCount + MulDiv(10, dpi, 96);

    SetWindowPos(m_hTemplateCombo, nullptr, x, topMargin, wPresetCombo, MulDiv(200, dpi, 96), SWP_NOZORDER | SWP_NOACTIVATE);
    x += wPresetCombo + gap;

    SetWindowPos(m_hBtnSelectPreset, nullptr, x, topMargin, wPresetBtn, ctrlH, SWP_NOZORDER | SWP_NOACTIVATE);
    x += wPresetBtn + gap;

    SetWindowPos(m_hBtnRevert, nullptr, x, topMargin, wDefaultsBtn, ctrlH, SWP_NOZORDER | SWP_NOACTIVATE);

    UpdateActionButtonsLayout(clientWidth);
}

void MainWindow::UpdateMenus() {
    HMENU hMenu = GetMenu(m_hWnd);
    if (!hMenu) return;

    // Top-level popups (by position)
    ModifyMenuW(hMenu, 0, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 0)), Loc("menu_file").c_str());
    ModifyMenuW(hMenu, 1, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 1)), Loc("menu_presets").c_str());
    ModifyMenuW(hMenu, 2, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 2)), Loc("menu_actions").c_str());
    ModifyMenuW(hMenu, 3, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 3)), Loc("menu_language").c_str());
    ModifyMenuW(hMenu, 4, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 4)), Loc("menu_tools").c_str());
    ModifyMenuW(hMenu, 5, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 5)), Loc("menu_help").c_str());

    // File menu
    ModifyMenuW(hMenu, IDM_FILE_EXPORT, MF_BYCOMMAND | MF_STRING, IDM_FILE_EXPORT, Loc("menu_export").c_str());
    ModifyMenuW(hMenu, IDM_FILE_IMPORT, MF_BYCOMMAND | MF_STRING, IDM_FILE_IMPORT, Loc("menu_import").c_str());
    ModifyMenuW(hMenu, IDM_FILE_RESTART_ADMIN, MF_BYCOMMAND | MF_STRING, IDM_FILE_RESTART_ADMIN, Loc("menu_restart_admin").c_str());
    ModifyMenuW(hMenu, IDM_FILE_EXIT, MF_BYCOMMAND | MF_STRING, IDM_FILE_EXIT, Loc("menu_exit").c_str());

    // Presets menu
    ModifyMenuW(hMenu, IDM_TPL_RECOMMENDED, MF_BYCOMMAND | MF_STRING, IDM_TPL_RECOMMENDED, Loc("menu_tpl_recommended").c_str());
    ModifyMenuW(hMenu, IDM_TPL_STRICT, MF_BYCOMMAND | MF_STRING, IDM_TPL_STRICT, Loc("menu_tpl_strict").c_str());
    ModifyMenuW(hMenu, IDM_TPL_MINIMAL, MF_BYCOMMAND | MF_STRING, IDM_TPL_MINIMAL, Loc("menu_tpl_minimal").c_str());

    // Actions menu
    ModifyMenuW(hMenu, IDM_ACT_APPLY, MF_BYCOMMAND | MF_STRING, IDM_ACT_APPLY, Loc("menu_apply").c_str());
    ModifyMenuW(hMenu, IDM_ACT_RESTORE_SELECTED, MF_BYCOMMAND | MF_STRING, IDM_ACT_RESTORE_SELECTED, Loc("menu_restore_selected").c_str());
    ModifyMenuW(hMenu, IDM_ACT_RESTORE_ALL, MF_BYCOMMAND | MF_STRING, IDM_ACT_RESTORE_ALL, Loc("menu_restore_all").c_str());
    ModifyMenuW(hMenu, IDM_SEL_RECOMMENDED, MF_BYCOMMAND | MF_STRING, IDM_SEL_RECOMMENDED, Loc("menu_sel_recommended").c_str());
    ModifyMenuW(hMenu, IDM_SEL_ALL_SHOWN, MF_BYCOMMAND | MF_STRING, IDM_SEL_ALL_SHOWN, Loc("menu_sel_all").c_str());
    ModifyMenuW(hMenu, IDM_SEL_INVERT_SHOWN, MF_BYCOMMAND | MF_STRING, IDM_SEL_INVERT_SHOWN, Loc("menu_sel_invert").c_str());
    ModifyMenuW(hMenu, IDM_SEL_CLEAR, MF_BYCOMMAND | MF_STRING, IDM_SEL_CLEAR, Loc("menu_sel_clear").c_str());
    ModifyMenuW(hMenu, IDM_ACT_REFRESH, MF_BYCOMMAND | MF_STRING, IDM_ACT_REFRESH, Loc("menu_refresh").c_str());
    ModifyMenuW(hMenu, IDM_ACT_RESTORE_PT, MF_BYCOMMAND | MF_STRING, IDM_ACT_RESTORE_PT, Loc("menu_restore_pt").c_str());

    // Language submenu items
    HMENU hLangSub = GetSubMenu(hMenu, 3);
    if (hLangSub) {
        const auto& langs = Localization::Instance().GetSupportedLanguages();
        for (const auto& l : langs) {
            std::wstring label = L"&" + l.nativeName;
            if (l.lang != Language::English) {
                label += L" (" + l.englishName + L")";
            }
            ModifyMenuW(hLangSub, l.menuId, MF_BYCOMMAND | MF_STRING, l.menuId, label.c_str());
        }
        int curLangIdx = static_cast<int>(Localization::Instance().GetCurrentLanguage());
        CheckMenuRadioItem(hLangSub, IDM_LANG_BASE, IDM_LANG_BASE + 11, IDM_LANG_BASE + curLangIdx, MF_BYCOMMAND);
    }

    // Tools menu
    ModifyMenuW(hMenu, IDM_TOOLS_SCHEDULE, MF_BYCOMMAND | MF_STRING, IDM_TOOLS_SCHEDULE, Loc("menu_schedule").c_str());
    ModifyMenuW(hMenu, IDM_TOOLS_TASKSCHD, MF_BYCOMMAND | MF_STRING, IDM_TOOLS_TASKSCHD, Loc("menu_taskschd").c_str());

    // Help menu
    ModifyMenuW(hMenu, IDM_HELP_ABOUT, MF_BYCOMMAND | MF_STRING, IDM_HELP_ABOUT, Loc("menu_about").c_str());
    ModifyMenuW(hMenu, IDM_HELP_GITHUB, MF_BYCOMMAND | MF_STRING, IDM_HELP_GITHUB, Loc("menu_github").c_str());

    DrawMenuBar(m_hWnd);
}

void MainWindow::UpdateLocalization() {
    // Window title
    std::wstring title = Loc("app_title");
    if (IsRunningAsAdmin()) {
        title += Loc("admin_suffix");
    } else {
        title += Loc("user_suffix");
    }
    SetWindowTextW(m_hWnd, title.c_str());

    // Menus
    UpdateMenus();

    // Search cue banner
    if (m_hSearchEdit) {
        SendMessageW(m_hSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(Loc("search_cue").c_str()));
    }

    // Filter combo
    if (m_hFilterCombo) {
        int curSel = static_cast<int>(SendMessageW(m_hFilterCombo, CB_GETCURSEL, 0, 0));
        if (curSel < 0) curSel = 0;
        SendMessageW(m_hFilterCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("filter_all").c_str()));
        SendMessageW(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("filter_not_applied").c_str()));
        SendMessageW(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("filter_applied").c_str()));
        SendMessageW(m_hFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("filter_recommended").c_str()));
        SendMessageW(m_hFilterCombo, CB_SETCURSEL, curSel, 0);
    }

    // Template combo
    if (m_hTemplateCombo) {
        int curSel = static_cast<int>(SendMessageW(m_hTemplateCombo, CB_GETCURSEL, 0, 0));
        if (curSel < 0) curSel = 0;
        SendMessageW(m_hTemplateCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_recommended").c_str()));
        SendMessageW(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_strict").c_str()));
        SendMessageW(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Loc("tpl_minimal").c_str()));
        SendMessageW(m_hTemplateCombo, CB_SETCURSEL, curSel, 0);
    }

    // Buttons
    if (m_hBtnSelectPreset) {
        SetWindowTextW(m_hBtnSelectPreset, Loc("btn_apply_preset").c_str());
    }
    if (m_hBtnRevert) {
        SetWindowTextW(m_hBtnRevert, Loc("btn_apply_defaults").c_str());
    }

    // ListView columns
    if (m_hListView) {
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT;

        col.pszText = const_cast<LPWSTR>(Loc("col_setting").c_str());
        ListView_SetColumn(m_hListView, 0, &col);

        col.pszText = const_cast<LPWSTR>(Loc("col_status").c_str());
        ListView_SetColumn(m_hListView, 1, &col);

        col.pszText = const_cast<LPWSTR>(Loc("col_impact").c_str());
        ListView_SetColumn(m_hListView, 2, &col);

        col.pszText = const_cast<LPWSTR>(Loc("col_scope").c_str());
        ListView_SetColumn(m_hListView, 3, &col);
    }

    // Header Tooltips
    HWND hHeader = ListView_GetHeader(m_hListView);
    if (m_hHeaderTooltip && hHeader) {
        std::wstring tt0 = Loc("tip_col_setting");
        std::wstring tt1 = Loc("tip_col_status");
        std::wstring tt2 = Loc("tip_col_impact");
        std::wstring tt3 = Loc("tip_col_scope");

        std::wstring* tts[4] = { &tt0, &tt1, &tt2, &tt3 };
        for (int i = 0; i < 4; ++i) {
            TOOLINFOW ti{};
            ti.cbSize = sizeof(ti);
            ti.hwnd = hHeader;
            ti.uId = static_cast<UINT_PTR>(i);
            ti.lpszText = const_cast<LPWSTR>(tts[i]->c_str());
            SendMessageW(m_hHeaderTooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ti));
        }
    }

    PopulateListView(m_currentFilter, m_filterMode);
    UpdateToolbarLayout();
}

void MainWindow::UpdateDetailsPane(int selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(m_displayedTweaks.size())) {
        SetWindowTextW(m_hDetailsEdit, L"");
        return;
    }

    const auto& t = m_displayedTweaks[selectedIndex];
    const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});

    std::wstring locTitle = Localization::Instance().GetTweakTitle(t.id, t.title);
    std::wstring locCat = Localization::Instance().GetCategory(t.category);

    std::wstringstream ss;
    ss << locTitle << L"\r\n";

    // State, impact, scope metadata line
    std::wstring stStr;
    switch (st) {
    case SettingStatus::Applied:       stStr = Loc("status_raw_applied"); break;
    case SettingStatus::NotApplied:    stStr = Loc("status_raw_not_applied"); break;
    case SettingStatus::Partial:       stStr = Loc("status_raw_partial"); break;
    case SettingStatus::Custom:        stStr = Loc("status_raw_custom"); break;
    case SettingStatus::Unknown:       stStr = Loc("status_raw_unknown"); break;
    case SettingStatus::NotApplicable: stStr = Loc("status_raw_not_applicable"); break;
    default:                           stStr = Loc("status_raw_not_applied"); break;
    }

    std::wstring impactStr;
    switch (t.impactLevel) {
    case ImpactLevel::Low:      impactStr = Loc("impact_low"); break;
    case ImpactLevel::Moderate: impactStr = Loc("impact_moderate"); break;
    case ImpactLevel::High:     impactStr = Loc("impact_high"); break;
    }

    std::wstring scopeStr;
    switch (t.scope) {
    case TargetScope::Machine: scopeStr = Loc("scope_machine"); break;
    case TargetScope::User:    scopeStr = Loc("scope_user"); break;
    case TargetScope::Service: scopeStr = Loc("scope_service"); break;
    }

    ss << Loc("label_state") << stStr
       << Loc("label_impact") << impactStr
       << Loc("label_scope") << scopeStr
       << Loc("label_recommended") << (t.isRecommended ? Loc("yes") : Loc("no"))
       << Loc("label_category") << locCat << L"\r\n\r\n";

    // 1. What applying it does (practical explanation first)
    ss << Loc("what_it_does") << L"\r\n" << t.description << L"\r\n\r\n";

    // 2. Features it may affect (specific consequence & tradeoff)
    if (!t.impact.empty()) {
        ss << Loc("features_affected") << L"\r\n" << t.impact << L"\r\n\r\n";
    }

    // 3. Restart/sign-out requirement
    ss << Loc("restart_requirement") << L"\r\n";
    if (t.requiresReboot) {
        ss << Loc("reboot_system") << L"\r\n\r\n";
    } else if (t.requiresSignOut) {
        ss << Loc("reboot_signout") << L"\r\n\r\n";
    } else {
        ss << Loc("reboot_none") << L"\r\n\r\n";
    }

    // 4. Technical information below practical explanation
    if (!t.regActions.empty() || !t.serviceActions.empty()) {
        ss << Loc("technical_info") << L"\r\n";
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

void MainWindow::UpdateStatusBar() {
    const size_t pendingApply = m_pendingEnableIds.size();
    const size_t pendingRevert = m_pendingRevertIds.size();
    const size_t totalSelected = pendingApply + pendingRevert;

    size_t shownSelected = 0;
    for (const auto& t : m_displayedTweaks) {
        if (m_pendingEnableIds.count(t.id) > 0 || m_pendingRevertIds.count(t.id) > 0) {
            shownSelected++;
        }
    }
    const size_t hiddenSelected = (totalSelected >= shownSelected) ? (totalSelected - shownSelected) : 0;

    std::wstringstream ss;
    ss << LocFmt("status_applied_count", m_appliedCount) << L" \u00B7 ";
    if (totalSelected > 0) {
        ss << LocFmt("match_selected", totalSelected);
        if (hiddenSelected > 0) {
            ss << L" " << LocFmt("match_hidden", hiddenSelected);
        }
        ss << L" \u00B7 ";
    }
    ss << LocFmt("match_shown", m_displayedTweaks.size());

    SendMessage(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(ss.str().c_str()));

    // Elevation Status
    const bool isAdmin = IsRunningAsAdmin();
    std::wstring part2 = isAdmin ? Loc("elevation_admin") : Loc("elevation_user");
    SendMessage(m_hStatusBar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(part2.c_str()));
}

void MainWindow::SelectPreset(std::string_view templateName) {
    const auto tpl = TemplateManager::Instance().GetTemplate(templateName);
    if (!tpl.has_value()) return;

    m_pendingEnableIds.clear();
    m_pendingRevertIds.clear();

    for (const auto& [id, shouldEnable] : tpl->tweakStates) {
        if (m_notApplicableIds.count(id) > 0) {
            continue;
        }
        if (shouldEnable) {
            if (!TweakRegistry::Instance().MatchesTargetState(id, true, UserSelectionMode::CurrentUser, {})) {
                m_pendingEnableIds.insert(id);
            }
        } else {
            if (!TweakRegistry::Instance().MatchesTargetState(id, false, UserSelectionMode::CurrentUser, {})) {
                m_pendingRevertIds.insert(id);
            }
        }
    }

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        CheckboxState cbState = CheckboxState::Unchecked;
        if (st == SettingStatus::NotApplicable || m_notApplicableIds.count(t.id) > 0) {
            cbState = CheckboxState::Disabled;
        } else if (m_pendingEnableIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingEnable;
        } else if (st == SettingStatus::Applied) {
            cbState = CheckboxState::AlreadyEnabled;
        }
        SetRowCheckboxState(i, cbState);
    }

    UpdateSelectionCounts();
    UpdateStatusBar();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::SelectRecommended() {
    SelectPreset("recommended");
}

void MainWindow::SelectAllShown() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        if (m_notApplicableIds.count(t.id) > 0) continue;
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        if (st == SettingStatus::NotApplicable) continue;
        if (st != SettingStatus::Applied) {
            m_pendingEnableIds.insert(t.id);
            m_pendingRevertIds.erase(t.id);
            SetRowCheckboxState(i, CheckboxState::PendingEnable);
        }
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::InvertShownSelection() {
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        if (m_notApplicableIds.count(t.id) > 0) continue;
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        if (st == SettingStatus::NotApplicable) continue;
        const CheckboxState cur = GetRowCheckboxState(i);
        if (cur == CheckboxState::Disabled) continue;
        if (cur == CheckboxState::PendingEnable) {
            m_pendingEnableIds.erase(t.id);
            SetRowCheckboxState(i, (st == SettingStatus::Applied) ? CheckboxState::AlreadyEnabled : CheckboxState::Unchecked);
        } else if (cur == CheckboxState::PendingRevert) {
            m_pendingRevertIds.erase(t.id);
            SetRowCheckboxState(i, CheckboxState::AlreadyEnabled);
        } else if (cur == CheckboxState::AlreadyEnabled) {
            m_pendingRevertIds.insert(t.id);
            SetRowCheckboxState(i, CheckboxState::PendingRevert);
        } else {
            m_pendingEnableIds.insert(t.id);
            SetRowCheckboxState(i, CheckboxState::PendingEnable);
        }
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::ClearSelection() {
    m_pendingEnableIds.clear();
    m_pendingRevertIds.clear();

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        CheckboxState cbState = CheckboxState::Unchecked;
        if (st == SettingStatus::NotApplicable || m_notApplicableIds.count(t.id) > 0) {
            cbState = CheckboxState::Disabled;
        } else if (st == SettingStatus::Applied) {
            cbState = CheckboxState::AlreadyEnabled;
        }
        SetRowCheckboxState(i, cbState);
    }
    UpdateSelectionCounts();
    UpdateStatusBar();
}

void MainWindow::ApplySelectedTweaks() {
    const size_t pendingApply = m_pendingEnableIds.size();
    const size_t pendingRevert = m_pendingRevertIds.size();
    const size_t totalSelected = pendingApply + pendingRevert;
    if (totalSelected == 0) {
        MessageBoxW(m_hWnd, Loc("no_staged_msg").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string conflictErr;
    if (!ValidateStagedPlan(&conflictErr)) {
        std::wstring wErr(conflictErr.begin(), conflictErr.end());
        MessageBoxW(m_hWnd, wErr.c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    bool needAdmin = false;
    for (const auto& id : m_pendingEnableIds) {
        const auto* t = TweakRegistry::Instance().GetTweakById(id);
        if (t && t->scope != TargetScope::User) { needAdmin = true; break; }
    }
    if (!needAdmin) {
        for (const auto& id : m_pendingRevertIds) {
            const auto* t = TweakRegistry::Instance().GetTweakById(id);
            if (t && t->scope != TargetScope::User) { needAdmin = true; break; }
        }
    }

    if (needAdmin && !IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            Loc("admin_required_prompt").c_str(),
            Loc("app_title").c_str(),
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            RelaunchAsAdminWithPendingState();
        }
        return;
    }

    // Review dialog (Item 7)
    std::vector<const Tweak*> moderateHighTweaks;
    bool anyReboot = false;
    bool anySignOut = false;
    for (const auto& id : m_pendingEnableIds) {
        const auto* t = TweakRegistry::Instance().GetTweakById(id);
        if (t) {
            if (t->impactLevel == ImpactLevel::Moderate || t->impactLevel == ImpactLevel::High) {
                moderateHighTweaks.push_back(t);
            }
            if (t->requiresReboot) anyReboot = true;
            if (t->requiresSignOut) anySignOut = true;
        }
    }
    for (const auto& id : m_pendingRevertIds) {
        const auto* t = TweakRegistry::Instance().GetTweakById(id);
        if (t) {
            if (t->impactLevel == ImpactLevel::Moderate || t->impactLevel == ImpactLevel::High) {
                moderateHighTweaks.push_back(t);
            }
            if (t->requiresReboot) anyReboot = true;
            if (t->requiresSignOut) anySignOut = true;
        }
    }

    std::wstringstream review;
    if (pendingApply > 0) {
        review << LocFmt("review_apply_count", pendingApply);
    }
    if (pendingRevert > 0) {
        review << LocFmt("review_revert_count", pendingRevert);
    }
    review << L"\n";

    if (!moderateHighTweaks.empty()) {
        review << Loc("col_impact") << L" (" << moderateHighTweaks.size() << L"):\n";
        const size_t limit = std::min<size_t>(moderateHighTweaks.size(), 8);
        for (size_t i = 0; i < limit; ++i) {
            const auto* t = moderateHighTweaks[i];
            std::wstring locT = Localization::Instance().GetTweakTitle(t->id, t->title);
            std::wstring impactName = (t->impactLevel == ImpactLevel::High) ? Loc("impact_high") : Loc("impact_moderate");
            review << L"  \u2022 " << locT << L" (" << impactName << L")\n";
        }
        if (moderateHighTweaks.size() > limit) {
            review << L"  ... (" << (moderateHighTweaks.size() - limit) << L")\n";
        }
        review << L"\n";
    }

    if (anyReboot) {
        review << Loc("reboot_system") << L"\n\n";
    } else if (anySignOut) {
        review << Loc("reboot_signout") << L"\n\n";
    }

    review << Loc("review_proceed");
    const int choice = MessageBoxW(m_hWnd, review.str().c_str(), Loc("review_title").c_str(), MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES) return;

    ShowWindow(m_hProgressBar, SW_SHOW);
    SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, static_cast<LPARAM>(totalSelected));
    SendMessage(m_hProgressBar, PBM_SETPOS, 0, 0);

    std::unordered_set<std::string> toExecuteEnables;
    std::unordered_set<std::string> toExecuteReverts;
    int appliedCount = 0;
    int restoredCount = 0;
    int unchangedCount = 0;
    int failedCount = 0;
    int progress = 0;

    for (const auto& id : m_pendingEnableIds) {
        if (TweakRegistry::Instance().MatchesTargetState(id, true, UserSelectionMode::CurrentUser, {})) {
            unchangedCount++;
            progress++;
            SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
        } else {
            toExecuteEnables.insert(id);
        }
    }
    for (const auto& id : m_pendingRevertIds) {
        if (TweakRegistry::Instance().MatchesTargetState(id, false, UserSelectionMode::CurrentUser, {})) {
            unchangedCount++;
            progress++;
            SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
        } else {
            toExecuteReverts.insert(id);
        }
    }

    // Apply entire staged plan
    for (const auto& id : toExecuteEnables) {
        TweakRegistry::Instance().ApplyTweak(id, true, UserSelectionMode::CurrentUser, {});
        progress++;
        SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
    }
    for (const auto& id : toExecuteReverts) {
        TweakRegistry::Instance().ApplyTweak(id, false, UserSelectionMode::CurrentUser, {});
        progress++;
        SendMessage(m_hProgressBar, PBM_SETPOS, progress, 0);
    }

    // Verify final results after the complete plan
    std::unordered_set<std::string> remainingEnable;
    std::unordered_set<std::string> remainingRevert;

    for (const auto& id : toExecuteEnables) {
        if (TweakRegistry::Instance().MatchesTargetState(id, true, UserSelectionMode::CurrentUser, {})) {
            appliedCount++;
        } else {
            failedCount++;
            remainingEnable.insert(id);
        }
    }
    for (const auto& id : toExecuteReverts) {
        if (TweakRegistry::Instance().MatchesTargetState(id, false, UserSelectionMode::CurrentUser, {})) {
            restoredCount++;
        } else {
            failedCount++;
            remainingRevert.insert(id);
        }
    }

    ShowWindow(m_hProgressBar, SW_HIDE);
    m_pendingEnableIds = std::move(remainingEnable);
    m_pendingRevertIds = std::move(remainingRevert);

    RefreshAuditState();

    std::wstring resMsg = (failedCount > 0 ? Loc("apply_completed_warn") : Loc("apply_completed_ok")) +
                          LocFmt("apply_success", appliedCount, restoredCount, unchangedCount, failedCount);
    if (anyReboot) {
        resMsg += Loc("reboot_recommended");
    }
    MessageBoxW(m_hWnd, resMsg.c_str(), Loc("app_title").c_str(), MB_OK | (failedCount == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void MainWindow::RestoreSelectedDefaults() {
    int count = 0;
    int i = -1;
    while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
        if (i >= 0 && i < static_cast<int>(m_displayedTweaks.size())) {
            const auto& t = m_displayedTweaks[i];
            if (m_notApplicableIds.count(t.id) > 0) continue;
            m_pendingEnableIds.erase(t.id);
            m_pendingRevertIds.insert(t.id);
            SetRowCheckboxState(i, CheckboxState::PendingRevert);
            count++;
        }
    }

    if (count == 0) {
        // If no rows highlighted, stage all currently displayed rows that are not in default state
        for (int idx = 0; idx < static_cast<int>(m_displayedTweaks.size()); ++idx) {
            const auto& t = m_displayedTweaks[idx];
            if (m_notApplicableIds.count(t.id) > 0) continue;
            if (!TweakRegistry::Instance().MatchesTargetState(t.id, false, UserSelectionMode::CurrentUser, {})) {
                m_pendingEnableIds.erase(t.id);
                m_pendingRevertIds.insert(t.id);
                SetRowCheckboxState(idx, CheckboxState::PendingRevert);
                count++;
            }
        }
    }

    UpdateSelectionCounts();
    UpdateStatusBar();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::RestoreAllDefaults() {
    m_pendingEnableIds.clear();
    m_pendingRevertIds.clear();

    const auto& catalog = TweakRegistry::Instance().GetAllTweaks();
    for (const auto& t : catalog) {
        if (m_notApplicableIds.count(t.id) > 0) continue;
        if (!TweakRegistry::Instance().MatchesTargetState(t.id, false, UserSelectionMode::CurrentUser, {})) {
            m_pendingRevertIds.insert(t.id);
        }
    }

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        CheckboxState cbState = CheckboxState::Unchecked;
        if (m_notApplicableIds.count(t.id) > 0) {
            cbState = CheckboxState::Disabled;
        } else if (m_pendingRevertIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingRevert;
        } else {
            const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
            if (st == SettingStatus::Applied) {
                cbState = CheckboxState::AlreadyEnabled;
            } else if (st == SettingStatus::NotApplicable) {
                cbState = CheckboxState::Disabled;
            }
        }
        SetRowCheckboxState(i, cbState);
    }

    UpdateSelectionCounts();
    UpdateStatusBar();

    int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (cur != -1) {
        UpdateDetailsPane(cur);
    }
}

void MainWindow::RefreshAuditState() {
    m_appliedCount = 0;
    m_notApplicableIds.clear();
    const auto& catalog = TweakRegistry::Instance().GetAllTweaks();
    for (const auto& t : catalog) {
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
        if (st == SettingStatus::Applied) {
            m_appliedCount++;
        } else if (st == SettingStatus::NotApplicable) {
            m_notApplicableIds.insert(t.id);
        }
    }

    m_isProgrammaticCheckChange = true;
    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});

        std::wstring stStr;
        switch (st) {
        case SettingStatus::Applied:       stStr = L"\u25CF " + Loc("status_raw_applied"); break;
        case SettingStatus::NotApplied:    stStr = L"\u25CB " + Loc("status_raw_not_applied"); break;
        case SettingStatus::Partial:       stStr = L"\u25D0 " + Loc("status_raw_partial"); break;
        case SettingStatus::Custom:        stStr = L"\u25C6 " + Loc("status_raw_custom"); break;
        case SettingStatus::Unknown:       stStr = L"? " + Loc("status_raw_unknown"); break;
        case SettingStatus::NotApplicable: stStr = L"\u2014 " + Loc("status_raw_not_applicable"); break;
        default:                           stStr = L"\u25CB " + Loc("status_raw_not_applied"); break;
        }

        ListView_SetItemText(m_hListView, i, 1, const_cast<LPWSTR>(stStr.c_str()));

        CheckboxState cbState = CheckboxState::Unchecked;
        if (st == SettingStatus::NotApplicable || m_notApplicableIds.count(t.id) > 0) {
            cbState = CheckboxState::Disabled;
        } else if (m_pendingEnableIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingEnable;
        } else if (m_pendingRevertIds.count(t.id) > 0) {
            cbState = CheckboxState::PendingRevert;
        } else if (st == SettingStatus::Applied) {
            cbState = CheckboxState::AlreadyEnabled;
        }
        SetRowCheckboxState(i, cbState);
    }
    m_isProgrammaticCheckChange = false;

    InvalidateRect(m_hListView, nullptr, TRUE);

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

    bool hasApplicable = false;
    int s = -1;
    while ((s = ListView_GetNextItem(m_hListView, s, LVNI_SELECTED)) != -1) {
        if (s >= 0 && s < static_cast<int>(m_displayedTweaks.size()) && m_notApplicableIds.count(m_displayedTweaks[s].id) == 0) {
            hasApplicable = true;
            break;
        }
    }
    const UINT actionFlag = hasApplicable ? MF_STRING : (MF_STRING | MF_GRAYED | MF_DISABLED);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, actionFlag, IDM_CTX_PROTECT_SELECTED, Loc("ctx_select").c_str());
    AppendMenuW(hMenu, actionFlag, IDM_CTX_DEFAULT_SELECTED, Loc("ctx_deselect").c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_ID, Loc("ctx_copy_id").c_str());
    AppendMenuW(hMenu, MF_STRING, IDM_CTX_COPY_DETAILS, Loc("ctx_copy_details").c_str());

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
}

void MainWindow::OnKeyDown(WPARAM vk) {
    if (vk == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SetFocus(m_hSearchEdit);
        SendMessage(m_hSearchEdit, EM_SETSEL, 0, -1);
    } else if (vk == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SelectAllShown();
    } else if (vk == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        ApplySelectedTweaks();
    } else if (vk == 'E' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        ExportConfiguration();
    } else if (vk == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        ImportConfiguration();
    } else if (vk == VK_F5) {
        RefreshAuditState();
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

void MainWindow::OnCommand(int id, int notifyCode, HWND /*hCtrl*/) {
    switch (id) {
    case IDM_FILE_EXPORT:
        ExportConfiguration();
        break;
    case IDM_FILE_IMPORT:
        ImportConfiguration();
        break;
    case IDM_FILE_RESTART_ADMIN:
        if (IsRunningAsAdmin()) {
            MessageBoxW(m_hWnd, Loc("already_admin").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONINFORMATION);
        } else {
            RelaunchAsAdminWithPendingState();
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
        RestoreSelectedDefaults();
        break;
    case IDM_ACT_RESTORE_ALL:
    case IDC_BTN_REVERT:
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
    case IDM_CTX_PROTECT_SELECTED: {
        int i = -1;
        while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
            const auto& t = m_displayedTweaks[i];
            if (m_notApplicableIds.count(t.id) > 0) continue;
            m_pendingEnableIds.insert(t.id);
            m_pendingRevertIds.erase(t.id);
            SetRowCheckboxState(i, CheckboxState::PendingEnable);
        }
        UpdateSelectionCounts();
        UpdateStatusBar();
        break;
    }
    case IDM_CTX_DEFAULT_SELECTED: {
        int i = -1;
        while ((i = ListView_GetNextItem(m_hListView, i, LVNI_SELECTED)) != -1) {
            const auto& t = m_displayedTweaks[i];
            if (m_notApplicableIds.count(t.id) > 0) continue;
            m_pendingEnableIds.erase(t.id);
            m_pendingRevertIds.erase(t.id);
            const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
            if (st == SettingStatus::Applied) {
                SetRowCheckboxState(i, CheckboxState::AlreadyEnabled);
            } else {
                SetRowCheckboxState(i, CheckboxState::Unchecked);
            }
        }
        UpdateSelectionCounts();
        UpdateStatusBar();
        break;
    }
    case IDM_CTX_COPY_ID:
        CopySelectedTweakIds();
        break;
    case IDM_CTX_COPY_DETAILS:
        CopySelectedTweakDetails();
        break;
    case IDC_SEARCH_EDIT:
        if (notifyCode == EN_CHANGE) {
            wchar_t buf[256]{};
            GetWindowTextW(m_hSearchEdit, buf, 256);
            m_currentFilter = buf;
            PopulateListView(m_currentFilter, m_filterMode);
        }
        break;
    case IDC_FILTER_COMBO:
        if (notifyCode == CBN_SELCHANGE) {
            const int sel = static_cast<int>(SendMessage(m_hFilterCombo, CB_GETCURSEL, 0, 0));
            if (sel == 0) m_filterMode = FilterMode::All;
            else if (sel == 1) m_filterMode = FilterMode::NotAppliedOnly;
            else if (sel == 2) m_filterMode = FilterMode::AppliedOnly;
            else if (sel == 3) m_filterMode = FilterMode::RecommendedOnly;
            PopulateListView(m_currentFilter, m_filterMode);
        }
        break;
    case IDM_FOCUS_SEARCH:
        SetFocus(m_hSearchEdit);
        SendMessage(m_hSearchEdit, EM_SETSEL, 0, -1);
        break;
    default:
        if (id >= IDM_LANG_BASE && id <= IDM_LANG_BASE + 11) {
            Language lang = static_cast<Language>(id - IDM_LANG_BASE);
            Localization::Instance().SetLanguage(lang);
            UpdateLocalization();
            return;
        }
        break;
    }
}

void MainWindow::OnNotify(NMHDR* pnmhdr) {
    if (m_hListView && pnmhdr->hwndFrom == ListView_GetHeader(m_hListView)) {
        if (pnmhdr->code == HDN_ITEMCHANGEDW || pnmhdr->code == HDN_ITEMCHANGEDA ||
            pnmhdr->code == HDN_ENDTRACKW || pnmhdr->code == HDN_ENDTRACKA) {
            UpdateHeaderTooltips();
        }
    }

    if (pnmhdr->idFrom == IDC_LIST_TWEAKS) {
        if (pnmhdr->code == LVN_ITEMCHANGED) {
            auto* pItem = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
            if (pItem->iItem >= 0 && pItem->iItem < static_cast<int>(m_displayedTweaks.size())) {
                // Selection change in ListView
                if ((pItem->uChanged & LVIF_STATE) && (pItem->uNewState & LVIS_SELECTED)) {
                    UpdateDetailsPane(pItem->iItem);
                }
            }
        } else if (pnmhdr->code == LVN_KEYDOWN) {
            auto* pnkd = reinterpret_cast<NMLVKEYDOWN*>(pnmhdr);
            if (pnkd->wVKey == VK_SPACE) {
                // Item 15: Space toggles checkbox of focused row
                const int focused = ListView_GetNextItem(m_hListView, -1, LVNI_FOCUSED);
                if (focused >= 0 && focused < static_cast<int>(m_displayedTweaks.size())) {
                    if (m_notApplicableIds.count(m_displayedTweaks[focused].id) > 0) {
                        return;
                    }
                    ToggleRowCheckbox(focused);
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

LRESULT MainWindow::OnCustomDraw(NMHDR* pnmhdr) {
    auto* pCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmhdr);
    switch (pCustomDraw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        const int itemIndex = static_cast<int>(pCustomDraw->nmcd.dwItemSpec);
        if (itemIndex >= 0 && itemIndex < static_cast<int>(m_displayedTweaks.size())) {
            const auto& t = m_displayedTweaks[itemIndex];
            if (m_notApplicableIds.count(t.id) > 0) {
                if (DarkMode::IsHighContrastActive()) {
                    pCustomDraw->clrText = GetSysColor(COLOR_GRAYTEXT);
                } else {
                    const bool isDark = DarkMode::IsDarkModeActive();
                    pCustomDraw->clrText = isDark ? RGB(115, 115, 115) : RGB(145, 145, 145);
                }
                return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
            }
        }
        return CDRF_DODEFAULT;
    }

    case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
        const int itemIndex = static_cast<int>(pCustomDraw->nmcd.dwItemSpec);
        if (itemIndex >= 0 && itemIndex < static_cast<int>(m_displayedTweaks.size())) {
            const auto& t = m_displayedTweaks[itemIndex];
            if (m_notApplicableIds.count(t.id) > 0) {
                if (DarkMode::IsHighContrastActive()) {
                    pCustomDraw->clrText = GetSysColor(COLOR_GRAYTEXT);
                } else {
                    const bool isDark = DarkMode::IsDarkModeActive();
                    pCustomDraw->clrText = isDark ? RGB(115, 115, 115) : RGB(145, 145, 145);
                }
                return CDRF_NEWFONT;
            }
        }
        return CDRF_DODEFAULT;
    }

    default:
        return CDRF_DODEFAULT;
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
        // Export full configuration: all applied or staged tweaks across catalog
        const auto& allTweaks = TweakRegistry::Instance().GetAllTweaks();
        for (const auto& t : allTweaks) {
            if (m_pendingEnableIds.count(t.id) > 0) {
                p.tweakStates[t.id] = true;
            } else if (m_pendingRevertIds.count(t.id) > 0) {
                p.tweakStates[t.id] = false;
            } else {
                const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
                if (st == SettingStatus::Applied) {
                    p.tweakStates[t.id] = true;
                }
            }
        }

        if (TemplateManager::Instance().SaveTemplateToFile(szFile, p)) {
            MessageBoxW(m_hWnd, Loc("export_success").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, Loc("export_fail").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONERROR);
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
        std::string loadErr;
        if (TemplateManager::Instance().LoadTemplateFromFile(szFile, p, &loadErr)) {
            std::string conflictErr;
            if (!TweakRegistry::Instance().ValidatePlanConflicts(p.tweakStates, conflictErr)) {
                std::wstring wErr(conflictErr.begin(), conflictErr.end());
                MessageBoxW(m_hWnd, wErr.c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONERROR);
                return;
            }

            m_pendingEnableIds.clear();
            m_pendingRevertIds.clear();

            for (const auto& [id, shouldEnable] : p.tweakStates) {
                if (shouldEnable) {
                    if (!TweakRegistry::Instance().MatchesTargetState(id, true, UserSelectionMode::CurrentUser, {})) {
                        m_pendingEnableIds.insert(id);
                    }
                } else {
                    if (!TweakRegistry::Instance().MatchesTargetState(id, false, UserSelectionMode::CurrentUser, {})) {
                        m_pendingRevertIds.insert(id);
                    }
                }
            }

            for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
                const auto& t = m_displayedTweaks[i];
                const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::CurrentUser, {});
                CheckboxState cbState = CheckboxState::Unchecked;
                if (m_pendingEnableIds.count(t.id) > 0) {
                    cbState = CheckboxState::PendingEnable;
                } else if (m_pendingRevertIds.count(t.id) > 0) {
                    cbState = CheckboxState::PendingRevert;
                } else if (st == SettingStatus::Applied) {
                    cbState = CheckboxState::AlreadyEnabled;
                }
                SetRowCheckboxState(i, cbState);
            }

            UpdateSelectionCounts();
            UpdateStatusBar();

            int cur = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
            if (cur != -1) {
                UpdateDetailsPane(cur);
            }

            std::wstring msg = LocFmt("import_success", m_pendingEnableIds.size(), m_pendingRevertIds.size());
            MessageBoxW(m_hWnd, msg.c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, Loc("import_fail").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::CreateSystemRestorePoint() {
    if (!IsRunningAsAdmin()) {
        const int res = MessageBoxW(m_hWnd,
            Loc("admin_required_prompt").c_str(),
            Loc("app_title").c_str(),
            MB_YESNO | MB_ICONWARNING);
        if (res == IDYES) {
            RelaunchAsAdminWithPendingState();
        }
        return;
    }

    const int choice = MessageBoxW(m_hWnd,
        Loc("review_restore_point").c_str(),
        Loc("menu_restore_pt").c_str(),
        MB_YESNO | MB_ICONQUESTION);

    if (choice != IDYES) return;

    SetCursor(LoadCursor(nullptr, IDC_WAIT));
    int64_t seqNumber = 0;
    const bool success = RestorePoint::Create(L"PrivatizeWin Baseline Restore Point", seqNumber);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    if (success) {
        MessageBoxW(m_hWnd, Loc("restore_pt_success").c_str(), Loc("app_title").c_str(), MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(m_hWnd, Loc("restore_pt_fail").c_str(), Loc("menu_restore_pt").c_str(), MB_OK | MB_ICONWARNING);
    }
}

LRESULT CALLBACK MainWindow::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass;
    auto* pThis = reinterpret_cast<MainWindow*>(dwRefData);
    if (!pThis) return DefSubclassProc(hWnd, uMsg, wParam, lParam);


    switch (uMsg) {
    case WM_LBUTTONDOWN: {
        LVHITTESTINFO hti{};
        hti.pt.x = GET_X_LPARAM(lParam);
        hti.pt.y = GET_Y_LPARAM(lParam);
        SendMessageW(hWnd, LVM_HITTEST, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&hti));
        if (hti.flags & LVHT_ONITEMSTATEICON) {
            if (hti.iItem >= 0 && hti.iItem < static_cast<int>(pThis->m_displayedTweaks.size())) {
                if (pThis->m_notApplicableIds.count(pThis->m_displayedTweaks[hti.iItem].id) > 0) {
                    pThis->m_mouseDownCheckboxItem = -1;
                    return 0;
                }
            }
            pThis->m_mouseDownCheckboxItem = hti.iItem;
            return 0;
        } else if ((hti.flags & LVHT_EX_GROUP_HEADER) && !(hti.flags & LVHT_EX_GROUP_COLLAPSE)) {
            pThis->m_mouseDownGroupId = (hti.iItem > 0) ? hti.iItem : hti.iGroup;
            pThis->m_mouseDownCheckboxItem = -1;
        } else {
            pThis->m_mouseDownGroupId = -1;
            pThis->m_mouseDownCheckboxItem = -1;
        }
        break;
    }
    case WM_LBUTTONUP: {
        LVHITTESTINFO hti{};
        hti.pt.x = GET_X_LPARAM(lParam);
        hti.pt.y = GET_Y_LPARAM(lParam);
        SendMessageW(hWnd, LVM_HITTEST, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&hti));
        if (pThis->m_mouseDownCheckboxItem != -1) {
            const int targetItem = pThis->m_mouseDownCheckboxItem;
            pThis->m_mouseDownCheckboxItem = -1;
            if ((hti.flags & LVHT_ONITEMSTATEICON) && hti.iItem == targetItem) {
                pThis->ToggleRowCheckbox(targetItem);
                return 0;
            }
        }
        if ((hti.flags & LVHT_EX_GROUP_HEADER) && !(hti.flags & LVHT_EX_GROUP_COLLAPSE)) {
            const int targetGroupId = (hti.iItem > 0) ? hti.iItem : hti.iGroup;
            if (pThis->m_mouseDownGroupId == targetGroupId && targetGroupId > 0) {
                const UINT curState = ListView_GetGroupState(hWnd, targetGroupId, LVGS_COLLAPSED);
                const UINT newState = (curState & LVGS_COLLAPSED) ? 0 : LVGS_COLLAPSED;
                LVGROUP grp{};
                grp.cbSize = sizeof(grp);
                grp.mask = LVGF_STATE;
                grp.stateMask = LVGS_COLLAPSED;
                grp.state = newState;
                ListView_SetGroupInfo(hWnd, targetGroupId, &grp);
                pThis->m_mouseDownGroupId = -1;
                return 0;
            }
        }
        pThis->m_mouseDownGroupId = -1;
        pThis->m_mouseDownCheckboxItem = -1;
        break;
    }
    case WM_LBUTTONDBLCLK: {
        LVHITTESTINFO hti{};
        hti.pt.x = GET_X_LPARAM(lParam);
        hti.pt.y = GET_Y_LPARAM(lParam);
        SendMessageW(hWnd, LVM_HITTEST, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&hti));
        if (hti.flags & LVHT_ONITEMSTATEICON) {
            if (hti.iItem >= 0 && hti.iItem < static_cast<int>(pThis->m_displayedTweaks.size())) {
                if (pThis->m_notApplicableIds.count(pThis->m_displayedTweaks[hti.iItem].id) > 0) {
                    return 0;
                }
            }
            pThis->ToggleRowCheckbox(hti.iItem);
            return 0;
        }
        if ((hti.flags & LVHT_EX_GROUP_HEADER) && !(hti.flags & LVHT_EX_GROUP_COLLAPSE)) {
            const int targetGroupId = (hti.iItem > 0) ? hti.iItem : hti.iGroup;
            if (targetGroupId > 0) {
                const UINT curState = ListView_GetGroupState(hWnd, targetGroupId, LVGS_COLLAPSED);
                const UINT newState = (curState & LVGS_COLLAPSED) ? 0 : LVGS_COLLAPSED;
                LVGROUP grp{};
                grp.cbSize = sizeof(grp);
                grp.mask = LVGF_STATE;
                grp.stateMask = LVGS_COLLAPSED;
                grp.state = newState;
                ListView_SetGroupInfo(hWnd, targetGroupId, &grp);
                return 0;
            }
        }
        break;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_UP && (GetKeyState(VK_CONTROL) & 0x8000)) {
            pThis->m_splitterY = std::max(180, pThis->m_splitterY - 20);
            pThis->UpdateSplitterLayout();
            pThis->SavePreferences();
            return 0;
        }
        if (wParam == VK_DOWN && (GetKeyState(VK_CONTROL) & 0x8000)) {
            pThis->m_splitterY = std::min(1000, pThis->m_splitterY + 20);
            pThis->UpdateSplitterLayout();
            pThis->SavePreferences();
            return 0;
        }
        if (wParam == VK_SPACE) {
            const int focused = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED);
            if (focused >= 0 && focused < static_cast<int>(pThis->m_displayedTweaks.size())) {
                if (pThis->m_notApplicableIds.count(pThis->m_displayedTweaks[focused].id) > 0) {
                    return 0;
                }
                pThis->ToggleRowCheckbox(focused);
                return 0;
            }
        }
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

} // namespace PrivatizeWin

