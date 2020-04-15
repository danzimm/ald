// Copyright (c) 2020 Daniel Zimmerman

#include "zld.h"

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/WithColor.h"

using namespace llvm;
using namespace llvm::zld;

static cl::OptionCategory ZldCat("zld Options");

cl::opt<bool> zld::Dummy("dummy", cl::desc("Dummy arg to sanity check cli"),
                         cl::cat(ZldCat));
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

LLVM_ATTRIBUTE_NORETURN void reportError(StringRef File, Twine Message) {
  WithColor::error(errs(), ToolName) << "'" << File << "': " << Message << "\n";
  exit(1);
}
#endif

static void reportCmdLineWarning(Twine Message) {
  WithColor::warning(errs(), ToolName) << Message << "\n";
}

#if 0
LLVM_ATTRIBUTE_NORETURN static void reportCmdLineError(Twine Message) {
  WithColor::error(errs(), ToolName) << Message << "\n";
  exit(1);
}
#endif

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

  ToolName = argv[0];

  if (Dummy) {
    reportCmdLineWarning("Passed dummy");
  }

  reportCmdLineWarning("Successfully started up");

  return EXIT_SUCCESS;
}
