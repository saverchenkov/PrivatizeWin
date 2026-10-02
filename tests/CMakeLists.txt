set(TEST_SOURCES
    main.cpp
    unit/TestSimpleJson.cpp
    unit/TestSmartHandle.cpp
    unit/TestCliParser.cpp
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
    ../src/cli/CliRunner.cpp
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

add_test(NAME PrivatizeWinTestSuite COMMAND PrivatizeWin_Tests)
