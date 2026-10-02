#include "TestFramework.h"
#include <windows.h>
#include <string>

int main(int argc, char* argv[]) {
    // Ensure console output handles UTF-8 / formatting
    SetConsoleOutputCP(CP_UTF8);

    std::string filter;
    if (argc > 1) {
        filter = argv[1];
    }

    return PrivatizeWin::Test::TestRegistry::Instance().Run(filter);
}
