// Copyright (c) 2020 Daniel Zimmerman

#include "MachO/File.h"

#include "llvm/Support/Error.h"

using namespace llvm::MachO;

namespace llvm {

namespace ald {

namespace MachO {

class MachOLoadError : public ErrorInfo<MachOLoadError> {
public:
  enum Kind {
    BAD_MAGIC,
  };

  MachOLoadError(Kind K, uint32_t P) : K_(K), P_(P) {}

  Kind getKind() { return K_; }

  void log(raw_ostream &OS) const override {
    switch (K_) {
    case BAD_MAGIC:
      OS << "Invalid magic (" << format_hex(P_, 10)
         << ") when loading 64 bit MachO file";
      break;
    }
  }

  std::error_code convertToErrorCode() const override {
    llvm_unreachable("Converting MachOLoadError to error_code unsupported");
  }

  static char ID;

private:
  Kind K_;
  uint32_t P_;
};
char MachOLoadError::ID;

Expected<std::unique_ptr<File>> File::create(std::unique_ptr<MemoryBuffer> MB) {
  auto *H = (const mach_header_64 *)MB->getBufferStart();
  if (H->magic != MH_MAGIC_64) {
    return make_error<MachOLoadError>(MachOLoadError::BAD_MAGIC, H->magic);
  }
  return std::unique_ptr<File>(new File(std::move(MB)));
}

const char *getLoadCommandName(uint32_t LCValue) {
  switch (LCValue) {
#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  case LCValue:                                                                \
    return #LCName;

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND
  default:
    return "UNKNOWN_CMD";
  }
}

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
