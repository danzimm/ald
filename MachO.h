// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/Triple.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/ObjectYAML/yaml2obj.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm::MachO;

namespace llvm {

namespace zld {

class LoadCommand {
public:
  virtual ~LoadCommand() {}
  virtual load_command *BuildCmd() = 0;
  virtual uint32_t Size() = 0;
};

class MachO {
public:
  class Builder {

    void SetTriple(const Triple &T) { Triple_ = T; }

    void AddLoadCommand(std::unique_ptr<LoadCommand> LC) {
      LoadCommands_.push_back(std::move(LC));
    }

    Expected<std::unique_ptr<MemoryBuffer>> Build() const;

  private:
    uint32_t BuildHeaderFlags() const;
    Expected<mach_header_64> BuildHeader(uint32_t LoadCommandsSize) const;

    Triple Triple_;
    std::vector<std::unique_ptr<LoadCommand>> LoadCommands_;
  };
};

} // end namespace zld

} // end namespace llvm
