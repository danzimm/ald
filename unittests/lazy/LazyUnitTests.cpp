// Copyright (c) 2020 Daniel Zimmerman

#include "ADT/Lazy.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::ald;

TEST(LazyTest, TestEager) {
  Lazy<uint32_t> L(4u);
  ASSERT_EQ(L.get(), 4u);

  auto LM = L.map([](uint32_t x) { return x * 7; });
  ASSERT_EQ(LM.get(), 28u);
}

TEST(LazyTest, TestLazy) {
  Lazy<uint32_t> L([]() { return 5u; });
  ASSERT_EQ(L.get(), 5u);

  auto LM = L.map([](uint32_t x) { return x * 7; });
  ASSERT_EQ(LM.get(), 35u);
}

#if 0
TEST(LazyTest, EagerMoveOnlySupported) {
  auto P = std::make_unique<uint32_t>(5);
  auto RP = P.get();
  Lazy<std::unique_ptr<uint32_t>> L(std::move(P));
  ASSERT_EQ(*(L.get()), 5u);

  auto TP = L.take();
  ASSERT_EQ(TP.get(), RP);

  Lazy<std::unique_ptr<uint32_t>> L2(std::move(TP));
  auto LM = std::move(L2).map([](std::unique_ptr<uint32_t> &&TakenP) {
    return std::move(TakenP);
  });

  auto TTP = LM.take();
  ASSERT_EQ(*(TTP.get()), RP);
}
#endif
