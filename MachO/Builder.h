// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/Triple.h"
#include "llvm/BinaryFormat/MachO.h"

namespace llvm {

namespace ald {

namespace MachO {

namespace Builder {

class LoadCommand {
public:
  virtual ~LoadCommand() {}
  virtual ::llvm::MachO::load_command *build() = 0;
  virtual uint32_t size() = 0;
};

class File {
public:
  File &setTriple(const Triple &T) {
    Triple_ = T;
    return *this;
  }

  File &addLoadCommand(std::unique_ptr<LoadCommand> LC) {
    LoadCommands_.push_back(std::move(LC));
    return *this;
  }

  Error buildAndWrite(std::string Filename) const;

private:
  uint32_t buildHeaderFlags() const;
  Expected<::llvm::MachO::mach_header_64>
  buildHeader(uint32_t LoadCommandsSize) const;

  Triple Triple_;
  std::vector<std::unique_ptr<LoadCommand>> LoadCommands_;
};

} // end namespace Builder

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
