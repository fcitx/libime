ecm_setup_version(${LibIME_VERSION} VARIABLE_PREFIX LIBIME
                                    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/libime_version.h"
                                    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfigVersion.cmake"
                                    SOVERSION 0)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/LibIME")

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMEConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfig.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR})

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMEConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel )

install(EXPORT LibIMETargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE LibIMETargets.cmake NAMESPACE LibIME:: )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libime_version.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Devel )

set(libime_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/kenlm
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

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
add_definitions(-DKENLM_MAX_ORDER=3 -DHAVE_ICU)

add_library(kenlm STATIC ${KENLM_SRCS})
target_include_directories(kenlm PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kenlm>")
set_target_properties(kenlm PROPERTIES COMPILE_FLAGS "-fPIC")

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
    )

set(LIBIME_SRCS
    datrie.cpp
    decoder.cpp
    tablebaseddictionary.cpp
    pinyinencoder.cpp
    pinyindata.cpp
    pinyindictionary.cpp
    languagemodel.cpp
    pinyinime.h
    pinyincontext.cpp
    inputbuffer.cpp
    )

add_library(libime SHARED ${LIBIME_SRCS})
generate_export_header(libime BASE_NAME LIBIME)
add_library(LibIME::LibIME ALIAS libime)

target_include_directories(libime PUBLIC "$<BUILD_INTERFACE:${libime_INCLUDE_DIRS}>")
target_include_directories(libime INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}/libime>")

set_target_properties(libime
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
                      EXPORT_NAME LibIME
)

target_link_libraries(libime PRIVATE kenlm
                             PUBLIC ICU::ICU Fcitx5::Utils)

add_executable(libime_slm_build_binary kenlm/lm/build_binary_main.cc)
target_link_libraries(libime_slm_build_binary kenlm ICU::ICU)


install(TARGETS libime EXPORT LibIMETargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")

install(FILES ${LIBIME_HDRS}
              ${CMAKE_CURRENT_BINARY_DIR}/libime_export.h
              DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libime")

