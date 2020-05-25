// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "ADT/DataTypes.h"

#include "Util/FileSearcher.h"

namespace llvm {

namespace ald {

class Aldy {
public:
  Aldy(const std::vector<std::string> &SDKPrefixes,
       const std::vector<std::string> &LibraryPaths,
       const std::vector<std::string> &FrameworkPaths,
       bool DisableDefaultSearchPaths = false);

private:
  SearchPath LibrarySearchPath;
  SearchPath FrameworkSearchPath;
};

} // end namespace ald

} // end namespace llvm
