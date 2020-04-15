// Copyright (c) 2020 Daniel Zimmerman

#include "zld.h"

#include "llvm/Object/MachO.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/WithColor.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::zld;

static cl::OptionCategory ZldCat("zld Options");

cl::opt<bool> zld::Dummy("dummy", cl::desc("Dummy arg to sanity check cli"),
                         cl::cat(ZldCat));

static cl::list<std::string> InputFilenames(cl::Positional,
                                            cl::desc("<input object files>"),
                                            cl::ZeroOrMore, cl::cat(ZldCat));

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

LLVM_ATTRIBUTE_NORETURN void reportError(StringRef File, Twine Message) {
  WithColor::error(errs(), ToolName) << "'" << File << "': " << Message << "\n";
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

  void LoadBins(const std::vector<std::string> &Filenames) {
    reportStatus("Loading binaries...");
    Bins.reserve(Filenames.size());
    ObjectFiles.reserve(Filenames.size());
    llvm::for_each(Filenames,
                   [this](StringRef Filename) { this->LoadBin(Filename); });
  }

private:
  void LoadBin(StringRef Filename) {
    OwningBinary<Binary> OBinary =
        unwrapOrError(createBinary(Filename), Filename);
    Binary &Binary = *OBinary.getBinary();
    if (ObjectFile *O = dyn_cast<ObjectFile>(&Binary)) {
      if (MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(O)) {
        ObjectFiles.push_back(MachO);
      } else {
        reportError(Filename, "not a mach-o object file");
      }
    } else {
      reportError(Filename, "not an object file");
    }
    Bins.emplace_back(std::move(OBinary));
    reportStatus("  Loaded " + Filename);
  }

  std::vector<OwningBinary<Binary>> Bins;
  std::vector<MachOObjectFile *> ObjectFiles;
};

} // namespace

int main(int argc, char **argv) {
  using namespace llvm;
  InitLLVM X(argc, argv);

#if 0
  const cl::OptionCategory *OptionFilters[] = {&ZldCat};
  cl::HideUnrelatedOptions(OptionFilters);
#endif

  cl::ParseCommandLineOptions(argc, argv, "novel mach-o linker\n", nullptr,
                              /*EnvVar=*/nullptr,
                              /*LongOptionsUseDoubleDash=*/true);

  // Defaults to a.out if no filenames specified.
  if (InputFilenames.empty()) {
    InputFilenames.push_back("a.out");
  }

  ToolName = argv[0];

  if (Dummy) {
    reportStatus("Passed dummy");
  }

  Context Ctx;
  Ctx.LoadBins(InputFilenames);

  reportStatus("Successfully started up");

  return EXIT_SUCCESS;
}
