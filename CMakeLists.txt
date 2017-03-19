cmake_minimum_required(VERSION 3.7)
set(LibIME_VERSION 0.0.1)
project(libime VERSION ${LibIME_VERSION})

find_package(ECM 1.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#########################################
# Options
#########################################
option(ENABLE_TEST "Build test" On)

#########################################
# Compiler and linker
#########################################
set(CMAKE_C_FLAGS "-Wall -Wextra -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

#########################################
# Dependency
#########################################

include(GNUInstallDirs)
include(FeatureSummary)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePkgConfigFile)

find_package(ICU REQUIRED)
find_package(Boost 1.61 REQUIRED)
find_package(Fcitx5Utils REQUIRED)

include_directories(src)

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(src)
