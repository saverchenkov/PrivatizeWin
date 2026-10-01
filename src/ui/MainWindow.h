#pragma once

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <memory>
#include "../core/Types.h"

namespace PrivatizeWin {

enum class FilterMode {
    All,
    NotAppliedOnly,
    AppliedOnly,
    RecommendedOnly
};

class MainWindow {
public:
    static bool RegisterClass(HINSTANCE hInstance);
    [[nodiscard]] static HWND Create(HINSTANCE hInstance);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SplitterWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    explicit MainWindow(HWND hWnd);
    ~MainWindow();

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
    void OnLButtonDown(int x, int y);
    void OnLButtonUp();
    void OnMouseMove(int x, int y);
    LRESULT OnCustomDraw(NMHDR* pnmhdr);

    void InitializeFonts();
    void InitializeControls();
    void PopulateListView(std::wstring_view filter = L"", FilterMode mode = FilterMode::All);
    void UpdateDetailsPane(int selectedIndex);
    void UpdateStatusBar();
    void UpdateSelectionCounts();
    void ToggleSelectedTweakFromDetails();
    void UpdateSplitterLayout();

    void ApplyPreset(std::string_view templateName);
    void SelectPreset(std::string_view templateName);
    void SelectRecommended();
    void SelectAllShown();
    void InvertShownSelection();
    void ClearSelection();
    void ShowSelectMenu();

    void ApplySelectedTweaks();
    void RestoreSelectedDefaults();
    void RestoreAllDefaults();
    void RefreshAuditState();

    void ExportConfiguration();
    void ImportConfiguration();
    void CreateSystemRestorePoint();

    void CopySelectedTweakIds();
    void CopySelectedTweakDetails();

    void LoadPreferences();
    void SavePreferences();

    HWND m_hWnd{ nullptr };
    HWND m_hSearchEdit{ nullptr };
    HWND m_hFilterCombo{ nullptr };
    HWND m_hLblMatchCount{ nullptr };
    HWND m_hTemplateCombo{ nullptr };
    HWND m_hBtnSelectPreset{ nullptr };
    HWND m_hBtnSelectMenu{ nullptr };
    HWND m_hBtnApply{ nullptr };
    HWND m_hBtnRevert{ nullptr };
    HWND m_hBtnRefresh{ nullptr };

    // Standard Controls
    HWND m_hListView{ nullptr };
    HWND m_hSplitterBar{ nullptr };
    HWND m_hDetailsEdit{ nullptr };
    HWND m_hBtnToggleTweak{ nullptr };
    HWND m_hBtnCopyTweak{ nullptr };
    HWND m_hStatusBar{ nullptr };
    HWND m_hProgressBar{ nullptr };

    HFONT m_hFontRegular{ nullptr };
    HFONT m_hFontBold{ nullptr };
    HFONT m_hFontTitle{ nullptr };
    HFONT m_hFontCode{ nullptr };

    HIMAGELIST m_hRowImageList{ nullptr };

    std::wstring m_currentFilter;
    FilterMode m_filterMode{ FilterMode::All };
    std::vector<Tweak> m_displayedTweaks;
    std::unordered_set<std::string> m_selectedTweakIds; // Stable selection by tweak ID (Items 1, 3, 15)
    int m_appliedCount{ 0 };
    bool m_isProgrammaticCheckChange{ false };
    HMODULE m_hRichEditLib{ nullptr };

    // Adjustable splitter (Item 13)
    int m_splitterY{ 430 };
    bool m_isDraggingSplitter{ false };

    // Group header collapse interaction
    int m_mouseDownGroupId{ -1 };
    static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};

} // namespace PrivatizeWin
