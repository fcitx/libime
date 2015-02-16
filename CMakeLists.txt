cmake_minimum_required(VERSION 2.8)
project(libime)

find_package(ECM 0.0.11 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" SUPPORT_CXX11)

if(NOT SUPPORT_CXX11)
    message(FATAL_ERROR "need c++ 11 compatible compiler to compile")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

#########################################
# Options
#########################################
option(ENABLE_TEST "Build test" On)

#########################################
# Compiler and linker
#########################################
set(CMAKE_C_FLAGS "-Wall -Wextra -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

#########################################
# Dependency
#########################################
find_package(ICU REQUIRED)
find_package(Boost REQUIRED)

include_directories(${ICU_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(src)
