#include "DarkMode.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include "../core/RegistryHelper.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace PrivatizeWin {

bool DarkMode::s_isDark = false;
bool DarkMode::s_isHighContrast = false;
bool DarkMode::s_initialized = false;

void DarkMode::Initialize() {
    if (s_initialized) return;
    s_initialized = true;

    // Check system High Contrast accessibility mode first
    HIGHCONTRASTW hc{ sizeof(hc) };
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) && (hc.dwFlags & HCF_HIGHCONTRASTON)) {
        s_isHighContrast = true;
        s_isDark = false;
        return;
    }

    // Check registry for AppsUseLightTheme
    auto lightVal = RegistryHelper::ReadDword(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme"
    );

    s_isDark = lightVal.has_value() && (lightVal.value() == 0);
}

bool DarkMode::IsDarkModeActive() {
    if (!s_initialized) Initialize();
    return s_isDark && !s_isHighContrast;
}

bool DarkMode::IsHighContrastActive() {
    if (!s_initialized) Initialize();
    return s_isHighContrast;
}

void DarkMode::ApplyToWindow(HWND hWnd) {
    if (!s_initialized) Initialize();
    if (!hWnd || s_isHighContrast) return;

    BOOL useDark = s_isDark ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
}

void DarkMode::ApplyToControl(HWND hCtrl) {
    if (!s_initialized) Initialize();
    if (!hCtrl || s_isHighContrast) return;

    if (s_isDark) {
        SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
    } else {
        SetWindowTheme(hCtrl, L"Explorer", nullptr);
    }
}

} // namespace PrivatizeWin
