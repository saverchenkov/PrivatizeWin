#include <windows.h>
#include <commctrl.h>
#include "core/Types.h"
#include "core/ProcessHelper.h"
#include "cli/CliRunner.h"
#include "ui/MainWindow.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// C++ Core Guidelines:
// R.1: RAII resource management for COM
// ES.20: Always initialize objects

namespace {
class ComApartmentScope {
public:
    explicit ComApartmentScope(DWORD dwCoInit = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) noexcept {
        m_hr = CoInitializeEx(nullptr, dwCoInit);
    }
    ~ComApartmentScope() noexcept {
        if (SUCCEEDED(m_hr)) {
            CoUninitialize();
        }
    }
    ComApartmentScope(const ComApartmentScope&) = delete;
    ComApartmentScope& operator=(const ComApartmentScope&) = delete;
    [[nodiscard]] bool isOk() const noexcept { return SUCCEEDED(m_hr); }
private:
    HRESULT m_hr;
};
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const int argc = __argc;
    wchar_t** const argv = __wargv;

    const auto cliOpts = PrivatizeWin::CliRunner::ParseArguments(argc, argv);
    if (cliOpts.isCli) {
        return PrivatizeWin::CliRunner::Execute(cliOpts);
    }

    // GUI Mode: PrivatizeWin modifies machine-wide registry (HKLM) and Windows Services.
    // If not running as administrator, attempt to prompt the OS for UAC elevation.
    if (!PrivatizeWin::IsRunningAsAdmin()) {
        bool noElevate = false;
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--no-elevate") == 0) {
                noElevate = true;
                break;
            }
        }

        if (!noElevate) {
            if (PrivatizeWin::RelaunchElevated()) {
                return 0; // Elevated child process launched successfully. Exit unelevated parent.
            }
            // User cancelled UAC prompt or elevation failed. Continue in Standard User mode.
        }
    }

    // GUI Mode
    const ComApartmentScope com;

    if (!PrivatizeWin::MainWindow::RegisterClass(hInstance)) {
        MessageBoxW(nullptr, L"Failed to register window class.", L"PrivatizeWin Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hWnd = PrivatizeWin::MainWindow::Create(hInstance);
    if (!hWnd) {
        MessageBoxW(nullptr, L"Failed to create application window.", L"PrivatizeWin Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int showCmd = (nCmdShow == 0 || nCmdShow == SW_HIDE) ? SW_SHOWNORMAL : nCmdShow;
    ShowWindow(hWnd, showCmd);
    UpdateWindow(hWnd);

    const HACCEL hAccel = LoadAcceleratorsW(hInstance, L"MAINMENU");

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!TranslateAcceleratorW(hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

int wmain(int argc, wchar_t* argv[]) {
    const auto cliOpts = PrivatizeWin::CliRunner::ParseArguments(argc, argv);
    if (cliOpts.isCli) {
        return PrivatizeWin::CliRunner::Execute(cliOpts);
    }
    return wWinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_SHOWNORMAL);
}
