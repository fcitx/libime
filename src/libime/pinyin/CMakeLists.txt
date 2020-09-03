set(LIBIME_PINYIN_HDRS
    pinyincontext.h
    pinyindata.h
    pinyindecoder.h
    pinyindictionary.h
    pinyinencoder.h
    pinyinime.h
    pinyinmatchstate.h
    pinyinmatchstate_p.h
    shuangpindata.h
    shuangpinprofile.h
    ${CMAKE_CURRENT_BINARY_DIR}/libimepinyin_export.h
    )

set(LIBIME_PINYIN_SRCS
    pinyincontext.cpp
    pinyindata.cpp
    pinyindecoder.cpp
    pinyindictionary.cpp
    pinyinencoder.cpp
    pinyinime.cpp
    pinyinmatchstate.cpp
    shuangpinprofile.cpp
    )

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX IMEPinyin
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMEPinyinConfigVersion.cmake"
                  SOVERSION 0)

add_library(IMEPinyin SHARED ${LIBIME_PINYIN_SRCS})
set_target_properties(IMEPinyin PROPERTIES
                      VERSION ${IMEPinyin_VERSION}
                      SOVERSION ${IMEPinyin_SOVERSION}
                      EXPORT_NAME Pinyin
)
target_include_directories(IMEPinyin PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>)

target_link_libraries(IMEPinyin PUBLIC Fcitx5::Utils Boost::boost LibIME::Core)

install(TARGETS IMEPinyin EXPORT LibIMEPinyinTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIBIME_PINYIN_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime/pinyin")

add_library(LibIME::Pinyin ALIAS IMEPinyin)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMEPinyinConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMEPinyinConfig.cmake"
                              INSTALL_DESTINATION  "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMEPinyin"
)

generate_export_header(IMEPinyin BASE_NAME LibIMEPinyin)

install(EXPORT LibIMEPinyinTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMEPinyin" FILE LibIMEPinyinTargets.cmake NAMESPACE LibIME:: )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMEPinyinConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMEPinyinConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMEPinyin"
        COMPONENT Devel )

