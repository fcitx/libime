set(LIBIME_TABLE_HDRS
    tablebaseddictionary.h
    tablecontext.h
    tableime.h
    tableoptions.h
    tableparser.h
    tablerule.h
    ${CMAKE_CURRENT_BINARY_DIR}/libimetable_export.h
    )

set(LIBIME_TABLE_SRCS
    tablebaseddictionary.cpp
    tablecontext.cpp
    tableime.cpp
    tableoptions.cpp
    tableparser.cpp
    )

add_library(IMETable SHARED ${LIBIME_TABLE_SRCS})
set_target_properties(IMETable
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
                      EXPORT_NAME Table
)
target_include_directories(IMETable PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..;${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/LibIME>")

target_link_libraries(IMETable PRIVATE kenlm
                              PUBLIC Fcitx5::Utils Boost::boost LibIME::Core)

install(TARGETS IMETable EXPORT LibIMETableTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIBIME_TABLE_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LibIME/libime/table")

add_library(LibIME::Table ALIAS IMETable)

ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibIMETableConfigVersion.cmake"
                  SOVERSION 0)

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


