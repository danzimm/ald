// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

namespace ald {

using file_t = int;
using Path = SmallString<256>;
using PathList = SmallVector<Path, 8>;

} // end namespace ald

} // end namespace llvm
