#pragma once

#include <string>
#include <vector>
#include "core/Types.h"

namespace PrivatizeWin {

// C++ Core Guidelines:
// I.10: Use [[nodiscard]]
// ES.48: Avoid casts

class CliRunner {
public:
    [[nodiscard]] static CliOptions ParseArguments(int argc, wchar_t* argv[]);
    static int Execute(const CliOptions& options);
    static void PrintHelp() noexcept;
};

} // namespace PrivatizeWin
