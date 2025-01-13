set(LM_TAR "lm_sc.arpa-20250113.tar.zst")
# Use our own file server for convenience.
set(LM_URL "https://download.fcitx-im.org/data/${LM_TAR}")

fcitx5_download(lm-download ${LM_URL} ${LM_TAR}
                ee83ecf20d52e8bccdba4cf6cd57183d53c257713a5eb77ee3a63d50fc3796dd)
fcitx5_extract(lm-extract ${LM_TAR} DEPENDS lm-download
               OUTPUT lm_sc.arpa)

set(LM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lm_sc.arpa")
set(LM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.lm")
set(LM_PREDICT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.lm.predict")
add_custom_command(
  OUTPUT "${LM_OUTPUT}"
  DEPENDS "${LM_SRC}" LibIME::slm_build_binary
  COMMAND LibIME::slm_build_binary -s -a 22 -q 8 trie "${LM_SRC}" "${LM_OUTPUT}")
add_custom_target(lm ALL DEPENDS "${LM_OUTPUT}")

add_custom_command(
  OUTPUT "${LM_PREDICT_OUTPUT}"
  DEPENDS "${LM_SRC}" lm LibIME::prediction
  COMMAND LibIME::prediction "${LM_OUTPUT}" "${LM_SRC}" "${LM_PREDICT_OUTPUT}")
add_custom_target(lm-predict ALL DEPENDS "${LM_PREDICT_OUTPUT}")

install(FILES "${LM_OUTPUT}" RENAME zh_CN.lm DESTINATION "${LIBIME_INSTALL_LIBDATADIR}")
install(FILES "${LM_PREDICT_OUTPUT}" RENAME zh_CN.lm.predict DESTINATION "${LIBIME_INSTALL_LIBDATADIR}")

set(DICT_TAR "dict-20241001.tar.zst")
set(DICT_URL "https://download.fcitx-im.org/data/${DICT_TAR}")

fcitx5_download(dict-download ${DICT_URL} ${DICT_TAR}
               d3313b88a68620a23bc8d5f9564cf1b678848dc7af541c4f5f02198e5a3686bf)
fcitx5_extract(dict-extract ${DICT_TAR} DEPENDS dict-download
               OUTPUT dict_sc.txt dict_extb.txt)

set(DICT_SRC "${CMAKE_CURRENT_BINARY_DIR}/dict_sc.txt")
set(DICT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sc.dict")
add_custom_command(
  OUTPUT "${DICT_OUTPUT}"
  DEPENDS "${DICT_SRC}" LibIME::pinyindict
  COMMAND LibIME::pinyindict "${DICT_SRC}" "${DICT_OUTPUT}")

set(DICT_EXTB_SRC "${CMAKE_CURRENT_BINARY_DIR}/dict_extb.txt")
set(DICT_EXTB_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/extb.dict")
add_custom_command(
  OUTPUT "${DICT_EXTB_OUTPUT}"
  DEPENDS "${DICT_EXTB_SRC}" LibIME::pinyindict
  COMMAND LibIME::pinyindict "${DICT_EXTB_SRC}" "${DICT_EXTB_OUTPUT}")
add_custom_target(dict ALL DEPENDS "${DICT_OUTPUT}" "${DICT_EXTB_OUTPUT}")
install(FILES "${DICT_OUTPUT}" "${DICT_EXTB_OUTPUT}" DESTINATION "${LIBIME_INSTALL_PKGDATADIR}")

set(TABLE_DICT_TAR "table-20240108.tar.zst")
set(TABLE_DICT_URL "https://download.fcitx-im.org/data/${TABLE_DICT_TAR}")
set(TABLE_TXT_FILES db.txt erbi.txt qxm.txt wanfeng.txt
                      wbpy.txt wbx.txt zrm.txt cj.txt)
fcitx5_download(table-dict-download ${TABLE_DICT_URL} ${TABLE_DICT_TAR}
                3e9d87b04a393f131723472c8eaa860dd23c378a3d4f6a9005513b2a95b3614b)
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
