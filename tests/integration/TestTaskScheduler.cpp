#include "../TestFramework.h"
#include "../../src/core/TaskScheduler.h"

using namespace PrivatizeWin;

TEST_CASE(Integration_TaskScheduler, QueryNonExistentTask) {
    const bool installed = TaskScheduler::IsTaskInstalled(L"PrivatizeWin_NonExistent_Test_Task_9876");
    ASSERT_FALSE(installed);
}

TEST_CASE(Integration_TaskScheduler, UninstallNonExistentTaskIdempotent) {
    const bool ok = TaskScheduler::UninstallTask(L"PrivatizeWin_NonExistent_Test_Task_9876");
    // Idempotent uninstall should succeed without error
    ASSERT_TRUE(ok);
}
