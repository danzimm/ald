set(LLVM_LINK_COMPONENTS
  Object
  Support
  )

add_llvm_tool(ald
  ald.cpp
  Macho/Builder.cpp
  )

target_compile_options(ald PUBLIC "-Wno-c99-extensions")
