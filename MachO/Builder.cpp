// Copyright (c) 2020 Daniel Zimmerman

#include "MachO/Builder.h"

//#include "llvm/ObjectYAML/yaml2obj.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm::MachO;

namespace llvm {

namespace ald {

namespace MachO {

namespace Builder {

#define CheckErrMove(x)                                                        \
  if (auto Err = x##OrErr.takeError()) {                                       \
    return std::move(Err);                                                     \
  }                                                                            \
  auto &x = *x##OrErr

#define CheckErr(x)                                                            \
  if (auto Err = x##OrErr.takeError()) {                                       \
    return Err;                                                                \
  }                                                                            \
  auto &x = *x##OrErr

uint32_t File::buildHeaderFlags() const {
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

Expected<mach_header_64> File::buildHeader(uint32_t LoadCommandsSize) const {
  mach_header_64 hdr;
  hdr.magic = MH_MAGIC_64;

  auto CPUTypeOrErr = getCPUType(Triple_);
  CheckErrMove(CPUType);
  hdr.cputype = CPUType;

  auto CPUSubTypeOrErr = getCPUSubType(Triple_);
  CheckErrMove(CPUSubType);
  hdr.cpusubtype = CPUSubType;

  hdr.filetype = MH_EXECUTE;
  hdr.ncmds = LoadCommands_.size();
  hdr.sizeofcmds = LoadCommandsSize;

  hdr.flags = buildHeaderFlags();
  hdr.reserved = 0;

  return hdr;
}

namespace {

Error createFile(const std::string &Filename, size_t Size) {
  using namespace sys::fs;
  Expected<file_t> FOrErr =
      openNativeFileForWrite(Filename, CD_CreateAlways, OF_None, 0755);
  CheckErr(F);
  if (auto Err = errorCodeToError(resize_file(F, Size))) {
    return Err;
  }
  if (auto Err = errorCodeToError(closeFile(F))) {
    return Err;
  }
  return Error::success();
}

} // namespace

Error File::buildAndWrite(std::string Filename) const {
  uint32_t LoadCommandsSize = 0;

  auto HeaderOrErr = buildHeader(LoadCommandsSize);
  CheckErr(Header);

  size_t TotalSize = sizeof(mach_header_64);

  if (auto Err = createFile(Filename, TotalSize)) {
    return Err;
  }

  auto BufferOrErr =
      errorOrToExpected(WriteThroughMemoryBuffer::getFile(Filename, TotalSize));
  CheckErr(Buffer);
  memcpy((void *)Buffer->getBufferStart(), &Header, sizeof(mach_header_64));

  return Error::success();
}

} // end namespace Builder

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
