// Copyright (c) 2020 Daniel Zimmerman

#include "ADT/UniqueFunc.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::ald;

TEST(UFTest, TestNormalLambda) {
  int Flag = 0;
  UniqueFunc<void(void)> F([&]() { Flag = 1; });
  F();

  ASSERT_EQ(Flag, 1);

  UniqueFunc<int(void)> F2([&]() { return ++Flag; });
  ASSERT_EQ(F2(), 2);
  ASSERT_EQ(F2(), 3);
}

TEST(UFTest, TestStdFunction) {
  std::function<int(int)> SF([](int x) { return x * 5; });

  UniqueFunc<int(int)> F(SF);
  ASSERT_EQ(F(5), 25);
  ASSERT_EQ(F(6), 30);
}

TEST(UFTest, TestCustomInvokable) {
  struct CI {
    CI(int X) : X_(X) {}

    std::string operator()(int X) { return std::to_string(X + X_); }

    bool operator()(int X, const std::string &S) { return S == (*this)(X); }

    int X_;
  };

  CI V(6);

  UniqueFunc<std::string(int)> F(V);
  ASSERT_EQ(F(1), "7");

  UniqueFunc<bool(int, const std::string &)> F2(V);
  ASSERT_EQ(F2(2, "7"), false);
  ASSERT_EQ(F2(3, "9"), true);
}

TEST(UFTest, TestPassingConstUF) {
  auto Invoker = [](const UniqueFunc<int(int)> &UF, int X) { return UF(X); };

  UniqueFunc<int(int)> F([](int X) { return X + 3; });
  ASSERT_EQ(Invoker(F, 5), 8);
}

TEST(UFTest, TestPassingMove) {
  auto Invoker = [](UniqueFunc<int(int)> UF, int X) { return UF(X); };

  UniqueFunc<int(int)> F([](int X) { return X + 3; });

  ASSERT_EQ(Invoker(std::move(F), 5), 8);
}

TEST(UFTest, TestCapturingMoveOnly) {
  auto X = std::make_unique<int>(5);
  auto RP = X.get();

  UniqueFunc<std::unique_ptr<int>(void)> F(
      [XX = std::move(X)]() mutable { return std::move(XX); });

  ASSERT_EQ(F().get(), RP);

  // If we call again then we get NULL because we already moved away the
  // unique_ptr
  ASSERT_EQ(F(), nullptr);
}

TEST(UFTest, TestPassingMoveAndCapturingMoveOnly) {
  auto Invoker = [](UniqueFunc<std::unique_ptr<int>(void)> UF, int X) {
    return UF().get() + X;
  };

  auto X = std::make_unique<int>(5);
  auto RP = X.get();
  UniqueFunc<std::unique_ptr<int>(void)> F(
      [XX = std::move(X)]() mutable { return std::move(XX); });

  ASSERT_EQ(Invoker(std::move(F), 5), RP + 5);
}
