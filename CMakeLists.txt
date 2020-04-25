set(LLVM_LINK_COMPONENTS
  Object
  Support
  )

add_llvm_tool(zld
  zld.cpp
  MachO.cpp
  )
