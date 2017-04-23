add_executable(libime_slm_build_binary ../src/libime/kenlm/lm/build_binary_main.cc)
target_link_libraries(libime_slm_build_binary kenlm)

install(TARGETS libime_slm_build_binary DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::slm_build_binary ALIAS libime_slm_build_binary)

add_executable(libime_pinyindict libime_pinyindict.cpp)
target_link_libraries(libime_pinyindict LibIME::LibIME)
install(TARGETS libime_pinyindict DESTINATION ${CMAKE_INSTALL_BINDIR})
add_executable(LibIME::pinyindict ALIAS libime_pinyindict)
