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
target_include_directories(kenlm PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kenlm>")
target_compile_definitions(kenlm PUBLIC -DKENLM_MAX_ORDER=3 PRIVATE -DNDEBUG)
target_link_libraries(kenlm PUBLIC Boost::boost)
set_target_properties(kenlm PROPERTIES
                      COMPILE_FLAGS "-fPIC")

set(LIBIME_HDRS
    datrie.h
    tablebaseddictionary.h
    tablerule.h
    pinyinencoder.h
    pinyindata.h
    pinyindictionary.h
    languagemodel.h
    pinyinime.h
    pinyincontext.h
    inputbuffer.h
    segmentgraph.h
    lattice.h
    pinyindecoder.h
    languagemodel.h
    historybigram.h
    dictionary.h
    userlanguagemodel.h
    ${CMAKE_CURRENT_BINARY_DIR}/libime_export.h
    )

set(LIBIME_SRCS
    datrie.cpp
    decoder.cpp
    tablebaseddictionary.cpp
    pinyinencoder.cpp
    pinyindata.cpp
    pinyindictionary.cpp
    languagemodel.cpp
    pinyinime.cpp
    pinyincontext.cpp
    inputbuffer.cpp
    lattice.cpp
    pinyindecoder.cpp
    userlanguagemodel.cpp
    historybigram.cpp
    segmentgraph.cpp
    )

add_library(ime SHARED ${LIBIME_SRCS})
set_target_properties(ime
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
                      EXPORT_NAME LibIME
)
target_include_directories(ime PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>")

target_link_libraries(ime PRIVATE kenlm
                          PUBLIC Fcitx5::Utils Boost::boost)

install(TARGETS ime EXPORT LibIMETargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIBIME_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime")

add_library(LibIME::LibIME ALIAS ime)
generate_export_header(ime BASE_NAME LIBIME)
ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfigVersion.cmake"
                  SOVERSION 0)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMEConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfig.cmake"
                              INSTALL_DESTINATION  "${CMAKE_INSTALL_LIBDIR}/cmake/LibIME")

install(EXPORT LibIMETargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIME" FILE LibIMETargets.cmake NAMESPACE LibIME:: )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIME"
        COMPONENT Devel )
