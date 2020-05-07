set(LLVM_LINK_COMPONENTS
  Object
  Support
  )

add_llvm_tool(ald
  ald.cpp
  MachO/Builder.cpp
  MachO/File.cpp
  MachO/Visitor.cpp
  )

target_compile_options(ald PUBLIC "-Wno-c99-extensions")
