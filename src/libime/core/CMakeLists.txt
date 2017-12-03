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
target_link_libraries(kenlm PUBLIC Boost::boost)
set_target_properties(kenlm PROPERTIES
                      COMPILE_FLAGS "-fPIC")

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
    )

add_library(IMECore SHARED ${LIBIME_SRCS})
set_target_properties(IMECore
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
                      EXPORT_NAME Core
)
target_include_directories(IMECore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>)

target_link_libraries(IMECore PUBLIC Fcitx5::Utils Boost::boost PRIVATE kenlm)

install(TARGETS IMECore EXPORT LibIMECoreTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIBIME_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime/core")

add_library(LibIME::Core ALIAS IMECore)

ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfigVersion.cmake"
                  SOVERSION 0)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMECoreConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfig.cmake"
                              INSTALL_DESTINATION  "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore"
)

generate_export_header(IMECore BASE_NAME LibIMECore)

install(EXPORT LibIMECoreTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore" FILE LibIMECoreTargets.cmake NAMESPACE LibIME:: )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMECoreConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMECore"
        COMPONENT Devel )
