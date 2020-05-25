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

    Twine FP = Tmp_ + "/" + P;
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

private:
  Path Tmp_;
};

class FileSearcherTest : public ::testing::Test {
public:
  void createPaths(std::initializer_list<const char *> Paths,
                   const char *Contents = nullptr) {
    FS->createPaths(std::move(Paths), Contents);
  }

  void assertFinds(const char *File, const char *Where) {
    auto POrErr = S->search(File);

    ASSERT_TRUE(bool(POrErr))
        << "Failed to find '" << File << "': " << POrErr.takeError();
    auto &P = *POrErr;
    auto &Base = FS->base();

    ASSERT_TRUE(P.substr(0, Base.size()).equals(Base))
        << P.str() << " does not start with the tmp directory as a prefix: "
        << Base.str().str();
    ASSERT_TRUE(P.substr(Base.size() + 1).equals(Where))
        << P.substr(Base.size() + 1) << " does not match expected '" << Where
        << "'";
  }

  void assertDoesntFind(const char *File) {
    ASSERT_FALSE(bool(S->search(File)));
  }

  template <typename Validator, typename NamingScheme, typename... Ts>
  void makeSearcher(Ts... args) {
    S = std::make_unique<FileSearcher<Validator, NamingScheme>>(
        std::initializer_list<Twine>{
            FS->base() + "/" + args...,
        });
  }

  void addDirectory(const char *Dir) {
    S->addDirectory(FS->base() + "/" + Dir);
  }

  std::unique_ptr<FileScope> FS;
  std::unique_ptr<details::FileSearcherImpl> S;

protected:
  void SetUp() override { FS = std::make_unique<FileScope>(); }

  void TearDown() override {
    FS = nullptr;
    S = nullptr;
  }
};

struct AnyFile {
  Error operator()(file_t &) const { return Error::success(); }
};

struct BasicName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << Name;
  }
};

TEST_F(FileSearcherTest, doesntFindEmpty) {
  makeSearcher<AnyFile, BasicName>("");

  assertDoesntFind("boo");
}

TEST_F(FileSearcherTest, findsSingleFile) {
  createPaths({"boo"});
  makeSearcher<AnyFile, BasicName>("");

  assertFinds("boo", "boo");
  assertDoesntFind("boo2");
}

TEST_F(FileSearcherTest, findsFileInSecondDir) {
  createPaths({"first/hoo", "second/boo"});
  makeSearcher<AnyFile, BasicName>("first", "second");

  assertFinds("boo", "second/boo");
  assertFinds("hoo", "first/hoo");
  assertDoesntFind("hoo2");
}

TEST_F(FileSearcherTest, ignoresNonExistentDirectories) {
  createPaths({"first/foo", "third/hoo"});
  makeSearcher<AnyFile, BasicName>("first", "second", "third");

  assertFinds("foo", "first/foo");
  assertFinds("hoo", "third/hoo");
  assertDoesntFind("goo");
}

TEST_F(FileSearcherTest, extraPathsOverrideDefaultPaths) {
  createPaths({"first/foo", "first/goo", "second/bar", "important/foo",
               "important/baz"});
  makeSearcher<AnyFile, BasicName>("first", "second");
  addDirectory("important");

  assertFinds("foo", "important/foo");
  assertFinds("bar", "second/bar");
  assertFinds("baz", "important/baz");
  assertFinds("goo", "first/goo");
  assertDoesntFind("moo");
}

struct TxtName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << Name << ".txt";
  }
};

TEST_F(FileSearcherTest, findsFileWithSuffix) {
  createPaths({"first/boo", "second/boo.txt", "first/foo.txt", "second/foo",
               "third/foo.txt", "third/boo.txt"});
  makeSearcher<AnyFile, TxtName>("first", "second", "third");

  assertFinds("boo", "second/boo.txt");
  assertFinds("foo", "first/foo.txt");
  assertDoesntFind("hoo2");
}

struct LibPrefixName {
  raw_ostream &operator()(StringRef Name, raw_ostream &FileBuilder) const {
    return FileBuilder << "lib" << Name;
  }
};

TEST_F(FileSearcherTest, findsFileWithPrefix) {
  createPaths({"first/boo", "second/libboo", "first/foo", "second/libfoo.txt",
               "third/libfoo", "third/libboo"});
  makeSearcher<AnyFile, LibPrefixName>("first", "second", "third");

  assertFinds("boo", "second/libboo");
  assertFinds("foo", "third/libfoo");
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
  createPaths({"first/boo", "second/boo", "first/goo", "second/moo"});
  createPaths({"first/woo", "second/goo", "third/boo", "third/hehe"}, "1337");
  makeSearcher<MagicFile, BasicName>("first", "second", "third");

  assertFinds("boo", "third/boo");
  assertFinds("woo", "first/woo");
  assertFinds("goo", "second/goo");
  assertFinds("hehe", "third/hehe");
  assertDoesntFind("hoohoo");
}
