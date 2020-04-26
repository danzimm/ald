// Copyright (c) 2020 Daniel Zimmerman

#include "zld.h"

#include "llvm/Object/MachO.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/WithColor.h"

#include <map>

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

void printToolError(Twine Message) {
  WithColor::error(errs(), ToolName) << Message << "\n";
}

LLVM_ATTRIBUTE_NORETURN void reportToolError(Twine Message) {
  printToolError(Message);
  exit(1);
}

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

  void loadFiles(const std::vector<std::string> &Filenames) {
    reportStatus("Loading binaries...");
    LoadedFiles_.reserve(Filenames.size());
    llvm::for_each(Filenames,
                   [this](StringRef Filename) { this->loadFile(Filename); });
    validateLoadedFiles_();
  }

private:
  void loadFile(StringRef Filename) {
    OwningBinary<Binary> OBinary =
        unwrapOrError(createBinary(Filename), Filename);
    Binary &Binary = *OBinary.getBinary();
    if (ObjectFile *O = dyn_cast<ObjectFile>(&Binary)) {
      if (MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(O)) {
        LoadedFiles_.push_back(LoadedFile{
            .Name = Filename,
            .Bin = std::move(OBinary),
            .ObjectFile = MachO,
        });
        reportStatus("  Loaded " + Filename);
      } else {
        reportError(Filename, "not a mach-o object file");
      }
    } else {
      reportError(Filename, "not an object file");
    }
  }

  void validateLoadedFiles_() {
    std::map<Triple::ArchType, const LoadedFile *> triple_map;
    llvm::for_each(LoadedFiles_, [&triple_map](const LoadedFile &LF) {
      triple_map[LF.ObjectFile->getArch()] = &LF;
    });
    if (triple_map.size() != 1) {
      printToolError("Unsure which architecture to link:");
      for (auto &mapping : triple_map) {
        printToolError("  " + Triple::getArchTypePrefix(mapping.first).str() +
                       " (" + mapping.second->Name + ")");
      }
      reportToolError("Please ensure all inputs have the same architecture");
    }
    Triple_ = triple_map.begin()->second->ObjectFile->makeTriple();

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
    StringRef Name;
    OwningBinary<Binary> Bin;
    MachOObjectFile *ObjectFile;
  };

  std::vector<LoadedFile> LoadedFiles_;
  Triple Triple_;
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
  Ctx.loadFiles(InputFilenames);

  reportStatus("Successfully started up");

  return EXIT_SUCCESS;
}
