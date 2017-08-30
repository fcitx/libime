add_executable(libime_slm_build_binary ../src/libime/core/kenlm/lm/build_binary_main.cc)
target_link_libraries(libime_slm_build_binary kenlm)

install(TARGETS libime_slm_build_binary DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::slm_build_binary ALIAS libime_slm_build_binary)

add_executable(libime_pinyindict libime_pinyindict.cpp)
target_link_libraries(libime_pinyindict LibIME::Pinyin)
install(TARGETS libime_pinyindict DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::pinyindict ALIAS libime_pinyindict)

add_executable(libime_history libime_history.cpp)
target_link_libraries(libime_history LibIME::Core)
install(TARGETS libime_history DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::history ALIAS libime_history)

add_executable(libime_tabledict libime_tabledict.cpp)
target_link_libraries(libime_tabledict LibIME::Table)
install(TARGETS libime_tabledict DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::tabledict ALIAS libime_tabledict)
