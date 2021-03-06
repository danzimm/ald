// Copyright (c) 2020 Daniel Zimmerman

#include "ald/ald.h"

#include "Aldy/Aldy.h"

#include "MachO/Builder.h"
#include "MachO/File.h"
#include "MachO/Visitor.h"

#include "llvm/Object/MachO.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/WithColor.h"

#include <map>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ald;
using namespace llvm::MachO;
using llvm::ald::MachO::LCSegVisitor;
using llvm::ald::MachO::LCVisitor;

cl::opt<bool> ald::Dummy("dummy", cl::desc("Dummy arg to sanity check cli"));

static cl::opt<std::string>
    OutputFilename("out", cl::desc("Specify the name of the resulting binary"));
static cl::alias OutputFilenameShort("o", cl::desc("Alias for --out"),
                                     cl::NotHidden, cl::Grouping,
                                     cl::aliasopt(OutputFilename));

static cl::list<std::string> InputFilenames(cl::Positional,
                                            cl::desc("<input object files>"),
                                            cl::ZeroOrMore);

static cl::list<std::string> LibrarySearchPaths(
    "L", cl::Prefix, cl::ZeroOrMore,
    cl::desc(
        "Specify additional directories in which to search for libraries"));
static cl::list<std::string> FrameworkSearchPaths(
    "F", cl::Prefix, cl::ZeroOrMore,
    cl::desc(
        "Specify additional directories in which to search for frameworks"));

static cl::list<std::string>
    Libraries("l", cl::Prefix, cl::ZeroOrMore,
              cl::desc("Specify which libraries to link against"));
static cl::list<std::string>
    Frameworks("framework", cl::ZeroOrMore,
               cl::desc("Specify which frameworks to link against"));

static cl::list<std::string>
    Prefixes("syslibroot", cl::ZeroOrMore,
             cl::desc("Specify SDK prefixes to prepend to every search path"));
static cl::opt<bool> DontAddStandardSearchPaths(
    "Z",
    cl::desc(
        "Don't search standard search paths by default (/usr/lib, "
        "/usr/lib/local, /Library/Frameworks/, /System/Library/Frameworks/)"));

static cl::extrahelp
    HelpResponse("\nPass @FILE as argument to read options from FILE.\n");

static StringRef ToolName;

namespace {
#if 0
void reportWarning(Twine Message, StringRef File) {
  // Output order between errs() and outs() matters especially for archive
  // files where the output is per member object.
  outs().flush();
  WithColor::warning(errs(), ToolName)
      << "'" << File << "': " << Message << "\n";
  errs().flush();
}
#endif

void printToolError(Twine Message) {
  WithColor::error(errs(), ToolName) << Message << "\n";
}

LLVM_ATTRIBUTE_NORETURN void reportToolError(Twine Message) {
  printToolError(Message);
  exit(1);
}

LLVM_ATTRIBUTE_NORETURN void reportError(Error E, StringRef FileName,
                                         StringRef ArchiveName = "",
                                         StringRef ArchitectureName = "") {
  assert(E);
  WithColor::error(errs(), ToolName);
  if (ArchiveName != "")
    errs() << ArchiveName << "(" << FileName << ")";
  else
    errs() << "'" << FileName << "'";
  if (!ArchitectureName.empty())
    errs() << " (for architecture " << ArchitectureName << ")";
  std::string Buf;
  raw_string_ostream OS(Buf);
  logAllUnhandledErrors(std::move(E), OS);
  OS.flush();
  errs() << ": " << Buf;
  exit(1);
}

template <typename T, typename... Ts>
T unwrapOrError(Expected<T> EO, Ts &&... Args) {
  if (EO) {
    return std::move(*EO);
  }
  reportError(EO.takeError(), std::forward<Ts>(Args)...);
}

static void reportStatus(Twine Message) {
  WithColor::note(errs(), ToolName) << Message << "\n";
}

#if 0
LLVM_ATTRIBUTE_NORETURN static void reportCmdLineError(Twine Message) {
  WithColor::error(errs(), ToolName) << Message << "\n";
  exit(1);
}
#endif

class Context {
public:
  Context() {}

  void loadFiles(const std::vector<std::string> &Filenames) {
    if (LoadedFiles_.size() != 0) {
      return;
    }

    reportStatus("Loading binaries...");
    LoadedFiles_.reserve(Filenames.size());
    llvm::for_each(Filenames,
                   [this](StringRef Filename) { this->loadFile(Filename); });
    validateLoadedFiles_();
  }

