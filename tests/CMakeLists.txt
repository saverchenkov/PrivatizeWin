set(TEST_SOURCES
    main.cpp
    unit/TestSimpleJson.cpp
    unit/TestSmartHandle.cpp
    unit/TestCliParser.cpp
    unit/TestPendingState.cpp
    unit/TestTemplateManager.cpp
    unit/TestTweakRegistry.cpp
    integration/TestRegistryHelperLive.cpp
    integration/TestServiceHelperLive.cpp
    integration/TestUserHiveDiscovery.cpp
    integration/TestTaskScheduler.cpp
    integration/TestCliEndToEnd.cpp
    ../src/core/RegistryHelper.cpp
    ../src/core/ServiceHelper.cpp
    ../src/core/UserHiveManager.cpp
    ../src/core/RestorePoint.cpp
    ../src/core/TaskScheduler.cpp
    ../src/core/TweakRegistry.cpp
    ../src/core/TemplateManager.cpp
    ../src/core/Localization.cpp
    ../src/core/PendingHandoff.cpp
    ../src/cli/CliRunner.cpp
    unit/TestLocalization.cpp
)

add_executable(PrivatizeWin_Tests ${TEST_SOURCES})

target_include_directories(PrivatizeWin_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(PrivatizeWin_Tests PRIVATE
    comctl32
    dwmapi
    uxtheme
    taskschd
    comsupp
    advapi32
    shell32
    ole32
    oleaut32
)

add_dependencies(PrivatizeWin_Tests PrivatizeWin)
target_compile_definitions(PrivatizeWin_Tests PRIVATE
    PRIVATIZEWIN_EXE_PATH="$<TARGET_FILE:PrivatizeWin>"
    PRIVATIZEWIN_ENABLE_TEST_HOOKS=1
)

# Granular suite targets for isolated CI reporting
add_test(NAME Unit_SimpleJson COMMAND PrivatizeWin_Tests Unit_SimpleJson)
add_test(NAME Unit_SmartHandle COMMAND PrivatizeWin_Tests Unit_SmartHandle)
add_test(NAME Unit_CliParser COMMAND PrivatizeWin_Tests Unit_CliParser)
add_test(NAME Unit_PendingState COMMAND PrivatizeWin_Tests Unit_PendingState)
add_test(NAME Unit_TemplateManager COMMAND PrivatizeWin_Tests Unit_TemplateManager)
add_test(NAME Unit_TweakRegistry COMMAND PrivatizeWin_Tests Unit_TweakRegistry)
add_test(NAME Integration_RegistryHelper COMMAND PrivatizeWin_Tests Integration_RegistryHelper)
add_test(NAME Integration_ServiceHelper COMMAND PrivatizeWin_Tests Integration_ServiceHelper)
add_test(NAME Integration_UserHiveManager COMMAND PrivatizeWin_Tests Integration_UserHiveManager)
add_test(NAME Integration_TaskScheduler COMMAND PrivatizeWin_Tests Integration_TaskScheduler)
add_test(NAME Integration_CliEndToEnd COMMAND PrivatizeWin_Tests Integration_CliEndToEnd)
add_test(NAME Unit_Localization COMMAND PrivatizeWin_Tests Unit_Localization)
