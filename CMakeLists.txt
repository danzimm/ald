set(LLVM_LINK_COMPONENTS
  Object
  Support
  )

add_llvm_tool(zld
  zld.cpp
  MachO.cpp
  )

target_compile_options(zld PUBLIC "-Wno-c99-extensions")
