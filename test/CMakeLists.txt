set(LIBIME_SINGLE_FILE_TEST
    testtrie
    testtable
    )

foreach(TESTCASE ${LIBIME_SINGLE_FILE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} libime ${ICU_LIBRARIES} ${ICU_IO_LIBRARIES})
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()
