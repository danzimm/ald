// Copyright (c) 2020 Daniel Zimmerman

#include "llvm/BinaryFormat/MachO.h"
#include "llvm/Support/MemoryBuffer.h"

#include "ADT/Lazy.h"

namespace llvm {

namespace ald {

namespace MachO {

class File;

class LCVisitor {
public:
  virtual ~LCVisitor() {}

  void visit(const File &F);

protected:
  virtual void visitHeader(const File &,
                           const ::llvm::MachO::mach_header_64 *) {}

  // If no implementation of Visit##LCName is found then VisitCmd will be
  // called.
  virtual void visitCmd(const File &, const ::llvm::MachO::load_command *) {}

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  virtual void visit_##LCName(const File &, const ::llvm::MachO::LCStruct *Cmd);

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND
};

class LCSegVisitor : public LCVisitor {
protected:
  void visit_LC_SEGMENT_64(const File &F,
                           const llvm::MachO::segment_command_64 *Cmd) override;

  virtual void visitSegment(const File &F,
                            const llvm::MachO::segment_command_64 *) {}
  virtual void visitSection(const File &F, const llvm::MachO::section_64 *) {}
};

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
