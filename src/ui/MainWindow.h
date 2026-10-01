#pragma once

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "../core/Types.h"

namespace PrivatizeWin {

enum class FilterMode {
    All,
    UnprotectedOnly,
    ProtectedOnly,
    RecommendedOnly,
    PendingChanges
};

class MainWindow {
public:
    static bool RegisterClass(HINSTANCE hInstance);
    [[nodiscard]] static HWND Create(HINSTANCE hInstance);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
    LRESULT OnCustomDraw(NMHDR* pnmhdr);

    void InitializeFonts();
    void InitializeControls();
    void PopulateListView(std::wstring_view filter = L"", FilterMode mode = FilterMode::All);
    void UpdateDetailsPane(int selectedIndex);
    void UpdateStatusBar();
    void UpdateSelectAllCheckboxState();
    void ToggleCurrentTweakFromDetails();

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
    HWND m_hSearchEdit{ nullptr };
    HWND m_hFilterCombo{ nullptr };
    HWND m_hLblMatchCount{ nullptr };
    HWND m_hTemplateCombo{ nullptr };
    HWND m_hChkSelectAll{ nullptr };
    HWND m_hBtnApply{ nullptr };
    HWND m_hBtnRevert{ nullptr };
    HWND m_hBtnRefresh{ nullptr };

    // Standard Controls (Unified Grouped ListView & Native RichEdit Inspector)
    HWND m_hListView{ nullptr };
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
    std::vector<bool> m_checkedStates;
    HMODULE m_hRichEditLib{ nullptr };
};

} // namespace PrivatizeWin
