// Copyright (c) 2020 Daniel Zimmerman

#include "Util/FileSearcher.h"

#include "gtest/gtest.h"

#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::ald;
using namespace llvm::sys;

class FileScope {
public:
  FileScope(std::initializer_list<const char *> Paths = {}) {
    fs::createUniqueDirectory("ald.FileSearcherScope.XXXXXX", Tmp_);
    fs::createUniqueDirectory("ald.FileSearcherScopeCwd.XXXXXX", Cwd_);
  }

  ~FileScope() { sys::fs::remove_directories(Tmp_); }

  void createPaths(std::initializer_list<const char *> Paths,
                   const char *Contents = nullptr) {
    for (const char *P : Paths) {
      createPath(StringRef(P), Contents);
    }
  }

  void createPath(StringRef P, const char *Contents = nullptr) {
    ASSERT_FALSE(P.empty());

    Twine FP = (P.front() == '/') ? (Tmp_ + P) : (Cwd_ + "/" + P);
    if (P.back() == '/') {
      mkdir_(FP);
    } else {
      touch_(FP, Contents);
    }
  }

  void mkdir_(const Twine &P) {
    if (P.isTriviallyEmpty()) {
      return;
    }
    ASSERT_FALSE(fs::create_directories(P));
  }

  void touch_(const Twine &TP, const char *Contents = nullptr) {
    Path Storage;
    StringRef P = TP.toStringRef(Storage);

    ASSERT_FALSE(fs::create_directories(path::parent_path(P)));

    auto FD = fs::openNativeFileForWrite(P, fs::CD_CreateNew, fs::OF_None);
    ASSERT_TRUE(bool(FD));

    if (Contents == nullptr) {
      Contents = "1234";
    }
    ::write(*FD, Contents, strlen(Contents));

    fs::closeFile(*FD);
  }

  const Path &base() { return Tmp_; }
  const Path &cwd() { return Cwd_; }

private:
  Path Tmp_;
  Path Cwd_;
};

class FileSearcherTest : public ::testing::Test {
public:
  void createPaths(std::initializer_list<const char *> Paths,
                   const char *Contents = nullptr) {
    FS->createPaths(std::move(Paths), Contents);
  }

  void assertFinds_(const char *File, const char *Where, int lineno) {
    auto POrErr = S->search(File);

    ASSERT_TRUE(bool(POrErr)) << lineno << ": Failed to find '" << File
                              << "': " << POrErr.takeError();
    auto &P = *POrErr;

    if (Where[0] == '/') {
      auto &Base = FS->base();
      ASSERT_TRUE(P.substr(0, Base.size()).equals(Base))
          << lineno << ": " << P.str()
          << " does not start with the tmp directory as a prefix: "
          << Base.str().str();
      ASSERT_TRUE(P.substr(Base.size() + 1).equals(Where + 1))
          << lineno << ": " << P.substr(Base.size() + 1)
          << " does not match expected '" << (Where + 1) << "'";
    } else {
      auto &Cwd = FS->cwd();
      ASSERT_TRUE(P.substr(0, Cwd.size()).equals(Cwd))
          << lineno << ": " << P.str()
          << " does not start with the local directory as a prefix: "
          << Cwd.str().str();
      ASSERT_TRUE(P.substr(Cwd.size() + 1).equals(Where))
          << lineno << ": " << P.substr(Cwd.size() + 1)
          << " does not match expected '" << Where << "'";
    }
  }

  void assertDoesntFind_(const char *File, int lineno) {
    ASSERT_FALSE(bool(S->search(File))) << lineno << ": found " << File;
  }

  template <typename Validator, typename NamingScheme, typename... Ts>
  void makeSearcher(Ts... args) {
    makeSearcher_<Validator, NamingScheme>(
        /* SDKPrefixes = */ Prefixes,
        /* Paths = */
        std::vector<std::string>{
            args...,
        },
        /* Defaults = */ SmallVector<StringRef, 2>{});
  }

  template <typename Validator, typename NamingScheme, typename... Ts>
  void makeSearcherWithDefault(StringRef DefaultPath, Ts... args) {
    makeSearcher_<Validator, NamingScheme>(
        /* SDKPrefixes = */ Prefixes,
        /* Paths = */
        std::vector<std::string>{
            args...,
        },
        /* Defaults = */ SmallVector<StringRef, 2>{DefaultPath});
  }

  template <typename Validator, typename NamingScheme, typename... Ts>
  void makeSearcher_(Ts &&... args) {
    SP = std::make_unique<SearchPath>(std::forward<Ts>(args)...,
                                      std::make_unique<Path>(FS->cwd()));
    S = std::make_unique<FileSearcher<Validator, NamingScheme>>(*SP);
  }

  template <typename... Ts> void setPrefixes(Ts... args) {
    Prefixes = std::vector<std::string>{
        (args[0] == '/' ? (FS->base() + args).str() : args)...,
    };
  }

  std::unique_ptr<FileScope> FS;
  std::vector<std::string> Prefixes;

  std::unique_ptr<SearchPath> SP;
  std::unique_ptr<details::FileSearcherImpl> S;

protected:
  void SetUp() override {
    FS = std::make_unique<FileScope>();
    Prefixes = {std::string(FS->base())};
  }

  void TearDown() override {
    S = nullptr;
    SP = nullptr;
    Prefixes.clear();
    FS = nullptr;
  }
};

#define assertFinds(F, W) assertFinds_((F), (W), __LINE__)
#define assertDoesntFind(F) assertDoesntFind_((F), __LINE__)

struct AnyFile {
  Error operator()(file_t &) const { return Error::success(); }
};

struct BasicName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << Name;
  }
};

TEST_F(FileSearcherTest, doesntFindEmpty) {
  makeSearcher<AnyFile, BasicName>("/");

  assertDoesntFind("boo");
}

