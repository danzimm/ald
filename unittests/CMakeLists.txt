add_custom_target(AldUnitTests)
set_target_properties(AldUnitTests PROPERTIES FOLDER "AldUnitTests")

function(add_ald_unittest test_dirname)
  add_unittest(AldUnitTests ${test_dirname} ${ARGN})

  include_directories(${ALD_MAIN_SRC_DIR})
endfunction()

add_subdirectory(filesearcher)
add_subdirectory(lazy)
add_subdirectory(uniquefunc)
