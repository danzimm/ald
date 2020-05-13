// Copyright (c) 2020 Daniel Zimmerman

#include "llvm/BinaryFormat/MachO.h"
#include "llvm/Support/MemoryBuffer.h"

#include "ADT/Lazy.h"

namespace llvm {

namespace ald {

namespace MachO {

class File;

/// This class contains the logic for parsing and visiting load commands in a
/// MachO file. Each load command has a visitor callback of the form
/// \c visit_${Load_Command_Name}. For example to if you wanted to run code
/// when the LC_SYMTAB command is found then you would create a derived class
/// that overrides \c visit_LC_SYMTAB(const File&, const symtab_command*).
class LCVisitor {
public:
  virtual ~LCVisitor() {}

  /// This is the entrypoint to parsing.
  void visit(const File &F);

protected:
  /// Upon visiting a new file the header will also be visited. This allows
  /// a callback to setup any structures/data it may need to setup for a given
  /// file (almost like an initializer for a given file).
  virtual void visitHeader(const File &,
                           const ::llvm::MachO::mach_header_64 *) {}

  /// By default every \c visit_${LCName} callback will redirect back to this
  /// function. This enables default handling of load commands. This is useful
  /// e.g. if you want to warn the user about an unsupported load command.
  virtual void visitCmd(const File &, const ::llvm::MachO::load_command *) {}

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  virtual void visit_##LCName(const File &, const ::llvm::MachO::LCStruct *Cmd);

#include "llvm/BinaryFormat/MachO.def"

#undef HANDLE_LOAD_COMMAND
};

/// This is a derived class of \c LCVisitor which also parses out each Segment's
/// sections. This class provides visitor callbacks for each Segment and
/// Section.
class LCSegVisitor : public LCVisitor {
protected:
  /// The function containing all the logic for parsing the sections of a given
  /// segment.
  void visit_LC_SEGMENT_64(const File &F,
                           const llvm::MachO::segment_command_64 *Cmd) final;

  /// In order to enable derived classes to visit a segment command still, this
  /// class exposes an additional callback for visiting a segment (with a more
  /// convenient name).
  virtual void visitSegment(const File &F,
                            const llvm::MachO::segment_command_64 *) {}

  /// The visitor callback for each section.
  virtual void visitSection(const File &F,
                            const llvm::MachO::segment_command_64 *,
                            const llvm::MachO::section_64 *) {}
};

} // end namespace MachO

} // end namespace ald

} // end namespace llvm