  const Triple &getTriple() const { return Triple_; }

  void visitFiles(LCVisitor &Visitor) {
    llvm::for_each(LoadedFiles_, [&Visitor](const LoadedFile &LF) {
      Visitor.visit(*LF.File);
    });
  }

private:
  void loadFile(StringRef Path) {
    LoadedFiles_.push_back(LoadedFile{
        .Path = Path,
        .File = unwrapOrError(ald::MachO::File::read(Path), Path),
    });
  }

  void validateLoadedFiles_() {
    std::map<Triple::ArchType, const LoadedFile *> triple_map;
    llvm::for_each(LoadedFiles_, [&triple_map](const LoadedFile &LF) {
      triple_map[LF.File->getTriple().getArch()] = &LF;
    });
    if (triple_map.size() != 1) {
      printToolError("Unsure which architecture to link:");
      for (auto &mapping : triple_map) {
        printToolError("  " + Triple::getArchTypePrefix(mapping.first).str() +
                       " (" + mapping.second->Path + ")");
      }
      reportToolError("Please ensure all inputs have the same architecture");
    }
    Triple_ = triple_map.begin()->second->File->getTriple();

    switch (Triple_.getArch()) {
    case Triple::ArchType::aarch64:
    case Triple::ArchType::x86_64:
      reportStatus("Linking binary with architecture '" +
                   Triple_.getArchName() + "'");
      break;
    default:
      reportToolError("Unable to link unsupported architecture '" +
                      Triple::getArchTypePrefix(Triple_.getArch()) + "'");
    }
  }

  struct LoadedFile {
    StringRef Path;
    std::unique_ptr<ald::MachO::File> File;
  };

  std::vector<LoadedFile> LoadedFiles_;
  Triple Triple_;
};

} // namespace

int main(int argc, char **argv) {
  using namespace llvm;
  InitLLVM X(argc, argv);

  cl::ParseCommandLineOptions(argc, argv, "novel mach-o linker\n", nullptr,
                              /*EnvVar=*/nullptr,
                              /*LongOptionsUseDoubleDash=*/false);

  // Defaults to a.out if no filenames specified.
  if (InputFilenames.empty()) {
    InputFilenames.push_back("a.out");
  }
  // Defaults to Input[0] + .bin
  if (OutputFilename.size() == 0) {
    OutputFilename = InputFilenames[0] + ".bin";
  }

  ToolName = argv[0];

  if (Dummy) {
    reportStatus("Passed dummy");
  }

  Aldy Al(Prefixes, LibrarySearchPaths, FrameworkSearchPaths,
          DontAddStandardSearchPaths);

  Context Ctx;
  Ctx.loadFiles(InputFilenames);

  class Printer : public LCSegVisitor {
  public:
    void visitHeader(const ald::MachO::File &F,
                     const mach_header_64 *) override {
      WithColor::remark(outs(), ToolName)
          << F.getPath() << ": Parsing load commands...\n";
    }

    void visitCmd(const ald::MachO::File &F, const load_command *LC) override {
      WithColor::remark(outs(), ToolName)
          << F.getPath() << ":  " << ald::MachO::getLoadCommandName(LC->cmd)
          << '\n';
    }

    void visitSegment(const ald::MachO::File &F,
                      const segment_command_64 *Cmd) override {
      WithColor::remark(outs(), ToolName)
          << F.getPath() << ":  Segment: '" << Cmd->segname << "'\n";
    }

    void visitSection(const ald::MachO::File &F, const segment_command_64 *Cmd,
                      const section_64 *Sect) override {
      auto &S = WithColor::remark(outs(), ToolName)
                << F.getPath() << ":    '" << Sect->sectname << ','
                << Sect->segname << "'";
      if (Cmd->segname[0] != '\0') {
        S << " (" << Cmd->segname << ")";
      }
      S << '\n';
    }
  };

  Printer P;
  Ctx.visitFiles(P);

  reportStatus("Successfully started up, will write to '" + OutputFilename +
               "'");

  ald::MachO::Builder::File FB;
  if (auto Err = FB.setTriple(Ctx.getTriple()).buildAndWrite(OutputFilename)) {
    reportError(std::move(Err), OutputFilename);
  }

  reportStatus("Wrote mach header!");

  return EXIT_SUCCESS;
}
