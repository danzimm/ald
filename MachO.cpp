// Copyright (c) 2020 Daniel Zimmerman

#include "MachO.h"

namespace llvm {

namespace zld {

#define CheckErr(x)                                                            \
  if (auto Err = x##OrErr.takeError()) {                                       \
    return std::move(Err);                                                     \
  }                                                                            \
  auto x = *x##OrErr

uint32_t MachO::Builder::BuildHeaderFlags() const {
  uint32_t result = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;

#if 0
  if (HasWeakExports()) {
    result |= MH_WEAK_DEFINES;
  }
  if (HasWeakImports()) {
    result |= MH_BINDS_TO_WEAK;
  }
  if (HasTLVDescriptors()) {
    result |= MH_HAS_TLV_DESCRIPTORS;
  }
#endif

  return result;
}

Expected<mach_header_64>
MachO::Builder::BuildHeader(uint32_t LoadCommandsSize) const {
  mach_header_64 hdr;
  hdr.magic = MH_MAGIC_64;

  auto CPUTypeOrErr = getCPUType(Triple_);
  CheckErr(CPUType);
  hdr.cputype = CPUType;

  auto CPUSubTypeOrErr = getCPUSubType(Triple_);
  CheckErr(CPUSubType);
  hdr.cpusubtype = CPUSubType;

  hdr.filetype = MH_EXECUTE;
  hdr.ncmds = LoadCommands_.size();
  hdr.sizeofcmds = LoadCommandsSize;

  hdr.flags = BuildHeaderFlags();
  hdr.reserved = 0;

  return hdr;
}

Expected<std::unique_ptr<MemoryBuffer>> MachO::Builder::Build() const {
  uint32_t LoadCommandsSize = 0;

  auto HeaderOrErr = BuildHeader(LoadCommandsSize);
  CheckErr(Header);

  size_t TotalSize = sizeof(struct mach_header_64);

  std::unique_ptr<WritableMemoryBuffer> Result =
      WritableMemoryBuffer::getNewUninitMemBuffer(TotalSize);

  return std::move(Result);
}

} // end namespace zld

} // end namespace llvm