#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "DetailsView.h"
#include "../../res/resource.h"
#include <commctrl.h>
#include <sstream>
#include <algorithm>

namespace PrivatizeWin {

static const wchar_t* DETAILS_VIEW_CLASS = L"PrivatizeWin_DetailsView";

bool DetailsView::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = DetailsView::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = DETAILS_VIEW_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Handled in WM_ERASEBKGND/WM_PAINT
    wc.style = CS_HREDRAW | CS_VREDRAW;
    return (RegisterClassExW(&wc) != 0);
}

HWND DetailsView::Create(HWND hParent, HINSTANCE hInstance, int id, int x, int y, int width, int height) {
    return CreateWindowExW(
        0, // Clean borderless container inside splitter frame
        DETAILS_VIEW_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL,
        x, y, width, height,
        hParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, nullptr
    );
}

LRESULT CALLBACK DetailsView::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto* pThis = reinterpret_cast<DetailsView*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_NCCREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* view = new DetailsView(hWnd);
        view->m_hParent = cs->hwndParent;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        return TRUE;
    }
    case WM_CREATE: {
        if (pThis) {
            HINSTANCE hInst = GetModuleHandle(nullptr);
            pThis->m_hBtnToggle = CreateWindowW(
                WC_BUTTONW, L"Enable Protection",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 140, 26, hWnd,
                reinterpret_cast<HMENU>(IDC_DETAILS_BTN_TOGGLE), hInst, nullptr
            );

            pThis->m_hBtnCopy = CreateWindowW(
                WC_BUTTONW, L"Copy Info",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 85, 26, hWnd,
                reinterpret_cast<HMENU>(IDC_DETAILS_BTN_COPY), hInst, nullptr
            );

            ShowWindow(pThis->m_hBtnToggle, SW_HIDE);
            ShowWindow(pThis->m_hBtnCopy, SW_HIDE);
        }
        return 0;
    }
    case WM_SIZE:
        if (pThis) {
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    case WM_PAINT: {
        if (pThis) {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hWnd, &ps);
            pThis->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        break;
    }
    case WM_ERASEBKGND:
        return 1; // Handled in double-buffered paint
    case WM_VSCROLL:
        if (pThis) {
            pThis->OnVScroll(LOWORD(wParam), static_cast<short>(HIWORD(wParam)));
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (pThis) {
            pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        }
        return 0;
    case WM_COMMAND:
        if (pThis) {
            pThis->OnCommand(LOWORD(wParam));
        }
        return 0;
    case WM_NCDESTROY:
        if (pThis) {
            delete pThis;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
        }
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

DetailsView::DetailsView(HWND hWnd) : m_hWnd(hWnd) {}

void DetailsView::SetFonts(HFONT hFontRegular, HFONT hFontBold, HFONT hFontTitle, HFONT hFontCode, HFONT hFontBadge) {
    m_hFontRegular = hFontRegular;
    m_hFontBold = hFontBold;
    m_hFontTitle = hFontTitle;
    m_hFontCode = hFontCode;
    m_hFontBadge = hFontBadge;

    if (m_hBtnToggle) SendMessage(m_hBtnToggle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);
    if (m_hBtnCopy) SendMessage(m_hBtnCopy, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontRegular), TRUE);

    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void DetailsView::SetDarkMode(bool isDark) {
    m_isDark = isDark;
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void DetailsView::SetTweak(const Tweak* pTweak, bool isChecked) {
    const bool hadTweak = m_currentTweak.has_value();
    if (pTweak) {
        m_currentTweak = *pTweak;
    } else {
        m_currentTweak = std::nullopt;
    }
    m_isChecked = isChecked;
    m_scrollY = 0;

    const bool hasTweak = m_currentTweak.has_value();
    if (hasTweak != hadTweak) {
        ShowWindow(m_hBtnToggle, hasTweak ? SW_SHOW : SW_HIDE);
        ShowWindow(m_hBtnCopy, hasTweak ? SW_SHOW : SW_HIDE);
    }
    if (hasTweak) {
        SetCheckedState(isChecked);
    }

    RECT rc{};
    GetClientRect(m_hWnd, &rc);
    OnSize(rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void DetailsView::SetCheckedState(bool isChecked) {
    m_isChecked = isChecked;
    if (m_hBtnToggle) {
        if (m_isChecked) {
            SetWindowTextW(m_hBtnToggle, L"Protected (Active)");
        } else {
            SetWindowTextW(m_hBtnToggle, L"Enable Protection");
        }
    }
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void DetailsView::OnSize(int width, int height) {
    // Header action buttons
    const int btnTop = 10;
    const int copyWidth = 85;
    const int toggleWidth = 140;
    const int gap = 8;
    const int rightMargin = 16;

    if (m_hBtnToggle) {
        MoveWindow(m_hBtnToggle, width - rightMargin - toggleWidth, btnTop, toggleWidth, 26, TRUE);
    }
    if (m_hBtnCopy) {
        MoveWindow(m_hBtnCopy, width - rightMargin - toggleWidth - gap - copyWidth, btnTop, copyWidth, 26, TRUE);
    }

    // Scrollbar calculation
    const int visibleBodyHeight = height - 70;
    const int maxScroll = (std::max)(0, m_totalContentHeight - visibleBodyHeight);
    if (m_scrollY > maxScroll) {
        m_scrollY = maxScroll;
    }

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (m_totalContentHeight > visibleBodyHeight) ? m_totalContentHeight : visibleBodyHeight;
    si.nPage = (visibleBodyHeight > 0) ? visibleBodyHeight : 1;
    si.nPos = m_scrollY;
    SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void DetailsView::OnVScroll(WORD scrollCode, short pos) {
    RECT rc{};
    GetClientRect(m_hWnd, &rc);
    const int visibleBodyHeight = (rc.bottom - rc.top) - 70;
    const int maxScroll = (std::max)(0, m_totalContentHeight - visibleBodyHeight);

    int newScroll = m_scrollY;
    switch (scrollCode) {
    case SB_LINEUP:
        newScroll -= 20;
        break;
    case SB_LINEDOWN:
        newScroll += 20;
        break;
    case SB_PAGEUP:
        newScroll -= visibleBodyHeight / 2;
        break;
    case SB_PAGEDOWN:
        newScroll += visibleBodyHeight / 2;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newScroll = pos;
        break;
    }

    newScroll = (std::clamp)(newScroll, 0, maxScroll);
    if (newScroll != m_scrollY) {
        m_scrollY = newScroll;
        SetScrollPos(m_hWnd, SB_VERT, m_scrollY, TRUE);
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void DetailsView::OnMouseWheel(short delta) {
    RECT rc{};
    GetClientRect(m_hWnd, &rc);
    const int visibleBodyHeight = (rc.bottom - rc.top) - 70;
    const int maxScroll = (std::max)(0, m_totalContentHeight - visibleBodyHeight);

    const int scrollAmount = (delta / WHEEL_DELTA) * 40;
    const int newScroll = (std::clamp)(m_scrollY - scrollAmount, 0, maxScroll);

    if (newScroll != m_scrollY) {
        m_scrollY = newScroll;
        SetScrollPos(m_hWnd, SB_VERT, m_scrollY, TRUE);
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void DetailsView::OnCommand(int id) {
    if (id == IDC_DETAILS_BTN_TOGGLE) {
        if (m_hParent) {
            SendMessage(m_hParent, WM_COMMAND, MAKEWPARAM(IDC_DETAILS_BTN_TOGGLE, 0), reinterpret_cast<LPARAM>(m_hWnd));
        }
    } else if (id == IDC_DETAILS_BTN_COPY) {
        CopyTweakInfo();
    }
}

void DetailsView::CopyTweakInfo() {
    if (!m_currentTweak.has_value()) return;
    const auto& t = m_currentTweak.value();

    std::wstringstream ss;
    ss << L"Tweak: " << t.title << L" [" << std::wstring(t.id.begin(), t.id.end()) << L"]\r\n";
    ss << L"Category: " << t.category << L"\r\n";
    ss << L"Recommendation: ";
    if (t.safety == SafetyLevel::Safe) ss << L"Safe (Recommended for all systems)\r\n";
    else if (t.safety == SafetyLevel::Normal) ss << L"Normal (Disables optional services)\r\n";
    else ss << L"Advanced (Requires caution)\r\n";

    ss << L"\r\nDescription:\r\n" << t.description << L"\r\n";
    ss << L"\r\nPrivacy Impact:\r\n" << t.impact << L"\r\n";

    if (!t.regActions.empty()) {
        ss << L"\r\nRegistry Actions:\r\n";
        for (const auto& ra : t.regActions) {
            std::wstring root = (ra.scope == TargetScope::Machine) ? L"HKLM" : L"HKCU";
            ss << L"  * [" << root << L"\\] " << ra.subKey << L" -> " << ra.valueName << L" = " << ra.dwordProtected << L"\r\n";
        }
    }
    if (!t.serviceActions.empty()) {
        ss << L"\r\nWindows Services:\r\n";
        for (const auto& sa : t.serviceActions) {
            ss << L"  * " << sa.serviceName << L" (Disabled)\r\n";
        }
    }

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

void DetailsView::DrawBadge(HDC hdc, int x, int y, int height, const std::wstring& text, COLORREF bg, COLORREF textCol, COLORREF borderCol, HFONT hFont, int& outWidth) {
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, hFont ? hFont : m_hFontBadge));

    SIZE sz{};
    GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.length()), &sz);
    const int badgeWidth = sz.cx + 20;
    outWidth = badgeWidth;

    HBRUSH hBr = CreateSolidBrush(bg);
    HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
    HBRUSH oldBr = static_cast<HBRUSH>(SelectObject(hdc, hBr));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

    RoundRect(hdc, x, y, x + badgeWidth, y + height, 8, 8);

    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(hBr);
    DeleteObject(hPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);
    RECT rcText{ x, y, x + badgeWidth, y + height };
    DrawTextW(hdc, text.c_str(), -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont);
}

void DetailsView::OnPaint(HDC hdc) {
    RECT rcClient{};
    GetClientRect(m_hWnd, &rcClient);
    const int width = rcClient.right - rcClient.left;
    const int height = rcClient.bottom - rcClient.top;
    if (width <= 0 || height <= 0) return;

    // Double-buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, memBmp));

    // Colors
    const COLORREF colBg = m_isDark ? RGB(32, 32, 34) : RGB(255, 255, 255);
    const COLORREF colHeaderBg = m_isDark ? RGB(38, 40, 44) : RGB(246, 248, 250);
    const COLORREF colBorder = m_isDark ? RGB(55, 58, 64) : RGB(225, 228, 232);
    const COLORREF colTextPrimary = m_isDark ? RGB(240, 240, 245) : RGB(24, 28, 36);
    const COLORREF colTextSecondary = m_isDark ? RGB(160, 165, 175) : RGB(90, 100, 115);
    const COLORREF colSectionHdr = m_isDark ? RGB(130, 150, 180) : RGB(70, 90, 120);

    // 1. Fill entire background
    HBRUSH hBrBg = CreateSolidBrush(colBg);
    FillRect(memDC, &rcClient, hBrBg);
    DeleteObject(hBrBg);

    // 2. Fixed Header (Height: 70px)
    const int headerHeight = 70;
    RECT rcHeader{ 0, 0, width, headerHeight };
    HBRUSH hBrHeader = CreateSolidBrush(colHeaderBg);
    FillRect(memDC, &rcHeader, hBrHeader);
    DeleteObject(hBrHeader);

    // Header bottom border
    HPEN hPenBorder = CreatePen(PS_SOLID, 1, colBorder);
    HPEN oldPen = static_cast<HPEN>(SelectObject(memDC, hPenBorder));
    MoveToEx(memDC, 0, headerHeight - 1, nullptr);
    LineTo(memDC, width, headerHeight - 1);
    SelectObject(memDC, oldPen);
    DeleteObject(hPenBorder);

    if (!m_currentTweak.has_value()) {
        // Empty state prompt
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, colTextSecondary);
        SelectObject(memDC, m_hFontRegular);
        RECT rcPrompt{ 20, 24, width - 20, headerHeight };
        DrawTextW(memDC, L"Select any privacy setting from the list above to view its technical details, safety impact, and registry keys.", -1, &rcPrompt, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    } else {
        const auto& t = m_currentTweak.value();

        // Title
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, colTextPrimary);
        SelectObject(memDC, m_hFontTitle ? m_hFontTitle : m_hFontBold);

        const int maxTitleWidth = width - 260; // Leave room for top-right action buttons
        RECT rcTitle{ 16, 10, 16 + maxTitleWidth, 34 };
        DrawTextW(memDC, t.title.c_str(), -1, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        // Badges Row at Y = 40
        int badgeX = 16;
        const int badgeY = 40;
        const int badgeH = 20;

        // Badge 1: Tweak ID
        std::wstring idStr = L"ID: " + std::wstring(t.id.begin(), t.id.end());
        int idW = 0;
        COLORREF idBg = m_isDark ? RGB(50, 52, 58) : RGB(236, 239, 244);
        COLORREF idText = m_isDark ? RGB(200, 205, 215) : RGB(70, 80, 95);
        COLORREF idBorder = m_isDark ? RGB(70, 75, 85) : RGB(215, 220, 228);
        DrawBadge(memDC, badgeX, badgeY, badgeH, idStr, idBg, idText, idBorder, m_hFontBadge, idW);
        badgeX += idW + 8;

        // Badge 2: Safety Level
        std::wstring safeText = L"Safe (Recommended)";
        COLORREF safeBg = m_isDark ? RGB(22, 55, 30) : RGB(225, 245, 225);
        COLORREF safeTextCol = m_isDark ? RGB(115, 215, 140) : RGB(18, 125, 68);
        COLORREF safeBorder = m_isDark ? RGB(35, 95, 50) : RGB(160, 220, 160);

        if (t.safety == SafetyLevel::Normal) {
            safeText = L"Normal (Disables Optional)";
            safeBg = m_isDark ? RGB(65, 48, 10) : RGB(255, 244, 215);
            safeTextCol = m_isDark ? RGB(245, 185, 55) : RGB(160, 95, 0);
            safeBorder = m_isDark ? RGB(105, 80, 20) : RGB(245, 215, 130);
        } else if (t.safety == SafetyLevel::Advanced) {
            safeText = L"Advanced (Requires Caution)";
            safeBg = m_isDark ? RGB(65, 20, 20) : RGB(255, 230, 230);
            safeTextCol = m_isDark ? RGB(250, 115, 115) : RGB(195, 40, 30);
            safeBorder = m_isDark ? RGB(105, 30, 30) : RGB(245, 170, 170);
        }

        int safeW = 0;
        DrawBadge(memDC, badgeX, badgeY, badgeH, safeText, safeBg, safeTextCol, safeBorder, m_hFontBadge, safeW);
        badgeX += safeW + 8;

        // Badge 3: Scope
        std::wstring scopeText = L"Scope: Machine (HKLM)";
        if (t.scope == TargetScope::User) scopeText = L"Scope: User (HKCU)";
        else if (t.scope == TargetScope::Both) scopeText = L"Scope: Machine & User";
        else if (t.scope == TargetScope::Service) scopeText = L"Scope: Windows Service";

        int scopeW = 0;
        COLORREF scopeBg = m_isDark ? RGB(35, 48, 65) : RGB(232, 240, 252);
        COLORREF scopeTextCol = m_isDark ? RGB(140, 190, 250) : RGB(30, 90, 175);
        COLORREF scopeBorder = m_isDark ? RGB(55, 75, 105) : RGB(190, 215, 245);
        DrawBadge(memDC, badgeX, badgeY, badgeH, scopeText, scopeBg, scopeTextCol, scopeBorder, m_hFontBadge, scopeW);
        badgeX += scopeW + 8;

        // Badge 4: Category
        int catW = 0;
        DrawBadge(memDC, badgeX, badgeY, badgeH, t.category, idBg, idText, idBorder, m_hFontBadge, catW);

        // 3. Scrollable Body
        HRGN hRgnClip = CreateRectRgn(0, headerHeight, width, height);
        SelectClipRgn(memDC, hRgnClip);

        int curY = headerHeight + 12 - m_scrollY;
        const int leftMargin = 20;
        const int contentWidth = width - 40;

        // --- Section: Description ---
        SetTextColor(memDC, colSectionHdr);
        SelectObject(memDC, m_hFontBold ? m_hFontBold : m_hFontRegular);
        RECT rcHdrDesc{ leftMargin, curY, leftMargin + contentWidth, curY + 18 };
        DrawTextW(memDC, L"DESCRIPTION", -1, &rcHdrDesc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        curY += 20;

        SetTextColor(memDC, colTextPrimary);
        SelectObject(memDC, m_hFontRegular);
        RECT rcDescCalc{ leftMargin, curY, leftMargin + contentWidth, curY + 10000 };
        const int descHeight = DrawTextW(memDC, t.description.c_str(), -1, &rcDescCalc, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        curY += descHeight + 16;

        // --- Section: Privacy Impact ---
        SetTextColor(memDC, colSectionHdr);
        SelectObject(memDC, m_hFontBold ? m_hFontBold : m_hFontRegular);
        RECT rcHdrImpact{ leftMargin, curY, leftMargin + contentWidth, curY + 18 };
        DrawTextW(memDC, L"PRIVACY IMPACT & RATIONALE", -1, &rcHdrImpact, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        curY += 20;

        // Card container for impact
        RECT rcImpactCalc{ leftMargin + 10, curY + 6, leftMargin + contentWidth - 10, curY + 10000 };
        SelectObject(memDC, m_hFontRegular);
        const int impactTextHeight = DrawTextW(memDC, t.impact.c_str(), -1, &rcImpactCalc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        const int cardHeight = impactTextHeight + 16;

        HBRUSH hBrImpact = CreateSolidBrush(m_isDark ? RGB(28, 38, 52) : RGB(242, 247, 255));
        HPEN hPenImpact = CreatePen(PS_SOLID, 1, m_isDark ? RGB(45, 65, 90) : RGB(205, 225, 250));
        HBRUSH oldBr2 = static_cast<HBRUSH>(SelectObject(memDC, hBrImpact));
        HPEN oldPen2 = static_cast<HPEN>(SelectObject(memDC, hPenImpact));
        RoundRect(memDC, leftMargin, curY, leftMargin + contentWidth, curY + cardHeight, 6, 6);
        SelectObject(memDC, oldBr2);
        SelectObject(memDC, oldPen2);
        DeleteObject(hBrImpact);
        DeleteObject(hPenImpact);

        RECT rcImpactText{ leftMargin + 10, curY + 8, leftMargin + contentWidth - 10, curY + cardHeight - 8 };
        SetTextColor(memDC, m_isDark ? RGB(180, 210, 250) : RGB(20, 60, 120));
        DrawTextW(memDC, t.impact.c_str(), -1, &rcImpactText, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        curY += cardHeight + 16;

        // --- Section: Technical Specifications (Registry & Services) ---
        if (!t.regActions.empty() || !t.serviceActions.empty()) {
            SetTextColor(memDC, colSectionHdr);
            SelectObject(memDC, m_hFontBold ? m_hFontBold : m_hFontRegular);
            RECT rcHdrTech{ leftMargin, curY, leftMargin + contentWidth, curY + 18 };
            DrawTextW(memDC, L"TECHNICAL SPECIFICATIONS (REGISTRY & SERVICES)", -1, &rcHdrTech, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
            curY += 20;

            std::wstringstream codeStream;
            for (const auto& ra : t.regActions) {
                std::wstring rootStr = (ra.scope == TargetScope::Machine) ? L"HKLM" : L"HKCU";
                codeStream << L"[" << rootStr << L"\\] " << ra.subKey << L"\r\n";
                codeStream << L"    Value: " << ra.valueName << L" = " << ra.dwordProtected << L" (REG_DWORD)\r\n\r\n";
            }
            for (const auto& sa : t.serviceActions) {
                codeStream << L"Windows Service: " << sa.serviceName << L" (Startup type disabled)\r\n\r\n";
            }

            std::wstring codeText = codeStream.str();
            SelectObject(memDC, m_hFontCode ? m_hFontCode : m_hFontRegular);
            RECT rcCodeCalc{ leftMargin + 10, curY + 8, leftMargin + contentWidth - 10, curY + 10000 };
            const int codeTextHeight = DrawTextW(memDC, codeText.c_str(), -1, &rcCodeCalc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            const int codeCardHeight = codeTextHeight + 16;

            HBRUSH hBrCode = CreateSolidBrush(m_isDark ? RGB(24, 26, 30) : RGB(246, 248, 251));
            HPEN hPenCode = CreatePen(PS_SOLID, 1, m_isDark ? RGB(48, 52, 60) : RGB(220, 225, 232));
            HBRUSH oldBr3 = static_cast<HBRUSH>(SelectObject(memDC, hBrCode));
            HPEN oldPen3 = static_cast<HPEN>(SelectObject(memDC, hPenCode));
            RoundRect(memDC, leftMargin, curY, leftMargin + contentWidth, curY + codeCardHeight, 6, 6);
            SelectObject(memDC, oldBr3);
            SelectObject(memDC, oldPen3);
            DeleteObject(hBrCode);
            DeleteObject(hPenCode);

            RECT rcCodeText{ leftMargin + 10, curY + 8, leftMargin + contentWidth - 10, curY + codeCardHeight - 8 };
            SetTextColor(memDC, m_isDark ? RGB(200, 215, 235) : RGB(35, 45, 60));
            DrawTextW(memDC, codeText.c_str(), -1, &rcCodeText, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
            curY += codeCardHeight + 20;
        }

        SelectClipRgn(memDC, nullptr);
        DeleteObject(hRgnClip);

        // Store total calculated content height
        m_totalContentHeight = curY + m_scrollY - headerHeight;
    }

    // 4. BitBlt to display
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

} // namespace PrivatizeWin
