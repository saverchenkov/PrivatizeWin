#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "../core/Types.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// R.11: Avoid calling new and delete explicitly
// F.16: Pass string types by string_view
// ES.48: Avoid casts

class MainWindow {
public:
    static bool RegisterClass(HINSTANCE hInstance);
    [[nodiscard]] static HWND Create(HINSTANCE hInstance);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    explicit MainWindow(HWND hWnd);
    ~MainWindow() = default;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

private:
    void OnCreate();
    void OnSize(int width, int height);
    void OnCommand(int id, HWND hCtrl);
    void OnNotify(NMHDR* pnmhdr);

    void InitializeControls();
    void PopulateCategories();
    void PopulateListView(std::wstring_view category = L"All Settings", std::wstring_view filter = L"");
    void UpdateDetailsPane(int selectedIndex);
    void UpdateStatusBar();

    void ApplyTemplate(std::string_view templateName);
    void ApplyCurrentSelection();
    void RevertAllToDefaults();
    void RefreshAuditState();

    void ExportConfiguration();
    void ImportConfiguration();
    void CreateSystemRestorePoint();

    HWND m_hWnd{ nullptr };
    HWND m_hToolbar{ nullptr };
    HWND m_hSearchEdit{ nullptr };
    HWND m_hTemplateCombo{ nullptr };
    HWND m_hBtnApply{ nullptr };
    HWND m_hBtnRevert{ nullptr };
    HWND m_hBtnRefresh{ nullptr };

    HWND m_hTreeView{ nullptr };
    HWND m_hListView{ nullptr };
    HWND m_hDetailsEdit{ nullptr };
    HWND m_hStatusBar{ nullptr };

    std::wstring m_currentCategory{ L"All Settings" };
    std::wstring m_currentFilter;
    std::vector<Tweak> m_displayedTweaks;
    std::vector<bool> m_checkedStates;

    int m_splitterX{ 240 };
    int m_splitterY{ 380 };
    bool m_isDraggingSplitter{ false };
};

} // namespace PrivatizeWin
