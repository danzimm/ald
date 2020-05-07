// Copyright (c) 2020 Daniel Zimmerman

#include "MachO/Visitor.h"

#include "MachO/File.h"

using namespace llvm::MachO;

namespace llvm {

namespace ald {

namespace MachO {

void LCVisitor::visit(const File &F) {
  auto Hdr = F.getHeader();

  visitHeader(F, Hdr);

  auto LCStart = (const load_command *)((const uint8_t *)Hdr + sizeof(*Hdr));
  auto LCEnd =
      (const load_command *)((const uint8_t *)LCStart + Hdr->sizeofcmds);

  for (auto LCIter = LCStart; LCIter < LCEnd;
       LCIter =
           (const load_command *)((const uint8_t *)LCIter + LCIter->cmdsize)) {
    switch (LCIter->cmd) {

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  case LCValue:                                                                \
    visit_##LCName(F, (const LCStruct *)LCIter);                                \
    break;

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND
    }
  }
}

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  void LCVisitor::visit_##LCName(const File &F, const LCStruct *Cmd) {          \
    visitCmd(F, (const load_command *)Cmd);                                    \
  }

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
