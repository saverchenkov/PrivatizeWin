#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <cstdint>

namespace PrivatizeWin {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]
// R.1: RAII resource management

class RestorePoint {
public:
    [[nodiscard]] static bool Create(std::wstring_view description, int64_t& outSequenceNumber);
    static bool Cancel(int64_t sequenceNumber);
    [[nodiscard]] static bool IsSystemRestoreAvailable() noexcept;
};

} // namespace PrivatizeWin
