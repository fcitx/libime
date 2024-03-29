configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(LIBIME_SINGLE_FILE_TEST
    testtable
    testpinyindata
    testpinyinencoder
    testinputbuffer
    testhistorybigram
    testshuangpinprofile
    testtrie
    testautophrasedict
    testtablerule
    )

if (ENABLE_DATA)
    list(APPEND LIBIME_SINGLE_FILE_TEST
         testpinyinime_unit
         testdecoder
         testpinyincontext
         testpinyindictionary
         testprediction
         testpinyinprediction
         testtableime_unit
         )
endif()

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} LibIME::Core LibIME::Pinyin LibIME::Table )
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

if (ENABLE_DATA)
    add_dependencies(testpinyinime_unit lm)
    add_dependencies(testdecoder dict lm)
    add_dependencies(testpinyincontext lm)
    add_dependencies(testpinyindictionary dict)
    add_dependencies(testprediction lm)
    add_dependencies(testpinyinprediction lm)
    add_dependencies(testtableime_unit lm)
endif()

add_executable(triebench triebench.cpp)
target_link_libraries(triebench LibIME::Core)

add_executable(testmodel testmodel.cpp)
target_link_libraries(testmodel LibIME::Core)

add_executable(testusermodel testusermodel.cpp)
target_link_libraries(testusermodel LibIME::Core)

add_executable(testpinyinime testpinyinime.cpp)
target_link_libraries(testpinyinime LibIME::Pinyin)

add_executable(testtableime testtableime.cpp)
target_link_libraries(testtableime LibIME::Table)
