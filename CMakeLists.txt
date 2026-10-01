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

add_executable(PrivatizeWin WIN32 ${SOURCES})

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
