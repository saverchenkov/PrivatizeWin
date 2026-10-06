#pragma once

#include <string>
#include <vector>
#include <functional>
#include <windows.h>

namespace PrivatizeWin {

struct PendingStatePlan {
    std::vector<std::string> pendingEnable;
    std::vector<std::string> pendingRevert;
};

class PendingHandoff {
public:
    static constexpr const char* MAGIC_HEADER = "PRIVATIZEWIN_HANDOFF_V1\n";
    static constexpr DWORD MAX_HANDOFF_SIZE = 1024 * 1024; // 1 MB

    // Validates that filePath resides in a safe temporary directory
    [[nodiscard]] static bool IsValidHandoffPath(const std::wstring& filePath);

    // Saves pending plan to a unique, securely created temporary file.
    // Returns full file path, or empty string on any failure.
    [[nodiscard]] static std::wstring SaveHandoff(const PendingStatePlan& plan, std::wstring* outToken = nullptr);

    // Validates format, magic header, JSON structure, reparse point, version, and checks for overlapping IDs.
    // Does NOT delete the file.
    [[nodiscard]] static bool ValidateAndLoadHandoff(const std::wstring& filePath, PendingStatePlan& outPlan, const std::wstring& expectedToken = L"");

    // Consumes handoff: validates and loads plan, and ONLY deletes the file if it was a valid, verified handoff.
    [[nodiscard]] static bool ConsumeHandoff(const std::wstring& filePath, PendingStatePlan& outPlan, const std::wstring& expectedToken = L"");

    // Test hook: called immediately after closing the verified handle during deletion
    using PostCloseCallback = std::function<void(const std::wstring&)>;
    inline static PostCloseCallback s_postCloseCallback = nullptr;
};

} // namespace PrivatizeWin
