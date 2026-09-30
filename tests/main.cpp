#include "TestFramework.h"
#include <windows.h>

int main() {
    // Ensure console output handles UTF-8 / formatting
    SetConsoleOutputCP(CP_UTF8);
    return PrivatizeWin::Test::TestRegistry::Instance().RunAll();
}
