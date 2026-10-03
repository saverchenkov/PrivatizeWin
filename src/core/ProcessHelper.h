#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <string_view>

namespace PrivatizeWin {

// C++ Core Guidelines:
// I.10: Use [[nodiscard]]
// R.1: RAII resource management

[[nodiscard]] inline bool IsRunningAsAdmin() noexcept {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD dwSize = sizeof(TOKEN_ELEVATION);
    const BOOL ok = GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize);
    CloseHandle(hToken);

    return (ok && elevation.TokenIsElevated != 0);
}

inline bool RelaunchElevated(HWND hWnd = nullptr, std::wstring_view extraArgs = L"") noexcept {
    wchar_t szPath[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, szPath, MAX_PATH) == 0) {
        return false;
    }

    std::wstring params(extraArgs);

    SHELLEXECUTEINFOW sei{ sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = szPath;
    sei.lpParameters = params.empty() ? nullptr : params.c_str();
    sei.hwnd = hWnd;
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteExW(&sei) != FALSE;
}

} // namespace PrivatizeWin
