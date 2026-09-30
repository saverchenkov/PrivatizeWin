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
// Enum.3: Use enum class

enum class SplitterDragMode {
    None,
    Vertical,   // between TreeView and ListView
    Horizontal  // between top panes and bottom Details pane
};

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
    void OnContextMenu(HWND hWnd, int x, int y);
    void OnKeyDown(WPARAM vk);

    void InitializeControls();
    void PopulateCategories();
    void PopulateListView(std::wstring_view category = L"All Settings", std::wstring_view filter = L"");
    void UpdateDetailsPane(int selectedIndex);
    void UpdateStatusBar();
    void UpdateSelectAllCheckboxState();

    void ApplyTemplate(std::string_view templateName);
    void ApplyCurrentSelection();
    void ApplySelectedItemsOnly();
    void RevertAllToDefaults();
    void RefreshAuditState();

    void ExportConfiguration();
    void ImportConfiguration();
    void CreateSystemRestorePoint();

    void SetSelectedItemsChecked(bool checked);
    void InvertSelectedItemsChecked();
    void CopySelectedTweakIds();
    void CopySelectedTweakDetails();
    void SelectAllListItems();

    HWND m_hWnd{ nullptr };
    HWND m_hToolbar{ nullptr };
    HWND m_hSearchEdit{ nullptr };
    HWND m_hTemplateCombo{ nullptr };
    HWND m_hChkSelectAll{ nullptr };
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
    SplitterDragMode m_dragMode{ SplitterDragMode::None };
};

} // namespace PrivatizeWin
