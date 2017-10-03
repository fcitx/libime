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
    )

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} LibIME::Core LibIME::Pinyin LibIME::Table )
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

add_executable(testmodel testmodel.cpp)
target_link_libraries(testmodel LibIME::Core)

add_executable(testpinyinime testpinyinime.cpp)
target_link_libraries(testpinyinime LibIME::Pinyin)

add_executable(testpinyindictionary testpinyindictionary.cpp)
target_link_libraries(testpinyindictionary LibIME::Pinyin)
add_executable(testpinyincontext testpinyincontext.cpp)
target_link_libraries(testpinyincontext LibIME::Pinyin)

add_executable(triebench triebench.cpp)
target_link_libraries(triebench LibIME::Core)
add_executable(testdecoder testdecoder.cpp)
target_link_libraries(testdecoder LibIME::Pinyin)

add_executable(testtableime testtableime.cpp)
target_link_libraries(testtableime LibIME::Table)
