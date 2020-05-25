// Copyright (c) 2020 Daniel Zimmerman

#include "Aldy/Aldy.h"

#include "llvm/Support/FileSystem.h"

namespace llvm {

namespace ald {

Aldy::Aldy(const std::vector<std::string> &SDKPrefixes,
           const std::vector<std::string> &LibraryPaths,
           const std::vector<std::string> &FrameworkPaths,
           bool DisableDefaultSearchPaths)
    : LibrarySearchPath(
          SDKPrefixes, LibraryPaths,
          DisableDefaultSearchPaths
              ? SmallVector<StringRef, 2>{}
              : SmallVector<StringRef, 2>{"/usr/lib", "/usr/local/lib"}),
      FrameworkSearchPath(
          SDKPrefixes, FrameworkPaths,
          DisableDefaultSearchPaths
              ? SmallVector<StringRef, 2>{}
              : SmallVector<StringRef, 2>{"/Library/Frameworks",
                                          "/System/Library/Frameworks"}) {}

} // end namespace ald

} // end namespace llvm
