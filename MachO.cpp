// Copyright (c) 2020 Daniel Zimmerman

#include "MachO.h"

#include "llvm/Support/FileSystem.h"

namespace llvm {

namespace ald {

#define CheckErr(x)                                                            \
  if (auto Err = x##OrErr.takeError()) {                                       \
    return std::move(Err);                                                     \
  }                                                                            \
  auto &x = *x##OrErr

uint32_t MachO::Builder::buildHeaderFlags() const {
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
MachO::Builder::buildHeader(uint32_t LoadCommandsSize) const {
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

  hdr.flags = buildHeaderFlags();
  hdr.reserved = 0;

  return hdr;
}

namespace {

Error touchFile(const std::string &Filename) {
  using namespace sys::fs;
  Expected<file_t> FOrErr =
      openNativeFileForWrite(Filename, CD_CreateAlways, OF_None, 0755);
  if (auto Err = FOrErr.takeError()) {
    return Err;
  }
  if (auto Err = errorCodeToError(closeFile(*FOrErr))) {
    return Err;
  }
  return Error::success();
}

} // namespace

Expected<std::unique_ptr<MemoryBuffer>>
MachO::Builder::build(std::string Filename) const {
  uint32_t LoadCommandsSize = 0;

  auto HeaderOrErr = buildHeader(LoadCommandsSize);
  CheckErr(Header);

  size_t TotalSize = sizeof(mach_header_64);

  if (auto Err = touchFile(Filename)) {
    return std::move(Err);
  }

  auto ResultOrErr =
      errorOrToExpected(WriteThroughMemoryBuffer::getFile(Filename, TotalSize));
  if (auto Err = ResultOrErr.takeError()) {
    return std::move(Err);
  }
  std::unique_ptr<MemoryBuffer> Result(ResultOrErr->release());
  memcpy((void *)Result->getBufferStart(), &Header, sizeof(mach_header_64));

  return std::move(Result);
}

} // end namespace ald

} // end namespace llvm
