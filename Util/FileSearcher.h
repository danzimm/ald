// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

namespace ald {

using file_t = int;
using Path = SmallString<256>;
using PathList = SmallVector<Path, 8>;
using ConcatPathList = std::initializer_list<Twine>;

namespace details {

class FileSearcherImpl {
public:
  explicit FileSearcherImpl(const ConcatPathList &DefaultPaths = {})
      : DefaultPaths_(processPaths(DefaultPaths)) {}

  virtual ~FileSearcherImpl() {}

  /// Adds a new user defined \c Path to the list of extra search directories.
  void addDirectory(const Twine &Dir) {
    ExtraPaths_.emplace_back(processPath(Dir));
  }

  /// Returns all the directories that will be searched in the order that they
  /// will be searched.
  PathList getAllPaths() const;

  virtual Expected<Path> search(StringRef File) const {
    return searchInternal(File);
  }

protected:
  Expected<Path> searchInternal(StringRef File) const;

  /// Search for \c File in \c Dir.
  /// \param File The full name of the File to search for.
  /// \param Dir The full path to a directory to search for \c File within.
  /// \param Result An out variable used to store the full path to \c File
  ///               found within \c Path. This is only set if the result is
  ///               \c true.
  /// \return \c true if \c File was found in \c Path and \c false otherwise.
  ///         If \c true then \c Result will also be set to the full path to
  ///         the found file.
  static bool searchForFileInDirectory(StringRef File, StringRef Dir,
                                       Path &Result);

  /// This is called by \c search after a file is found. If this returns true
  /// then \c File will be returned from \c search, otherwise the file will be
  /// ignored. This callback can be used to ensure e.g. the file has the right
  /// magic in its header.
  /// By default this will open \c File for reading and pass the \c file_t to
  /// \c validateFile.
  /// \param File The full path to a File that exists but needs to be validated.
  /// \return Whether or not the \c File is valid.
  virtual bool validatePath(StringRef Path) const;

  virtual bool validateFile(file_t &) const { return true; }

  /// Convert \c Path into an absolute path.
  static Path processPath(const Twine &P);
  static PathList processPaths(const ConcatPathList &PS) {
    PathList RV;
    std::transform(PS.begin(), PS.end(), std::back_inserter(RV), processPath);
    return RV;
  }

  PathList DefaultPaths_;
  PathList ExtraPaths_;
};

} // end namespace details

/// This class houses the logic for searching for various types of files.
/// In essence this is a list of directories and its main API is to search
/// for a file with a given name in those directories.
///
/// \param Validator A class used to validate a file before returning it from
///                  the searcher.
/// \code{c++}
/// Error operator()(file_t &FD) const;
/// \endcode
///
/// \param NamingScheme A class used to map an incoming query to an actual file
///                     name
/// \code{c++}
/// Path operator()(StringRef File, raw_ostream&) const;
/// \endcode
template <typename Validator, typename NamingScheme>
class FileSearcher : public details::FileSearcherImpl {
public:
  /// Construct a new FileSearcher.
  /// \param DefaultPaths A list of directories to consult by default when
  ///                     searching for a file.
  explicit FileSearcher(const ConcatPathList &DefaultPaths = {})
      : FileSearcherImpl(DefaultPaths) {}

  /// Searches for \c NamingScheme(File) within the extra
  /// search directories added via FileSearcher::addDirectory and then in the
  /// default directories passed when this object was constructed.
  ///
  /// After finding a file it is opened and passed to \c Validator for
  /// validation. If no error is returned then the path to this file is
  /// returned.
  ///
  /// \param File The name of the file to search for.
  /// \return The full path of the found file or a file not found Error.
  Expected<Path> search(StringRef File) const final {
    std::string Filename;
    raw_string_ostream OS(Filename);
    NamingScheme()(File, OS);
    if (OS.str().size() == 0) {
      OS << File;
    }
    return searchInternal(OS.str());
  }

private:
  bool validateFile(file_t &FD) const final { return !Validator()(FD); }
};

} // end namespace ald

} // end namespace llvm