TEST_F(FileSearcherTest, findsSingleFile) {
  createPaths({"/boo"});
  makeSearcher<AnyFile, BasicName>("/");

  assertFinds("boo", "/boo");
  assertDoesntFind("boo2");
}

TEST_F(FileSearcherTest, findsFileInSecondDir) {
  createPaths({"/first/hoo", "/second/boo"});
  makeSearcher<AnyFile, BasicName>("/first", "/second");

  assertFinds("boo", "/second/boo");
  assertFinds("hoo", "/first/hoo");
  assertDoesntFind("hoo2");
}

TEST_F(FileSearcherTest, ignoresNonExistentDirectories) {
  createPaths({"/first/foo", "/third/hoo"});
  makeSearcher<AnyFile, BasicName>("/first", "/second", "/third");

  assertFinds("foo", "/first/foo");
  assertFinds("hoo", "/third/hoo");
  assertDoesntFind("goo");
}

TEST_F(FileSearcherTest, defaultPathsGoLast) {
  createPaths({"/first/foo", "/first/goo", "/default/bar", "/important/foo",
               "/important/baz", "/default/foo", "/default/goo",
               "/default/baz"});
  makeSearcherWithDefault<AnyFile, BasicName>("/default", "/important",
                                              "/first");

  assertFinds("foo", "/important/foo");
  assertFinds("bar", "/default/bar");
  assertFinds("baz", "/important/baz");
  assertFinds("goo", "/first/goo");
  assertDoesntFind("moo");
}

struct TxtName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << Name << ".txt";
  }
};

TEST_F(FileSearcherTest, findsFileWithSuffix) {
  createPaths({"/first/boo", "/second/boo.txt", "/first/foo.txt", "/second/foo",
               "/third/foo.txt", "/third/boo.txt"});
  makeSearcher<AnyFile, TxtName>("/first", "/second", "/third");

  assertFinds("boo", "/second/boo.txt");
  assertFinds("foo", "/first/foo.txt");
  assertDoesntFind("hoo2");
}

struct LibPrefixName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << "lib" << Name;
  }
};

TEST_F(FileSearcherTest, findsFileWithPrefix) {
  createPaths({"/first/boo", "/second/libboo", "/first/foo",
               "/second/libfoo.txt", "/third/libfoo", "/third/libboo"});
  makeSearcher<AnyFile, LibPrefixName>("/first", "/second", "/third");

  assertFinds("boo", "/second/libboo");
  assertFinds("foo", "/third/libfoo");
  assertDoesntFind("baz");
}

struct MagicFile {
  Error operator()(file_t &FD) const {
    char buffer[4];
    ::read(FD, buffer, 4);
    if (strncmp(buffer, "1337", 4) == 0) {
      return Error::success();
    } else {
      return make_error<StringError>("Bad magic", llvm::errc::invalid_argument);
    }
  }
};

TEST_F(FileSearcherTest, findsFileWithSpecialMagic) {
  createPaths({"/first/boo", "/second/boo", "/first/goo", "/second/moo"});
  createPaths({"/first/woo", "/second/goo", "/third/boo", "/third/hehe"},
              "1337");
  makeSearcher<MagicFile, BasicName>("/first", "/second", "/third");

  assertFinds("boo", "/third/boo");
  assertFinds("woo", "/first/woo");
  assertFinds("goo", "/second/goo");
  assertFinds("hehe", "/third/hehe");
  assertDoesntFind("hoohoo");
}

TEST_F(FileSearcherTest, findsLocalFiles) {
  createPaths({"/first/boo", "local/boo"});
  makeSearcher<AnyFile, BasicName>("local", "/first");

  assertFinds("boo", "local/boo");
  assertDoesntFind("nope");
}

TEST_F(FileSearcherTest, findsFilesWithMultipleSDKPrefixes) {
  createPaths({"/first/boo", "/second/hoo", "/first/goo", "/third/",
               "/prefix/first/boo", "/prefix/second/hoo",
               "/prefix/third/snoo"});
  setPrefixes("/prefix", "/");
  makeSearcher<AnyFile, BasicName>("/first", "/second", "/third");

  assertFinds("boo", "/prefix/first/boo");
  assertFinds("hoo", "/prefix/second/hoo");
  assertFinds("goo", "/first/goo");
  assertFinds("snoo", "/prefix/third/snoo");
  assertDoesntFind("meow");
}

TEST_F(FileSearcherTest, findsFilesInLocalPrefixes) {
  createPaths({"/boo", "/prefix/boo", "prefix/boo"});
  setPrefixes("prefix", "/prefix", "/");
  makeSearcher<AnyFile, BasicName>("/");

  assertFinds("boo", "prefix/boo");
}

TEST_F(FileSearcherTest, findsFilesInCwd) {
  createPaths({"boo", "foo"});
  makeSearcher<AnyFile, BasicName>(".");

  assertFinds("boo", "./boo");
  assertFinds("foo", "./foo");
}

TEST_F(FileSearcherTest, prefixDefaultsToRoot) {
  createPaths({"/boo"});
  setPrefixes();
  makeSearcher<AnyFile, BasicName>(FS->base().str().str());

  assertFinds("boo", "/boo");
}

struct FrameworkName {
  raw_ostream &operator()(StringRef File, raw_ostream &OS) const {
    return OS << File << ".framework/" << File;
  }
};

TEST_F(FileSearcherTest, findsFrameworks) {
  createPaths({"/boo.framework/boo", "/foo.framework/", "/new.framework/n"});
  makeSearcher<AnyFile, FrameworkName>("/");

  assertFinds("boo", "/boo.framework/boo");
  assertDoesntFind("foo");
  assertDoesntFind("new");
}
