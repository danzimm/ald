// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/Triple.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/ObjectYAML/yaml2obj.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm::MachO;

namespace llvm {

namespace ald {

class LoadCommand {
public:
  virtual ~LoadCommand() {}
  virtual load_command *buildCmd() = 0;
  virtual uint32_t size() = 0;
};

class MachO {
public:
  class Builder {
  public:
    Builder &setTriple(const Triple &T) {
      Triple_ = T;
      return *this;
    }

    Builder &addLoadCommand(std::unique_ptr<LoadCommand> LC) {
      LoadCommands_.push_back(std::move(LC));
      return *this;
    }

    Error buildAndWrite(std::string Filename) const;

  private:
    uint32_t buildHeaderFlags() const;
    Expected<mach_header_64> buildHeader(uint32_t LoadCommandsSize) const;

    Triple Triple_;
    std::vector<std::unique_ptr<LoadCommand>> LoadCommands_;
  };
};

} // end namespace ald

} // end namespace llvm
