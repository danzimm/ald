// Copyright (c) 2020 Daniel Zimmerman

#include "llvm/ADT/Triple.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/Object/MachO.h"
#include "llvm/Support/MemoryBuffer.h"

namespace llvm {

namespace ald {

namespace MachO {

class File {
public:
  static Expected<std::unique_ptr<File>> read(StringRef Path);

  const ::llvm::MachO::mach_header_64 *getHeader() const { return Hdr_; }

  ::llvm::MachO::HeaderFileType getType() const {
    return (::llvm::MachO::HeaderFileType)getHeader()->filetype;
  }

  const Triple &getTriple() const { return Triple_; }

  size_t loadCommandCount() const { return getHeader()->ncmds; }

  uint32_t getFlags() const { return getHeader()->flags; }

  const MemoryBuffer &getBuffer() const { return *MB_; }

  const char *getFileStart() const { return getBuffer().getBufferStart(); }

  const char *getFileEnd() const { return getBuffer().getBufferEnd(); }

private:
  File(std::unique_ptr<MemoryBuffer> MB, StringRef Path)
      : MB_(std::move(MB)), Path_(Path),
        Hdr_((const ::llvm::MachO::mach_header_64 *)MB_->getBufferStart()),
        Triple_(object::MachOObjectFile::getArchTriple(Hdr_->cputype,
                                                       Hdr_->cpusubtype)) {}

  std::unique_ptr<MemoryBuffer> MB_;
  std::string Path_;
  const ::llvm::MachO::mach_header_64 *Hdr_;
  Triple Triple_;

  friend class LCVisitor;
};

const char *getLoadCommandName(uint32_t LCValue);

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
