cmake_minimum_required(VERSION 3.20)
project(PrivatizeWin LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE WINVER=0x0A00 _WIN32_WINNT=0x0A00)

# Force static CRT linking (/MT in Release, /MTd in Debug) for single standalone executable with 0 dependencies
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Optimization flags for minimal footprint
if(MSVC)
    add_compile_options(
        /W4
        /permissive-
        /utf-8
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Gw>
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

set(SOURCES
    src/main.cpp
    src/core/Types.h
    src/core/SimpleJson.h
    src/core/RegistryHelper.h
    src/core/RegistryHelper.cpp
    src/core/ServiceHelper.h
    src/core/ServiceHelper.cpp
    src/core/UserHiveManager.h
    src/core/UserHiveManager.cpp
    src/core/RestorePoint.h
    src/core/RestorePoint.cpp
    src/core/TaskScheduler.h
    src/core/TaskScheduler.cpp
    src/core/TweakRegistry.h
    src/core/TweakRegistry.cpp
    src/core/TemplateManager.h
    src/core/TemplateManager.cpp
    src/core/Localization.h
    src/core/Localization.cpp
    src/core/PendingHandoff.h
    src/core/PendingHandoff.cpp
    src/core/ProcessHelper.h
    src/cli/CliRunner.h
    src/cli/CliRunner.cpp
    src/ui/DarkMode.h
    src/ui/DarkMode.cpp
    src/ui/ScheduleDialog.h
    src/ui/ScheduleDialog.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    res/resource.h
    res/app.rc
)

# Architecture suffix configuration (_x65 / _x64 for x64, _arm / _arm64 for ARM)
if(NOT DEFINED ARCH_SUFFIX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64|arm64|ARM" OR CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|arm64")
        set(ARCH_SUFFIX "_arm")
    else()
        set(ARCH_SUFFIX "_x65")
    endif()
endif()

add_executable(PrivatizeWin WIN32 ${SOURCES})

if(ARCH_SUFFIX)
    set_target_properties(PrivatizeWin PROPERTIES
        OUTPUT_NAME "PrivatizeWin${ARCH_SUFFIX}"
    )
endif()

# Create architecture aliases so both _x65 and _x64, and _arm and _arm64, as well as PrivatizeWin.exe, always exist
if(ARCH_SUFFIX STREQUAL "_x65" OR ARCH_SUFFIX STREQUAL "_x64")
    add_custom_command(TARGET PrivatizeWin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin_x65.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin_x64.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin.exe"
        COMMENT "Generating PrivatizeWin_x65.exe, PrivatizeWin_x64.exe, and PrivatizeWin.exe"
    )
elseif(ARCH_SUFFIX STREQUAL "_arm" OR ARCH_SUFFIX STREQUAL "_arm64")
    add_custom_command(TARGET PrivatizeWin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin_arm.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin_arm64.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrivatizeWin>"
            "$<TARGET_FILE_DIR:PrivatizeWin>/PrivatizeWin.exe"
        COMMENT "Generating PrivatizeWin_arm.exe, PrivatizeWin_arm64.exe, and PrivatizeWin.exe"
    )
endif()

target_include_directories(PrivatizeWin PRIVATE src)

target_link_libraries(PrivatizeWin PRIVATE
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

# Automated Unit and Integration Test Suite
option(BUILD_TESTING "Build the test suite" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
