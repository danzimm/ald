// Copyright (c) 2020 Daniel Zimmerman

#include "Util/FileSearcher.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

namespace llvm {

namespace ald {

namespace {

PathList processSDKPrefixes(const std::vector<std::string> &SDKPrefixes,
                            const std::unique_ptr<Path> &Cwd) {
  PathList RV;
  if (!SDKPrefixes.empty()) {
    for (const std::string &Prefix : SDKPrefixes) {
      RV.emplace_back(Prefix);
      auto &ProcessedPrefix = RV.back();
      if (Cwd != nullptr) {
        sys::fs::make_absolute(*Cwd, ProcessedPrefix);
      } else {
        sys::fs::make_absolute(ProcessedPrefix);
      }
      if (ProcessedPrefix.back() == '/') {
        ProcessedPrefix.pop_back();
      }
    }
  } else {
    RV.push_back(Path(""));
  }
  return RV;
}

} // namespace

SearchPath::SearchPath(const std::vector<std::string> &SDKPrefixes,
                       const std::vector<std::string> &Paths,
                       const SmallVector<StringRef, 2> &DefaultPaths,
                       std::unique_ptr<Path> Cwd)
    : SDKPrefixes_(processSDKPrefixes(SDKPrefixes, Cwd)) {
  if (Cwd == nullptr) {
    sys::fs::current_path(Cwd_);
  } else {
    Cwd_.assign(*Cwd);
  }
  if (Cwd_.back() != '/') {
    Cwd_.push_back('/');
  }

  for (const std::string &Path : Paths) {
    Paths_.push_back(Path);
    if (Path.front() == '/') {
      NumAbsolute_ += 1;
    }
  }
  for (StringRef Path : DefaultPaths) {
    Paths_.push_back(Path.str());
    if (Path.front() == '/') {
      NumAbsolute_ += 1;
    }
  }
}

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
  RV.reserve(SearchPath_.size());
  SearchPath_.visit([&RV](const Twine &File) {
    RV.emplace_back();
    File.toVector(RV.back());
    return false;
  });
  return RV;
}

Expected<Path> FileSearcherImpl::searchInternal(StringRef File) const {
  Path RV;

  auto Visitor = [&](const Twine &Dir) -> bool {
    return searchForFileInDirectory(File, Dir, RV) && validatePath(RV);
  };

  if (SearchPath_.visit(Visitor)) {
    return std::move(RV);
  }

  return make_error<FileNotFoundInSearchPath>(File, getAllPaths());
}

bool FileSearcherImpl::searchForFileInDirectory(StringRef File,
                                                const Twine &Dir,
                                                Path &Result) {
  Result.clear();
  Dir.toVector(Result);
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

} // end namespace details

} // end namespace ald

} // end namespace llvm
