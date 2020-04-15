set(LLVM_LINK_COMPONENTS
  Support
  )

add_llvm_tool(zld
  zld.cpp
  )
