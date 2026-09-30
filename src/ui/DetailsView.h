#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <optional>
#include "../core/Types.h"

namespace PrivatizeWin {

class DetailsView {
public:
    static bool RegisterClass(HINSTANCE hInstance);
    [[nodiscard]] static HWND Create(HWND hParent, HINSTANCE hInstance, int id, int x, int y, int width, int height);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    explicit DetailsView(HWND hWnd);
    ~DetailsView() = default;

    void SetTweak(const Tweak* pTweak, bool isChecked);
    void SetCheckedState(bool isChecked);
    void SetDarkMode(bool isDark);
    void SetFonts(HFONT hFontRegular, HFONT hFontBold, HFONT hFontTitle, HFONT hFontCode, HFONT hFontBadge);

private:
    void OnPaint(HDC hdc);
    void OnSize(int width, int height);
    void OnVScroll(WORD scrollCode, short pos);
    void OnMouseWheel(short delta);
    void OnCommand(int id);
    void CopyTweakInfo();
    void DrawBadge(HDC hdc, int x, int y, int height, const std::wstring& text, COLORREF bg, COLORREF textCol, COLORREF borderCol, HFONT hFont, int& outWidth);

    HWND m_hWnd{ nullptr };
    HWND m_hParent{ nullptr };
    HWND m_hBtnToggle{ nullptr };
    HWND m_hBtnCopy{ nullptr };

    std::optional<Tweak> m_currentTweak;
    bool m_isChecked{ false };
    bool m_isDark{ false };
    int m_scrollY{ 0 };
    int m_totalContentHeight{ 0 };

    HFONT m_hFontRegular{ nullptr };
    HFONT m_hFontBold{ nullptr };
    HFONT m_hFontTitle{ nullptr };
    HFONT m_hFontCode{ nullptr };
    HFONT m_hFontBadge{ nullptr };
};

} // namespace PrivatizeWin
