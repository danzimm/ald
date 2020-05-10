// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include <functional>

namespace llvm {

namespace ald {

template <typename Value> class Lazy {
public:
  using GenType = ::std::function<Value()>;

  // Construct a Lazy value eagerly. Exposed to break into the lazy world.
  // Also useful for testing.
  Lazy(const Value &V) : Computed_(true), Holder_(V) {}
  Lazy(Value &&V) : Computed_(true), Holder_(std::move(V)) {}

  // Construct a Lazy value from another Lazy value.
  Lazy(const Lazy &Other)
      : Computed_(Other.Computed_), Holder_(Other.Holder_, Computed_) {}
  Lazy(Lazy &&Other)
      : Computed_(Other.Computed_), Holder_(Other.Holder_, Computed_) {}

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

  Lazy &operator=(const Lazy &Other) {
    DestructHolder_();
    new (this) Lazy(Other);
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

  // Lazily map a Lazy value. This delays computation of the map function until
  // Lazy::get() is called on the return value.
  template <typename Functor>
  Lazy<std::result_of_t<Functor(Value)>> map(Functor F) & {
    return Lazy(
        [FF = std::move(F), L = *this]() mutable { return FF(L.get()); });
  }

  template <typename Functor>
  Lazy<std::result_of_t<Functor(Value)>> map(Functor F) && {
    return Lazy([FF = std::move(F), L = std::move(*this)]() mutable {
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

    Holder(const Value &VV) : V(VV) {}
    Holder(Value &&VV) : V(std::move(VV)) {}

    Holder(GenType &&GG) : G(std::move(GG)) {}

    Holder(const Holder &O, bool Computed) {
      if (Computed) {
        V = O.V;
      } else {
        G = O.G;
      }
    }
    Holder(Holder &&O, bool Computed) {
      if (Computed) {
        V = std::move(O.V);
      } else {
        G = std::move(O.G);
      }
    }

    ~Holder() {}
  };
  Holder Holder_;
};

} // end namespace ald

} // end namespace llvm
