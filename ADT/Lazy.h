// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include <functional>

#include "ADT/UniqueFunc.h"

namespace llvm {

namespace ald {

template <typename Value> class Lazy {
public:
  using GenType = UniqueFunc<Value()>;

  // Construct a Lazy value eagerly. Exposed to break into the lazy world.
  // Also useful for testing.
  Lazy(const Value &V) : Computed_(true), Holder_(V) {}
  Lazy(Value &&V) : Computed_(true), Holder_(std::move(V)) {}

  // Construct a Lazy value from another Lazy value.
  Lazy(Lazy &&Other) : Computed_(Other.Computed_) {
    if (Computed_) {
      new (&Holder_.V) Value(std::move(Other.Holder_.V));
    } else {
      new (&Holder_.G) GenType(std::move(Other.Holder_.G));
    }
  }

  // Construct a Lazy value from an std::function. This is the usual constructor
  // to use to create a truly Lazy value. Remember to be careful of how values
  // are copied/referenced if you pass a lambda here.
  template <typename Functor>
  Lazy(Functor F) : Computed_(false), Holder_(std::move(F)) {}

  // Standard assignment operators associated with the above constructors.
  Lazy &operator=(const Value &V) {
    DestructHolder_();
    new (this) Lazy(V);
    return *this;
  }

  Lazy &operator=(Value &&V) {
    DestructHolder_();
    new (this) Lazy(std::move(V));
    return *this;
  }

  Lazy &operator=(Lazy &&Other) {
    DestructHolder_();
    new (this) Lazy(std::move(Other));
    return *this;
  }

  template <typename Functor> Lazy &operator=(Functor F) {
    DestructHolder_();
    new (this) Lazy(std::move(F));
    return *this;
  }

  template <typename F> using LazyMapped = Lazy<std::result_of_t<F(Value)>>;

  // Lazily map a Lazy value. This delays computation of the map function until
  // Lazy::get() is called on the return value. Note: the return value's
  // lifetime is contrained by this' lifetime.
  template <typename Functor> LazyMapped<Functor> map(Functor F) & {
    return LazyMapped<Functor>(
        [FF = std::move(F), &L = *this]() mutable { return FF(L.get()); });
  }

  template <typename Functor> LazyMapped<Functor> map(Functor F) && {
    return LazyMapped<Functor>(
        [FF = std::move(F), L = std::move(*this)]() mutable {
          return FF(L.take());
        });
  }

  // Force the Lazy computation to occur and get the underlying value.
  const Value &get() {
    EnsureComputed_();
    return Holder_.V;
  }

  Value take() {
    EnsureComputed_();
    return std::move(Holder_.V);
  }

private:
  void EnsureComputed_() {
    if (!Computed_) {
      auto V = std::move(Holder_.G());
      Holder_.G.~GenType();
      Holder_.V = std::move(V);
      Computed_ = true;
    }
  }

  void DestructHolder_() {
    if (Computed_) {
      Holder_.V.~Value();
    } else {
      Holder_.G.~GenType();
    }
  }

  bool Computed_;
  union Holder {
    Value V;
    GenType G;

    Holder() {}

    Holder(const Value &VV) : V(VV) {}
    Holder(Value &&VV) : V(std::move(VV)) {}

    template <typename F> Holder(F &&FF) : G(std::move(FF)) {}

    ~Holder() {}
  };
  Holder Holder_;
};

} // end namespace ald

} // end namespace llvm
