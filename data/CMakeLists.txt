find_package(PythonInterp 3 REQUIRED)
set(OPENGRAM_LM_TAR "lm_sc.3gm.arpa-20140820.tar.bz2")
set(OPENGRAM_LM_URL "http://downloads.sourceforge.net/project/open-gram/${OPENGRAM_LM_TAR}")

fcitx5_download(opengram-lm-download ${OPENGRAM_LM_URL} ${OPENGRAM_LM_TAR}
                751bab7c55ea93a2cedfb0fbb7eb09f67d4da9c2c55496e5f31eb8580f1d1e2f)
fcitx5_extract(opengram-lm-extract ${OPENGRAM_LM_TAR} DEPENDS opengram-lm-download
               OUTPUT lm_sc.3gm.arpa)

set(OPENGRAM_LM_SRC "${CMAKE_CURRENT_BINARY_DIR}/kenlm_sc.arpa")
set(OPENGRAM_LM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.lm")
add_custom_command(
  OUTPUT "${OPENGRAM_LM_SRC}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lm_sc.3gm.arpa" ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_arpa.py
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_arpa.py "${CMAKE_CURRENT_BINARY_DIR}/lm_sc.3gm.arpa" > "${OPENGRAM_LM_SRC}")
add_custom_command(
  OUTPUT "${OPENGRAM_LM_OUTPUT}"
  DEPENDS "${OPENGRAM_LM_SRC}" LibIME::slm_build_binary
  COMMAND LibIME::slm_build_binary -s trie -a 22 -q 8 "${OPENGRAM_LM_SRC}" "${OPENGRAM_LM_OUTPUT}")
add_custom_target(opengram-lm ALL DEPENDS "${OPENGRAM_LM_OUTPUT}")
install(FILES "${OPENGRAM_LM_OUTPUT}" DESTINATION "${CMAKE_INSTALL_DATADIR}/libime")

set(OPENGRAM_DICT_TAR "dict.utf8-20170423.tar.xz")
set(OPENGRAM_DICT_URL "https://download.fcitx-im.org/data/dict.utf8-20170423.tar.xz")

fcitx5_download(opengram-dict-download ${OPENGRAM_DICT_URL} ${OPENGRAM_DICT_TAR}
                15775a25c9b4e02891e935e0b0dc692ee6d5a747079e7239c1a71976db90f54e)
fcitx5_extract(opengram-dict-extract ${OPENGRAM_DICT_TAR} DEPENDS opengram-dict-download
               OUTPUT dict.utf8)
set(OPENGRAM_DICT_SRC "${CMAKE_CURRENT_BINARY_DIR}/dict.converted")
set(OPENGRAM_DICT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.dict")
add_custom_command(
  OUTPUT "${OPENGRAM_DICT_SRC}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dict.utf8" ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_dict.py
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_dict.py "${CMAKE_CURRENT_BINARY_DIR}/dict.utf8" > "${OPENGRAM_DICT_SRC}")
add_custom_command(
  OUTPUT "${OPENGRAM_DICT_OUTPUT}"
  DEPENDS "${OPENGRAM_DICT_SRC}" LibIME::pinyindict
  COMMAND LibIME::pinyindict "${OPENGRAM_DICT_SRC}" "${OPENGRAM_DICT_OUTPUT}")
add_custom_target(opengram-dict ALL DEPENDS "${OPENGRAM_DICT_OUTPUT}")
install(FILES "${OPENGRAM_DICT_OUTPUT}" DESTINATION "${CMAKE_INSTALL_DATADIR}/libime")
