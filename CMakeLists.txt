cmake_minimum_required(VERSION 3.7)
set(LibIME_VERSION 0.0.1)
project(libime VERSION ${LibIME_VERSION})

find_package(ECM 1.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

#########################################
# Options
#########################################
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)

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
include(ECMUninstallTarget)
include(FcitxCompilerSettings)

find_package(Boost 1.61 REQUIRED)
find_package(Fcitx5Utils REQUIRED)
set(LIBIME_INSTALL_PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/libime")

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)

    if (ENABLE_COVERAGE)
        add_custom_target(coverage
                        COMMAND "${CMAKE_CTEST_COMMAND}"
                        COMMAND lcov --no-external --capture --directory ./ -b "${CMAKE_CURRENT_SOURCE_DIR}" --output-file coverage.info
                        COMMAND lcov --remove coverage.info "*/kenlm/*" -o coverage.info
                        COMMAND genhtml coverage.info --output-directory "coverage_pages"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(data)
