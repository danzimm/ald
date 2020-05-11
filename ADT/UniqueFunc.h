// Copyright (c) 2020 Daniel Zimmerman

#pragma once

#include <memory>
#include <type_traits>

namespace llvm {

namespace ald {

namespace details {
template <typename F, typename R, typename... Args> class RVConv {
public:
  static constexpr bool value =
      std::is_convertible<std::result_of_t<F(Args...)>, R>::value;
};

template <typename R, typename... Args> struct UFImpl {
  virtual R Invoke(Args &&... args) = 0;
  virtual ~UFImpl() {}
};

template <typename F, typename R, typename... Args>
struct UFImplImpl : public UFImpl<R, Args...> {
  template <typename FF> UFImplImpl(FF &&FF_) : F_(std::forward<FF>(FF_)) {}

  R Invoke(Args &&... args) override { return F_(std::forward<Args>(args)...); }

  F F_;
};

template <typename F, typename... Args>
struct UFImplImpl<F, void, Args...> : public UFImpl<void, Args...> {
  template <typename FF> UFImplImpl(FF &&FF_) : F_(std::forward<FF>(FF_)) {}

  void Invoke(Args &&... args) override { F_(std::forward<Args>(args)...); }

  F F_;
};
} // namespace details

template <typename Sig> class UniqueFunc;

// This class is similar to std::function, but enables capturing data that is
// move only. For example the following isn't allowed as the input functor
// must be copyable (as per C++ standard):
//
// std::function<std::unique_ptr<int>(void)> F([XX = std::move(X)]() {
//    return std::move(XX);
// });
//
// However we are allowed to write the same statement, but with UniqueFunc in
// place of std::function.
template <typename R, typename... Args> class UniqueFunc<R(Args...)> {
public:
  UniqueFunc() = default;
  UniqueFunc(UniqueFunc &&) = default;
  UniqueFunc &operator=(UniqueFunc &&) = default;

  UniqueFunc(const UniqueFunc &) = delete;
  UniqueFunc &operator=(const UniqueFunc &) = delete;

  // This is the magic constructor that will enable a UniqueFunc to be
  // constructed from a lambda, a std::function or any other custom invokable
  // the user may provide.
  //
  // The type traits validate that the functor F:
  //  - can be called with Args...  to return a value that is convertible to R
  //  - is not a UniqueFunc itself. Without this type trait attempting to copy
  //    a UniqueFunc will attempt to use this overload and eventually end up
  //    stack overflowing.
  template <typename F, typename = std::enable_if_t<
                            details::RVConv<F, R, Args...>::value &&
                            !std::is_same<std::decay_t<F>, UniqueFunc>::value>>
  UniqueFunc(F &&Func) : Impl_(make_impl<F>(std::forward<F>(Func))) {}

  R operator()(Args... args) const {
    return Impl_->Invoke(std::forward<Args>(args)...);
  }

private:
  template <typename F>
  std::unique_ptr<details::UFImpl<R, Args...>> make_impl(F &&Func) {
    return std::make_unique<details::UFImplImpl<std::decay_t<F>, R, Args...>>(
        std::forward<F>(Func));
  }

  std::unique_ptr<details::UFImpl<R, Args...>> Impl_;
};

} // end namespace ald

} // end namespace llvm
