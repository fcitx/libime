find_package(PythonInterp 3 REQUIRED)
set(OPENGRAM_LM_TAR "lm_sc.3gm.arpa-20140820.tar.bz2")
# Use our own file server for convenience.
set(OPENGRAM_LM_URL "https://download.fcitx-im.org/data/${OPENGRAM_LM_TAR}")

fcitx5_download(opengram-lm-download ${OPENGRAM_LM_URL} ${OPENGRAM_LM_TAR}
                751bab7c55ea93a2cedfb0fbb7eb09f67d4da9c2c55496e5f31eb8580f1d1e2f)
fcitx5_extract(opengram-lm-extract ${OPENGRAM_LM_TAR} DEPENDS opengram-lm-download
               OUTPUT lm_sc.3gm.arpa)

set(OPENGRAM_LM_SRC "${CMAKE_CURRENT_BINARY_DIR}/kenlm_sc.arpa")
set(OPENGRAM_LM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.lm")
set(OPENGRAM_LM_PREDICT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.lm.predict")
add_custom_command(
  OUTPUT "${OPENGRAM_LM_SRC}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lm_sc.3gm.arpa" ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_arpa.py
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/convert_open_gram_arpa.py "${CMAKE_CURRENT_BINARY_DIR}/lm_sc.3gm.arpa" > "${OPENGRAM_LM_SRC}")
add_custom_command(
  OUTPUT "${OPENGRAM_LM_OUTPUT}"
  DEPENDS "${OPENGRAM_LM_SRC}" LibIME::slm_build_binary
  COMMAND LibIME::slm_build_binary -s -a 22 -q 8 trie "${OPENGRAM_LM_SRC}" "${OPENGRAM_LM_OUTPUT}")
add_custom_target(opengram-lm ALL DEPENDS "${OPENGRAM_LM_OUTPUT}")

add_custom_command(
  OUTPUT "${OPENGRAM_LM_PREDICT_OUTPUT}"
  DEPENDS "${OPENGRAM_LM_SRC}" opengram-lm LibIME::prediction
  COMMAND LibIME::prediction "${OPENGRAM_LM_OUTPUT}" "${OPENGRAM_LM_SRC}" "${OPENGRAM_LM_PREDICT_OUTPUT}")
add_custom_target(opengram-lm-predict ALL DEPENDS "${OPENGRAM_LM_PREDICT_OUTPUT}")

install(FILES "${OPENGRAM_LM_OUTPUT}" RENAME zh_CN.lm DESTINATION "${LIBIME_INSTALL_LIBDATADIR}")
install(FILES "${OPENGRAM_LM_PREDICT_OUTPUT}" RENAME zh_CN.lm.predict DESTINATION "${LIBIME_INSTALL_LIBDATADIR}")

set(OPENGRAM_DICT_TAR "dict.utf8-20210302.tar.xz")
set(OPENGRAM_DICT_URL "https://download.fcitx-im.org/data/${OPENGRAM_DICT_TAR}")

fcitx5_download(opengram-dict-download ${OPENGRAM_DICT_URL} ${OPENGRAM_DICT_TAR}
               cd43f97749f38e65fc1f706b981a3c1991599770268932dbb11b2ab90c087646)
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
install(FILES "${OPENGRAM_DICT_OUTPUT}" DESTINATION "${LIBIME_INSTALL_PKGDATADIR}")

set(TABLE_DICT_URL "https://download.fcitx-im.org/data/table.tar.gz")
set(TABLE_DICT_TAR "table.tar.gz")
set(TABLE_TXT_FILES db.txt erbi.txt qxm.txt wanfeng.txt
                      wbpy.txt wbx.txt zrm.txt cj.txt)
fcitx5_download(table-dict-download ${TABLE_DICT_URL} ${TABLE_DICT_TAR}
                6196053c724125e3ae3d8bd6b2f9172d0c83b65b0d410d3cde63b7a8d6ab87b7)
fcitx5_extract(table-dict-extract ${TABLE_DICT_TAR} DEPENDS table-dict-download
               OUTPUT ${TABLE_TXT_FILES})

set(TABLE_DICT_FILES)
foreach(TABLE_TXT_FILE ${TABLE_TXT_FILES})
  string(REPLACE .txt .main.dict TABLE_DICT_FILE ${TABLE_TXT_FILE})
  add_custom_command(OUTPUT ${TABLE_DICT_FILE}
                     DEPENDS ${TABLE_TXT_FILE} LibIME::tabledict
                     COMMAND LibIME::tabledict ${TABLE_TXT_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${TABLE_DICT_FILE})
  list(APPEND TABLE_DICT_FILES ${CMAKE_CURRENT_BINARY_DIR}/${TABLE_DICT_FILE})
endforeach()

add_custom_target(table-dict ALL DEPENDS ${TABLE_DICT_FILES})
install(FILES ${TABLE_DICT_FILES} DESTINATION "${LIBIME_INSTALL_PKGDATADIR}")
