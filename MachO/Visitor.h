// Copyright (c) 2020 Daniel Zimmerman

#include "llvm/BinaryFormat/MachO.h"
#include "llvm/Support/MemoryBuffer.h"

namespace llvm {

namespace ald {

namespace MachO {

class File;

class LCVisitor {
public:
  virtual ~LCVisitor() {}

  void visit(const File &F);

protected:
  virtual void visitHeader(const ::llvm::MachO::mach_header_64 *) {}

  // If no implementation of Visit##LCName is found then VisitCmd will be
  // called.
  virtual void visitCmd(const ::llvm::MachO::load_command *) {}

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  virtual void visit##LCName(const ::llvm::MachO::LCStruct *Cmd);

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND
};

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
