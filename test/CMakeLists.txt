set(LIBIME_SINGLE_FILE_TEST
    testtable
    testpinyindata
    testpinyinencoder
    testinputbuffer
    testhistorybigram
    testshuangpinprofile
    )

# enable assert for tests
add_definitions(-UNDEBUG)

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} LibIME::LibIME )
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

add_executable(testmodel testmodel.cpp)
target_link_libraries(testmodel LibIME::LibIME)

add_executable(testpinyinime testpinyinime.cpp)
target_link_libraries(testpinyinime LibIME::LibIME)

add_executable(testpinyindictionary testpinyindictionary.cpp)
target_link_libraries(testpinyindictionary LibIME::LibIME)
add_executable(testpinyincontext testpinyincontext.cpp)
target_link_libraries(testpinyincontext LibIME::LibIME)

add_executable(testtrie testtrie.cpp)
target_link_libraries(testtrie LibIME::LibIME)
add_executable(testdecoder testdecoder.cpp)
target_link_libraries(testdecoder LibIME::LibIME)

add_executable(testtableime testtableime.cpp)
target_link_libraries(testtableime LibIME::LibIME)
