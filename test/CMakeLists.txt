set(LIBIME_SINGLE_FILE_TEST
    testtable
    testpinyindata
    testpinyinencoder
    )

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} libime )
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

add_executable(testmodel testmodel.cpp)
target_link_libraries(testmodel kenlm ICU::ICU)
target_compile_definitions(testmodel PRIVATE -DKENLM_MAX_ORDER=3 -DHAVE_ICU)

add_executable(testpinyindictionary testpinyindictionary.cpp)
target_link_libraries(testpinyindictionary libime)
