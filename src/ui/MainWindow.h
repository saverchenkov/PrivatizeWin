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

// Multi-state Checkbox (All-or-Nothing)
enum class CheckboxState : UINT {
    None = 0,
    Unchecked = 1,      // [ ] Empty box - Windows default / not applied
    AlreadyEnabled = 2, // [✔] Solid green box - Already applied in Windows
    PendingEnable = 3,  // [☑] Solid blue box - Selected / pending to be applied
    PendingRevert = 4,  // [-] Solid red box - Selected / pending to be restored/reverted
    Disabled = 5        // [ ] Faint grey box - Not applicable / disabled
};

class MainWindow {
public:
    static bool RegisterClass(HINSTANCE hInstance);
    [[nodiscard]] static HWND Create(HINSTANCE hInstance, std::wstring_view resumePendingFile = L"");

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
    void OnCommand(int id, int notifyCode, HWND hCtrl);
    void OnNotify(NMHDR* pnmhdr);
    void OnContextMenu(HWND hWnd, int x, int y);
    void OnKeyDown(WPARAM vk);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp();
    void OnMouseMove(int x, int y);
    LRESULT OnCustomDraw(NMHDR* pnmhdr);

    void InitializeFonts();
    void InitializeControls();
    void CreateStateImages();
    void SetRowCheckboxState(int itemIndex, CheckboxState state);
    [[nodiscard]] CheckboxState GetRowCheckboxState(int itemIndex) const;
    void ToggleRowCheckbox(int itemIndex);
    void PopulateListView(std::wstring_view filter = L"", FilterMode mode = FilterMode::All);
    void UpdateDetailsPane(int selectedIndex);
    void UpdateStatusBar();
    void UpdateSelectionCounts();
    int GetApplyButtonWidth() const;
    void UpdateActionButtonsLayout(int clientWidth = -1);
    void UpdateSplitterLayout();
    void InitializeHeaderTooltips();
    void UpdateHeaderTooltips();

    void SelectPreset(std::string_view templateName);
    void SelectRecommended();
    void SelectAllShown();
    void InvertShownSelection();
    void ClearSelection();

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

    bool RelaunchAsAdminWithPendingState();
    [[nodiscard]] std::wstring SavePendingStateToTempFile() const;
    bool RestorePendingStateFromFile(const std::wstring& filePath);
    inline static std::wstring s_resumePendingFile;

    HWND m_hWnd{ nullptr };
    HWND m_hSearchEdit{ nullptr };
    HWND m_hFilterCombo{ nullptr };
    HWND m_hLblMatchCount{ nullptr };
    HWND m_hTemplateCombo{ nullptr };
    HWND m_hBtnSelectPreset{ nullptr };
    HWND m_hBtnApply{ nullptr };
    HWND m_hBtnRevert{ nullptr };
    HWND m_hBtnRefresh{ nullptr };

    // Standard Controls
    HWND m_hListView{ nullptr };
    HWND m_hHeaderTooltip{ nullptr };
    HWND m_hSplitterBar{ nullptr };
    HWND m_hDetailsEdit{ nullptr };
    HWND m_hStatusBar{ nullptr };
    HWND m_hProgressBar{ nullptr };

    HFONT m_hFontRegular{ nullptr };
    HFONT m_hFontBold{ nullptr };
    HFONT m_hFontTitle{ nullptr };
    HFONT m_hFontCode{ nullptr };

    HIMAGELIST m_hRowImageList{ nullptr };
    HIMAGELIST m_hStateImageList{ nullptr }; // Multi-state checkbox image list (Item 10)

    std::wstring m_currentFilter;
    FilterMode m_filterMode{ FilterMode::All };
    std::vector<Tweak> m_displayedTweaks;
    std::unordered_set<std::string> m_pendingEnableIds; // Staged to apply (Item 10)
    std::unordered_set<std::string> m_pendingRevertIds; // Staged to restore (Item 10)
    std::unordered_set<std::string> m_notApplicableIds; // Evaluated as NotApplicable
    int m_appliedCount{ 0 };
    bool m_isProgrammaticCheckChange{ false };
    HMODULE m_hRichEditLib{ nullptr };

    // Adjustable splitter (Item 13)
    int m_splitterY{ 430 };
    bool m_isDraggingSplitter{ false };

    // Group header collapse & checkbox click tracking
    int m_mouseDownGroupId{ -1 };
    int m_mouseDownCheckboxItem{ -1 };
    static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};

} // namespace PrivatizeWin
