cmake_minimum_required(VERSION 3.20)
project(MCUDungeonCrawler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build Unit Test Suite" ON)
option(BUILD_MCU "Build for Target Microcontroller" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(NOT BUILD_MCU)
    # =========================================================================
    # DESKTOP SIMULATOR TARGET
    # =========================================================================
    find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

    add_executable(dungeon_sim src/main.cpp)
    target_link_libraries(dungeon_sim PRIVATE sfml-graphics sfml-window sfml-system)

    # =========================================================================
    # UNIT TEST TARGET (Desktop Only)
    # =========================================================================
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()

else()
    # =========================================================================
    # MCU TARGET (e.g. ARM Cortex-M)
    # =========================================================================
    # Excludes SFML, Desktop Main, and Tests entirely from build tree
    add_executable(dungeon_mcu mcu/main_mcu.cpp)
    target_compile_definitions(dungeon_mcu PRIVATE MCU_BUILD)
endif()