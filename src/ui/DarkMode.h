#pragma once

#include <windows.h>

namespace PrivatizeWin {

class DarkMode {
public:
    static void Initialize();
    static bool IsDarkModeActive();
    static bool IsHighContrastActive();
    static void ApplyToWindow(HWND hWnd);
    static void ApplyToControl(HWND hCtrl);

private:
    static bool s_isDark;
    static bool s_isHighContrast;
    static bool s_initialized;
};

} // namespace PrivatizeWin
