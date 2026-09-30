#pragma once

#include <windows.h>

namespace PrivatizeWin {

class ScheduleDialog {
public:
    static void Show(HWND hParent);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void UpdateStatus(HWND hDlg);
};

} // namespace PrivatizeWin
