// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include "ADT/DataTypes.h"

#include <iostream>

namespace llvm {

namespace ald {

/// This class houses the model backing a \c FileSearcher. The user must
/// construct one of these in order to tell \c FileSearcher where to look for
/// files.
class SearchPath {
public:
  /// Construct a SearchPath.
  /// \param SDKPrefixes  A list of SDKs to use as prefixes. If this list is
  ///                     empty then the default prefix '/' will be used.
  /// \param Paths        A list of paths the user specified to search within.
  /// \param DefaultPaths A list of default paths to search within. These paths
  ///                     will be searched after all the paths in \c Paths are
  ///                     searched.
  /// \param Cwd          An optional CWD to root all relative paths against.
  SearchPath(const std::vector<std::string> &SDKPrefixes,
             const std::vector<std::string> &Paths,
             const SmallVector<StringRef, 2> &DefaultPaths,
             std::unique_ptr<Path> Cwd = nullptr);

  /// Visit each of the search paths that were used to initialize this object.
  /// \param V The visitor that should be invoked for each search path. Note
  ///          this visitor should implement `bool operator()(const Twine&)`.
  ///          When this function returns true the visitor will stop early.
  /// \return Whether or not the visitor returned early.
  template <typename Visitor> bool visit(Visitor V) const {
    for (const Path &Prefix : SDKPrefixes_) {
      for (StringRef Path : Absolute_) {
        if (V(Prefix + Path)) {
          return true;
        }
      }
    }
    for (StringRef Path : Relative_) {
      if (V(Cwd_ + Path)) {
        return true;
      }
    }
    return false;
  }

  size_t size() const {
    return SDKPrefixes_.size() * Absolute_.size() + Relative_.size();
  }

private:
  PathList SDKPrefixes_;
  Path Cwd_;
  SmallVector<std::string, 8> Absolute_;
  SmallVector<std::string, 8> Relative_;
};

namespace details {

class FileSearcherImpl {
public:
  virtual ~FileSearcherImpl() {}

  /// Returns all the directories that will be searched in the order that they
  /// will be searched.
  PathList getAllPaths() const;

  virtual Expected<Path> search(StringRef File) const {
    return searchInternal(File);
  }

protected:
  virtual bool validateFile(file_t &) const { return true; }

  explicit FileSearcherImpl(const SearchPath &SP) : SearchPath_(SP) {}

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
  static bool searchForFileInDirectory(StringRef File, const Twine &Dir,
                                       Path &Result);

  /// This is called by \c search after a file is found. If this returns true
  /// then \c File will be returned from \c search, otherwise the file will be
  /// ignored. This callback can be used to ensure e.g. the file has the right
  /// magic in its header.
  /// By default this will open \c File for reading and pass the \c file_t to
  /// \c validateFile.
  /// \param File The full path to a File that exists but needs to be validated.
  /// \return Whether or not the \c File is valid.
  bool validatePath(StringRef Path) const;

  const SearchPath &SearchPath_;
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
/// raw_ostream &operator()(StringRef File, raw_ostream&) const;
/// \endcode
template <typename Validator, typename NamingScheme>
class FileSearcher : public details::FileSearcherImpl {
public:
  /// Construct a new FileSearcher.
  /// \param SP The \c SearchPath to consult when searching for a file.
  explicit FileSearcher(const SearchPath &SP) : FileSearcherImpl(SP) {}

  FileSearcher(FileSearcher &&) = default;
  FileSearcher &operator=(FileSearcher &&) = default;

  FileSearcher(const FileSearcher &) = delete;
  FileSearcher &operator=(const FileSearcher &) = delete;

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
    return searchInternal(
        ((raw_string_ostream &)NamingScheme()(File, OS)).str());
  }

private:
  bool validateFile(file_t &FD) const final { return !Validator()(FD); }
};

} // end namespace ald

} // end namespace llvm
