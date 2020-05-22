set(ALD_TEST_DEPENDS AldUnitTests LLVMAldy)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.cfg.py
  PATHS
  "ALD_SOURCE_DIR"
  "ALD_BINARY_DIR"
  )

add_lit_testsuites(ALD ${CMAKE_CURRENT_SOURCE_DIR} "" DEPENDS ${ALD_TEST_DEPENDS})
