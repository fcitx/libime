set(LIBIME_TABLE_HDRS
    tablebaseddictionary.h
    tablecontext.h
    tableoptions.h
    tablerule.h
    tabledecoder.h
    autophrasedict.h
    ${CMAKE_CURRENT_BINARY_DIR}/libimetable_export.h
    )

set(LIBIME_TABLE_SRCS
    tablebaseddictionary.cpp
    tablecontext.cpp
    tableoptions.cpp
    tabledecoder.cpp
    autophrasedict.cpp
    log.cpp
    )

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX IMETable
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMETableConfigVersion.cmake")
set(IMETable_SOVERSION 0)

add_library(IMETable SHARED ${LIBIME_TABLE_SRCS})
set_target_properties(IMETable
                      PROPERTIES VERSION ${IMETable_VERSION}
                      SOVERSION ${IMETable_SOVERSION}
                      EXPORT_NAME Table
)

target_include_directories(IMETable PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>)

target_link_libraries(IMETable PUBLIC Fcitx5::Utils Boost::boost LibIME::Core)

install(TARGETS IMETable EXPORT LibIMETableTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIBIME_TABLE_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime/table" COMPONENT header)

add_library(LibIME::Table ALIAS IMETable)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibIMETableConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibIMETableConfig.cmake"
                              INSTALL_DESTINATION  "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMETable"
)

generate_export_header(IMETable BASE_NAME LibIMETable)

install(EXPORT LibIMETableTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMETable" FILE LibIMETableTargets.cmake NAMESPACE LibIME:: )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibIMETableConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibIMETableConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LibIMETable"
        COMPONENT Devel )


