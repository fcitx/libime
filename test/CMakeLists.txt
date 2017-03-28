set(LIBIME_SINGLE_FILE_TEST
    testtable
    testpinyindata
    testpinyinencoder
    testinputbuffer
    )

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} libime )
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

add_executable(testmodel testmodel.cpp)
target_link_libraries(testmodel libime)
target_compile_definitions(testmodel PRIVATE -DKENLM_MAX_ORDER=3 -DHAVE_ICU)

add_executable(testpinyindictionary testpinyindictionary.cpp)
target_link_libraries(testpinyindictionary libime)

add_executable(testtrie testtrie.cpp)
target_link_libraries(testtrie libime)
add_executable(testdecoder testdecoder.cpp)
target_link_libraries(testdecoder libime)
