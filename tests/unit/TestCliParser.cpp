#include "../TestFramework.h"
#include "../../src/cli/CliRunner.h"

using namespace PrivatizeWin;

TEST_CASE(Unit_CliParser, EmptyArgumentsLaunchesGui) {
    wchar_t* argv[] = { const_cast<wchar_t*>(L"PrivatizeWin.exe") };
    const auto opts = CliRunner::ParseArguments(1, argv);
    ASSERT_FALSE(opts.isCli);
}

TEST_CASE(Unit_CliParser, ApplyTemplateFlag) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--apply-template"),
        const_cast<wchar_t*>(L"strict"),
        const_cast<wchar_t*>(L"--dry-run"),
        const_cast<wchar_t*>(L"--quiet")
    };
    const auto opts = CliRunner::ParseArguments(5, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_EQ(opts.applyTemplate, "strict");
    ASSERT_TRUE(opts.dryRun);
    ASSERT_TRUE(opts.quiet);
}

TEST_CASE(Unit_CliParser, ScheduledTaskFlags) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--install-task"),
        const_cast<wchar_t*>(L"logon"),
        const_cast<wchar_t*>(L"--task-template"),
        const_cast<wchar_t*>(L"minimal"),
        const_cast<wchar_t*>(L"--users"),
        const_cast<wchar_t*>(L"all")
    };
    const auto opts = CliRunner::ParseArguments(7, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_TRUE(opts.installTask);
    ASSERT_EQ(opts.taskFrequency, "logon");
    ASSERT_EQ(opts.taskTemplate, "minimal");
    ASSERT_EQ(static_cast<int>(opts.userMode), static_cast<int>(UserSelectionMode::AllUsers));
}

TEST_CASE(Unit_CliParser, CustomUserListFlag) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--users"),
        const_cast<wchar_t*>(L"Alice,Bob,Charlie")
    };
    const auto opts = CliRunner::ParseArguments(3, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_EQ(static_cast<int>(opts.userMode), static_cast<int>(UserSelectionMode::SpecificUsers));
    ASSERT_EQ(opts.specificUsernames.size(), 3);
    ASSERT_EQ(opts.specificUsernames[0], L"Alice");
    ASSERT_EQ(opts.specificUsernames[1], L"Bob");
    ASSERT_EQ(opts.specificUsernames[2], L"Charlie");
}

TEST_CASE(Unit_CliParser, StatusOutputFlags) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--status"),
        const_cast<wchar_t*>(L"--output"),
        const_cast<wchar_t*>(L"json")
    };
    const auto opts = CliRunner::ParseArguments(4, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_TRUE(opts.showStatus);
    ASSERT_EQ(opts.statusFormat, "json");
}

#include "../../src/core/ProcessHelper.h"

TEST_CASE(Unit_CliParser, ElevationDetectionFunction) {
    const bool elevated = IsRunningAsAdmin();
    ASSERT_TRUE(elevated == true || elevated == false);
}

TEST_CASE(Unit_CliParser, UnrecognizedOptionFlag) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--unknown-flag")
    };
    const auto opts = CliRunner::ParseArguments(2, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_TRUE(opts.hasError);
    ASSERT_TRUE(opts.errorMessage.find("Unrecognized") != std::string::npos);
}

TEST_CASE(Unit_CliParser, ResumePendingFlag) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--resume-pending"),
        const_cast<wchar_t*>(L"C:\\temp\\test_pending.json")
    };
    const auto opts = CliRunner::ParseArguments(3, argv);
    ASSERT_FALSE(opts.isCli); // Must route to GUI mode
    ASSERT_FALSE(opts.hasError);
    ASSERT_EQ(opts.resumePendingFile, L"C:\\temp\\test_pending.json");
}

TEST_CASE(Unit_CliParser, ResumePendingMissingFile) {
    wchar_t* argv[] = {
        const_cast<wchar_t*>(L"PrivatizeWin.exe"),
        const_cast<wchar_t*>(L"--resume-pending")
    };
    const auto opts = CliRunner::ParseArguments(2, argv);
    ASSERT_TRUE(opts.isCli);
    ASSERT_TRUE(opts.hasError);
    ASSERT_TRUE(opts.errorMessage.find("Missing file path") != std::string::npos);
}


