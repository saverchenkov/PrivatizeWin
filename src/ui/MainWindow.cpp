#include "MainWindow.h"
#include "DarkMode.h"
#include "ScheduleDialog.h"
#include "../../res/resource.h"
#include "../core/TweakRegistry.h"
#include "../core/TemplateManager.h"
#include "../core/RestorePoint.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <sstream>
#include <algorithm>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace PrivatizeWin {

static std::unique_ptr<MainWindow> s_pMainWnd = nullptr;

bool MainWindow::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PrivatizeWin_MainWindow";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.lpszMenuName = L"MAINMENU";
    return (RegisterClassExW(&wc) != 0);
}

HWND MainWindow::Create(HINSTANCE hInstance) {
    return CreateWindowExW(
        WS_EX_WINDOWEDGE,
        L"PrivatizeWin_MainWindow",
        L"PrivatizeWin - Windows Privacy & Telemetry Silencer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 700,
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
        case WM_SIZE:
            s_pMainWnd->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            s_pMainWnd->OnCommand(LOWORD(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_NOTIFY:
            s_pMainWnd->OnNotify(reinterpret_cast<NMHDR*>(lParam));
            return 0;
        case WM_DESTROY:
            s_pMainWnd.reset();
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {}

void MainWindow::OnCreate() {
    DarkMode::ApplyToWindow(m_hWnd);

    const HICON hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_APPICON));
    if (hIcon) {
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    }

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    InitializeControls();

    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    PopulateCategories();
    PopulateListView(L"All Settings", L"");
    UpdateStatusBar();
}

void MainWindow::InitializeControls() {
    const auto hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    // Toolbar Container
    m_hToolbar = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY, 0, 0, 1024, 45, m_hWnd, nullptr, nullptr, nullptr);

    // Search Label & Edit
    HWND hLblSearch = CreateWindowW(L"STATIC", L"Search:", WS_CHILD | WS_VISIBLE, 12, 14, 50, 20, m_hToolbar, nullptr, nullptr, nullptr);
    SendMessage(hLblSearch, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP, 65, 10, 200, 24, m_hToolbar, reinterpret_cast<HMENU>(IDC_SEARCH_EDIT), nullptr, nullptr);
    SendMessage(m_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter tweaks (Ctrl+F)..."));

    // Template Selector
    HWND hLblTpl = CreateWindowW(L"STATIC", L"Preset:", WS_CHILD | WS_VISIBLE, 280, 14, 50, 20, m_hToolbar, nullptr, nullptr, nullptr);
    SendMessage(hLblTpl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hTemplateCombo = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 335, 10, 180, 200, m_hToolbar, reinterpret_cast<HMENU>(IDC_TPL_COMBO), nullptr, nullptr);
    SendMessage(m_hTemplateCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Recommended (Safe)"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strict Privacy"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minimal Telemetry"));
    SendMessage(m_hTemplateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Factory Defaults"));
    SendMessage(m_hTemplateCombo, CB_SETCURSEL, 0, 0);

    // Action Buttons
    m_hBtnApply = CreateWindowW(WC_BUTTONW, L"Apply Changes", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 530, 9, 120, 26, m_hToolbar, reinterpret_cast<HMENU>(IDC_BTN_APPLY), nullptr, nullptr);
    SendMessage(m_hBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnRevert = CreateWindowW(WC_BUTTONW, L"Revert Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 660, 9, 120, 26, m_hToolbar, reinterpret_cast<HMENU>(IDC_BTN_REVERT), nullptr, nullptr);
    SendMessage(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnRefresh = CreateWindowW(WC_BUTTONW, L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 790, 9, 80, 26, m_hToolbar, reinterpret_cast<HMENU>(IDC_BTN_REFRESH), nullptr, nullptr);
    SendMessage(m_hBtnRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Category TreeView
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        10, 50, m_splitterX - 15, m_splitterY - 55, m_hWnd, reinterpret_cast<HMENU>(IDC_TREE_CATEGORIES), nullptr, nullptr);
    SendMessage(m_hTreeView, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    DarkMode::ApplyToControl(m_hTreeView);

    // Main ListView
    m_hListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        m_splitterX, 50, 1024 - m_splitterX - 20, m_splitterY - 55, m_hWnd, reinterpret_cast<HMENU>(IDC_LIST_TWEAKS), nullptr, nullptr);
    SendMessage(m_hListView, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    DarkMode::ApplyToControl(m_hListView);

    // ListView Columns
    LVCOLUMNW lvc{};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"Privacy Tweak");
    lvc.cx = 400;
    ListView_InsertColumn(m_hListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Current Status");
    lvc.cx = 120;
    ListView_InsertColumn(m_hListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Safety Level");
    lvc.cx = 100;
    ListView_InsertColumn(m_hListView, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.pszText = const_cast<LPWSTR>(L"Scope");
    lvc.cx = 80;
    ListView_InsertColumn(m_hListView, 3, &lvc);

    // Details Edit Control
    m_hDetailsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        10, m_splitterY + 5, 1024 - 30, 200, m_hWnd, reinterpret_cast<HMENU>(IDC_EDIT_DETAILS), nullptr, nullptr);
    SendMessage(m_hDetailsEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    DarkMode::ApplyToControl(m_hDetailsEdit);

    // Status Bar
    m_hStatusBar = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), nullptr, nullptr);
    int sbParts[] = { 350, 650, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(sbParts));
}

void MainWindow::OnSize(int width, int height) {
    if (m_hToolbar) MoveWindow(m_hToolbar, 0, 0, width, 45, TRUE);

    int sbHeight = 22;
    if (m_hStatusBar) {
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        RECT rcSb{};
        GetWindowRect(m_hStatusBar, &rcSb);
        sbHeight = rcSb.bottom - rcSb.top;
    }

    const int contentHeight = height - 45 - sbHeight - 10;
    if (m_splitterY > contentHeight - 80) m_splitterY = contentHeight - 120;
    if (m_splitterY < 150) m_splitterY = 150;

    // TreeView
    MoveWindow(m_hTreeView, 10, 48, m_splitterX - 15, m_splitterY - 48, TRUE);

    // ListView
    MoveWindow(m_hListView, m_splitterX + 5, 48, width - m_splitterX - 15, m_splitterY - 48, TRUE);

    // Details
    const int detailsTop = m_splitterY + 8;
    const int detailsHeight = height - detailsTop - sbHeight - 5;
    if (detailsHeight > 0) {
        MoveWindow(m_hDetailsEdit, 10, detailsTop, width - 20, detailsHeight, TRUE);
    }
}

void MainWindow::PopulateCategories() {
    TreeView_DeleteAllItems(m_hTreeView);
    const auto categories = TweakRegistry::Instance().GetCategories();

    for (const auto& cat : categories) {
        std::wstring label = cat.name + L" (" + std::to_wstring(cat.totalCount) + L")";
        TVINSERTSTRUCTW tvis{};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvis.item.pszText = const_cast<LPWSTR>(label.c_str());
        TreeView_InsertItem(m_hTreeView, &tvis);
    }
}

void MainWindow::PopulateListView(std::wstring_view category, std::wstring_view filter) {
    ListView_DeleteAllItems(m_hListView);
    m_displayedTweaks = TweakRegistry::Instance().GetTweaksByCategory(category);

    std::wstring lowerFilter(filter);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::towlower);

    std::vector<Tweak> filtered;
    for (const auto& t : m_displayedTweaks) {
        if (!lowerFilter.empty()) {
            std::wstring lowerTitle = t.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::towlower);
            if (lowerTitle.find(lowerFilter) == std::wstring::npos) {
                continue;
            }
        }
        filtered.push_back(t);
    }

    m_displayedTweaks = std::move(filtered);
    m_checkedStates.assign(m_displayedTweaks.size(), false);

    for (int i = 0; i < static_cast<int>(m_displayedTweaks.size()); ++i) {
        const auto& t = m_displayedTweaks[i];

        LVITEMW lvi{};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(t.title.c_str());
        ListView_InsertItem(m_hListView, &lvi);

        // Status
        const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {});
        std::wstring stStr = (st == SettingStatus::Protected) ? L"Protected" : L"Default";
        ListView_SetItemText(m_hListView, i, 1, const_cast<LPWSTR>(stStr.c_str()));

        // Safety
        std::wstring safeStr = L"Safe";
        if (t.safety == SafetyLevel::Normal) safeStr = L"Normal";
        else if (t.safety == SafetyLevel::Advanced) safeStr = L"Advanced";
        ListView_SetItemText(m_hListView, i, 2, const_cast<LPWSTR>(safeStr.c_str()));

        // Scope
        std::wstring scopeStr = L"Machine";
        if (t.scope == TargetScope::User) scopeStr = L"User";
        else if (t.scope == TargetScope::Both) scopeStr = L"Both";
        else if (t.scope == TargetScope::Service) scopeStr = L"Service";
        ListView_SetItemText(m_hListView, i, 3, const_cast<LPWSTR>(scopeStr.c_str()));

        // Set checkbox initial state to match protected status
        const bool isProtected = (st == SettingStatus::Protected);
        m_checkedStates[i] = isProtected;
        ListView_SetCheckState(m_hListView, i, isProtected);
    }

    if (!m_displayedTweaks.empty()) {
        UpdateDetailsPane(0);
    } else {
        SetWindowTextW(m_hDetailsEdit, L"No settings found matching current category and search filter.");
    }
}

void MainWindow::UpdateDetailsPane(int selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(m_displayedTweaks.size())) {
        SetWindowTextW(m_hDetailsEdit, L"");
        return;
    }

    const auto& t = m_displayedTweaks[selectedIndex];
    std::wstringstream ss;
    ss << L"TWEAK: " << t.title << L" [" << std::wstring(t.id.begin(), t.id.end()) << L"]\r\n";
    ss << L"CATEGORY: " << t.category << L"  |  RECOMMENDATION: ";

    if (t.safety == SafetyLevel::Safe) ss << L"SAFE (Recommended for all users)\r\n";
    else if (t.safety == SafetyLevel::Normal) ss << L"NORMAL (Disables optional services)\r\n";
    else ss << L"ADVANCED (May affect specific hardware/apps)\r\n";

    ss << L"\r\nDESCRIPTION:\r\n" << t.description << L"\r\n\r\n";
    ss << L"PRIVACY IMPACT & RATIONALE:\r\n" << t.impact << L"\r\n\r\n";

    if (!t.regActions.empty()) {
        ss << L"AFFECTED REGISTRY KEYS:\r\n";
        for (const auto& ra : t.regActions) {
            std::wstring rootStr = (ra.scope == TargetScope::Machine) ? L"HKLM" : L"HKCU / Users";
            ss << L"  * [" << rootStr << L"\\] " << ra.subKey << L" -> " << ra.valueName << L" = " << ra.dwordProtected << L"\r\n";
        }
    }

    if (!t.serviceActions.empty()) {
        ss << L"AFFECTED WINDOWS SERVICES:\r\n";
        for (const auto& sa : t.serviceActions) {
            ss << L"  * " << sa.serviceName << L" (Startup type disabled)\r\n";
        }
    }

    SetWindowTextW(m_hDetailsEdit, ss.str().c_str());
}

void MainWindow::UpdateStatusBar() {
    const auto& all = TweakRegistry::Instance().GetAllTweaks();
    int protectedCount = 0;
    for (const auto& t : all) {
        if (TweakRegistry::Instance().AuditTweak(t.id, UserSelectionMode::AllUsers, {}) == SettingStatus::Protected) {
            protectedCount++;
        }
    }

    std::wstring part1 = L"Privatized: " + std::to_wstring(protectedCount) + L" / " + std::to_wstring(all.size()) + L" settings";
    SendMessage(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(part1.c_str()));

    std::wstring part2 = L"Active Category: " + m_currentCategory;
    SendMessage(m_hStatusBar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(part2.c_str()));

    SendMessage(m_hStatusBar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Mode: Administrator (Full Elevation)"));
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
            L"PrivatizeWin v1.0\n"
            L"Open-source Windows Privacy & Telemetry Silencer\n"
            L"Minimalist, zero-footprint Sysinternals-grade utility.\n\n"
            L"Copyright (C) 2026 PrivatizeWin Project (MIT License)",
            L"About PrivatizeWin", MB_OK | MB_ICONINFORMATION);
        break;
    case IDM_HELP_GITHUB:
        ShellExecuteW(nullptr, L"open", L"https://github.com/saverchenkov/PrivatizeWin", nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case IDC_SEARCH_EDIT:
        if (HIWORD(reinterpret_cast<DWORD_PTR>(hCtrl)) == EN_CHANGE || (hCtrl == nullptr && GetFocus() == m_hSearchEdit)) {
            wchar_t buf[256]{};
            GetWindowTextW(m_hSearchEdit, buf, 256);
            m_currentFilter = buf;
            PopulateListView(m_currentCategory, m_currentFilter);
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
            PopulateListView(m_currentCategory, m_currentFilter);
            UpdateStatusBar();
        }
    } else if (pnmhdr->idFrom == IDC_LIST_TWEAKS) {
        if (pnmhdr->code == LVN_ITEMCHANGED) {
            auto* pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmhdr);
            if (pnmv->uNewState & LVIS_SELECTED) {
                UpdateDetailsPane(pnmv->iItem);
            }
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
    PopulateListView(m_currentCategory, m_currentFilter);
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
            MessageBoxW(m_hWnd, L"Configuration successfully imported. Review the checkboxes and click 'Apply Changes'.", L"Import Configuration", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"Invalid or corrupted JSON template file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

} // namespace PrivatizeWin
