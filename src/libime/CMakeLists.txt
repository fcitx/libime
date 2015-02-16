include_directories(kenlm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
set_target_properties(kenlm PROPERTIES COMPILE_FLAGS "-fPIC")

set(LIBIME_SRCS
    datrie.cpp
    tablebaseddictionary.cpp
    )

add_library(libime SHARED ${LIBIME_SRCS})
target_link_libraries(libime kenlm ICU::ICU)

add_executable(libime_slm_build_binary kenlm/lm/build_binary_main.cc)
target_link_libraries(libime_slm_build_binary kenlm ICU::ICU)
