# logic from setup.py
file(GLOB __KENLM_SRCS kenlm/lm/*.cc kenlm/util/*.cc kenlm/util/double-conversion/*.cc)
set(KENLM_SRCS)
foreach(f ${__KENLM_SRCS})
    string(REGEX MATCH "test\\.cc" IS_TEST ${f})
    string(REGEX MATCH "main\\.cc" IS_MAIN ${f})
    if(NOT IS_TEST AND NOT IS_MAIN)
        set(KENLM_SRCS ${KENLM_SRCS} ${f})
    endif()
endforeach()

add_library(kenlm STATIC ${KENLM_SRCS})
target_include_directories(kenlm PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kenlm>)
target_compile_definitions(kenlm PUBLIC -DKENLM_MAX_ORDER=3 PRIVATE -DNDEBUG)
target_link_libraries(kenlm PUBLIC Boost::boost PkgConfig::ZSTD)
set_target_properties(kenlm PROPERTIES
  CXX_STANDARD 11
  POSITION_INDEPENDENT_CODE ON)

if(UNIX)
  check_library_exists(rt clock_gettime "clock_gettime from librt" HAVE_CLOCKGETTIME_RT)
  if (HAVE_CLOCKGETTIME_RT)
    target_link_libraries(kenlm PUBLIC rt)
  else()
    check_library_exists(c clock_gettime "clock_gettime from the libc" HAVE_CLOCKGETTIME)
  endif()

  if (HAVE_CLOCKGETTIME_RT OR HAVE_CLOCKGETTIME)
    target_compile_definitions(kenlm PRIVATE -DHAVE_CLOCKGETTIME)
  endif()
endif()

set(LIBIME_HDRS
    datrie.h
    decoder.h
    languagemodel.h
    inputbuffer.h
    segmentgraph.h
    lattice.h
    languagemodel.h
    historybigram.h
    dictionary.h
    userlanguagemodel.h
    lrucache.h
    prediction.h
    triedictionary.h
    utils.h
    ${CMAKE_CURRENT_BINARY_DIR}/libimecore_export.h
    )

set(LIBIME_SRCS
    datrie.cpp
    decoder.cpp
    languagemodel.cpp
    inputbuffer.cpp
    lattice.cpp
    userlanguagemodel.cpp
    historybigram.cpp
    segmentgraph.cpp
    utils.cpp
    prediction.cpp
    triedictionary.cpp
    )

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX IMECore
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfigVersion.cmake")
# the cmake if will
set(IMECore_SOVERSION 0)

add_library(IMECore ${LIBIME_SRCS})
set_target_properties(IMECore
                      PROPERTIES VERSION ${IMECore_VERSION}
                      SOVERSION ${IMECore_SOVERSION}
                      EXPORT_NAME Core
)
target_include_directories(IMECore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>)

target_link_libraries(IMECore PUBLIC Fcitx5::Utils Boost::boost PRIVATE kenlm Boost::iostreams PkgConfig::ZSTD)

set(IMECore_INSTALL_TARGETS IMECore)
if (NOT BUILD_SHARED_LIBS)
    list(APPEND IMECore_INSTALL_TARGETS kenlm)
endif()

install(TARGETS ${IMECore_INSTALL_TARGETS} EXPORT LibIMECoreTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib)
install(FILES ${LIBIME_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime/core" COMPONENT header)

add_library(LibIME::Core ALIAS IMECore)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMECoreConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfig.cmake"
                              INSTALL_DESTINATION  "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore"
)

generate_export_header(IMECore BASE_NAME LibIMECore)

install(EXPORT LibIMECoreTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore"
        FILE LibIMECoreTargets.cmake
        NAMESPACE LibIME::
        COMPONENT Devel)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore"
        COMPONENT Devel)
