
find_package(PkgConfig)

find_path(ICU_INCLUDE_DIR
    NAMES unicode/utypes.h
)
mark_as_advanced(ICU_INCLUDE_DIR)
find_library(ICU_LIBRARY
    NAMES icuuc cygicuuc cygicuuc32
)
mark_as_advanced(ICU_LIBRARY)

# Copy the results to the output variables.
if (ICU_INCLUDE_DIR AND ICU_LIBRARY AND EXISTS "${ICU_INCLUDE_DIR}/unicode/uvernum.h")
    set(ICU_VERSION 0)
    set(ICU_MAJOR_VERSION 0)
    set(ICU_MINOR_VERSION 0)
    file(READ "${ICU_INCLUDE_DIR}/unicode/uvernum.h" _ICU_VERSION_CONENTS)
    string(REGEX REPLACE ".*#define U_ICU_VERSION_MAJOR_NUM ([0-9]+).*" "\\1" ICU_MAJOR_VERSION "${_ICU_VERSION_CONENTS}")
    string(REGEX REPLACE ".*#define U_ICU_VERSION_MINOR_NUM ([0-9]+).*" "\\1" ICU_MINOR_VERSION "${_ICU_VERSION_CONENTS}")

    set(ICU_VERSION "${ICU_MAJOR_VERSION}.${ICU_MINOR_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ICU
    FOUND_VAR
        ICU_FOUND
    REQUIRED_VARS
        ICU_LIBRARY
        ICU_INCLUDE_DIR
    VERSION_VAR
        ICU_VERSION
)

if(ICU_FOUND AND NOT TARGET ICU::ICU)
    add_library(ICU::ICU UNKNOWN IMPORTED)
    set_target_properties(ICU::ICU PROPERTIES
        IMPORTED_LOCATION "${ICU_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
endif()

include(FeatureSummary)
set_package_properties(ICU PROPERTIES
    URL "http://www.icu-project.org/"
    DESCRIPTION "International Components for Unicode library"
)
