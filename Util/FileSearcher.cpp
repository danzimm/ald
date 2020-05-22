// Copyright (c) 2020 Daniel Zimmerman

#include "Util/FileSearcher.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

namespace llvm {

namespace ald {

namespace details {

namespace {

class FileNotFoundInSearchPath : public ErrorInfo<FileNotFoundInSearchPath> {
public:
  static char ID;

  SmallString<256> File;
  SmallVector<SmallString<256>, 8> Paths;

  FileNotFoundInSearchPath(Twine Filename, SmallVector<SmallString<256>, 8> PS)
      : File(Filename.str()), Paths(std::move(PS)) {}

  void log(raw_ostream &OS) const override {
    OS << File << " not found in { ";
    if (!Paths.empty()) {
      auto Iter = Paths.begin();
      auto End = Paths.end();
      OS << *Iter++;
      while (Iter != End) {
        OS << ", " << *Iter++;
      }
      OS << " ";
    }
    OS << "}";
  }

  std::error_code convertToErrorCode() const override {
    llvm_unreachable("Converting FileNotFoundInSearchPath to error_code"
                     " not supported");
  }
};
char FileNotFoundInSearchPath::ID;
} // namespace

PathList FileSearcherImpl::getAllPaths() const {
  PathList RV;
  RV.reserve(DefaultPaths_.size() + ExtraPaths_.size());
  RV.insert(RV.end(), ExtraPaths_.begin(), ExtraPaths_.end());
  RV.insert(RV.end(), DefaultPaths_.begin(), DefaultPaths_.end());
  return RV;
}

Expected<Path> FileSearcherImpl::searchInternal(StringRef File,
                                                StringRef Prefix,
                                                StringRef Suffix) const {
  Path RV;

  Path Filename;
  Filename.reserve(File.size() + Prefix.size() + Suffix.size());
  Filename += Prefix;
  Filename += File;
  Filename += Suffix;

  auto Visitor = [&](const Path &P) -> bool {
    return searchForFileInDirectory(Filename, P, RV) && validatePath(RV);
  };

  for (const Path &P : ExtraPaths_) {
    if (Visitor(P)) {
      return std::move(RV);
    }
  }

  for (const Path &P : DefaultPaths_) {
    if (Visitor(P)) {
      return std::move(RV);
    }
  }

  return make_error<FileNotFoundInSearchPath>(Filename, getAllPaths());
}

bool FileSearcherImpl::searchForFileInDirectory(StringRef File, StringRef Dir,
                                                Path &Result) {
  Result = Dir;
  sys::path::append(Result, File);
  if (!sys::fs::exists(Result)) {
    return false;
  }
  return true;
}

bool FileSearcherImpl::validatePath(StringRef File) const {
  bool RV = false;
  if (auto FD = sys::fs::openNativeFileForRead(File)) {
    RV = validateFile(*FD);
    sys::fs::closeFile(*FD);
  }
  return RV;
}

Path FileSearcherImpl::processPath(const Twine &P) {
  Path RV;
  if (sys::path::is_absolute(P)) {
    P.toVector(RV);
  } else {
    sys::fs::make_absolute(P, RV);
  }
  return RV;
}

} // end namespace details

} // end namespace ald

} // end namespace llvm
