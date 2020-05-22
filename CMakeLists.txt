set(ALD_MAIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ALD_SOURCE_DIR ${ALD_MAIN_SRC_DIR})
set(ALD_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_llvm_component_library(LLVMAldy
  MachO/Builder.cpp
  MachO/File.cpp
  MachO/Visitor.cpp
  Util/FileSearcher.cpp
  )

target_compile_options(LLVMAldy PUBLIC "-Wno-c99-extensions")

set(LLVM_LINK_COMPONENTS
  Aldy
  Object
  Support
  )

add_llvm_tool(ald
  ald/ald.cpp
  )

target_compile_options(ald PUBLIC "-Wno-c99-extensions")

add_subdirectory(unittests)
add_subdirectory(test)
