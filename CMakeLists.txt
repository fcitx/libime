cmake_minimum_required(VERSION 3.12)
project(libime VERSION 1.1.13)
set(LibIME_VERSION ${PROJECT_VERSION})

set(REQUIRED_FCITX_VERSION 5.1.13)
find_package(ECM 1.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

#########################################
# Options
#########################################
option(BUILD_SHARED_LIBS "Build library as shared libs" On)
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)
set(GCOV_TOOL "gcov" CACHE STRING "Path to gcov tool used by coverage.")
option(ENABLE_DOC "Build doxygen" Off)
option(ENABLE_DATA "Build data" On)
option(ENABLE_TOOLS "Build tools" On)

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
include(CheckLibraryExists)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET "libzstd")

find_package(Fcitx5Utils ${REQUIRED_FCITX_VERSION} REQUIRED CONFIG)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

find_package(Boost 1.61 CONFIG REQUIRED COMPONENTS iostreams)

set(LIBIME_INSTALL_PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/libime")
set(LIBIME_INSTALL_LIBDATADIR "${CMAKE_INSTALL_FULL_LIBDIR}/libime")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)

    if (ENABLE_COVERAGE)
        add_custom_target(coverage
                        COMMAND "${CMAKE_CTEST_COMMAND}"
                        COMMAND lcov --gcov-tool "${GCOV_TOOL}" --no-external --capture --directory ./ -b "${CMAKE_CURRENT_SOURCE_DIR}" --output-file coverage.info
                        COMMAND lcov --remove coverage.info "*/kenlm/*" -o coverage.info
                        COMMAND genhtml coverage.info --output-directory "coverage_pages"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()

add_subdirectory(src)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif()

if (ENABLE_DATA)
  add_subdirectory(data)
endif()

if (ENABLE_DOC)
  find_package(Doxygen REQUIRED)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.codedocs" FCITX_DOXYGEN_CONFIGURATION)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)
  add_custom_target(doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
