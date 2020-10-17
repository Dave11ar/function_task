#pragma once
#include "descriptor.h"

template<typename F>
struct function;

template<typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept: des(&fns::empty_descriptor<R, Args...>) {}

  function(function const &other) : des(other.des) {
    des->copy(&buf, &other.buf);
  }
  function(function &&other) noexcept: des(other.des) {
    des->move(&buf, &other.buf);
    other.des = &fns::empty_descriptor<R, Args...>;
  }

  template<typename T>
  function(T val) : des(&fns::descriptor<T, R, Args...>) {
    if constexpr (fns::is_small<T>) {
      new(&buf) T(std::move(val));
    } else {
      *reinterpret_cast<T **>(&buf) = new T(std::move(val));
    }
  }

  function &operator=(function const &rhs) {
    if (this == &rhs) {
      return *this;
    }

    function(rhs).swap(*this);
    return *this;
  }
  function &operator=(function &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }

    des->destroy(&buf);
    des = rhs.des;
    des->move(&buf, &rhs.buf);
    rhs.des = &fns::empty_descriptor<R, Args...>;
    return *this;
  }

  ~function() {
    des->destroy(&buf);
  }

  explicit operator bool() const noexcept {
    return des != &fns::empty_descriptor<R, Args...>;
  }

  R operator()(Args... args) const {
    if (!*this == true) {
      throw bad_function_call();
    }

    return des->invoke(&buf, std::forward<Args>(args)...);
  }

  template<typename T>
  T *target() noexcept {
    return check_target<T>() ? fns::get_pointer<T>(&buf) : nullptr;
  }

  template<typename T>
  T const *target() const noexcept {
    return check_target<T>() ? fns::get_pointer<T>(&buf) : nullptr;
  }

 private:
  template<typename T>
  bool check_target() const noexcept {
    return static_cast<bool>(*this) && des == &fns::descriptor<T, R, Args...>;
  }

  void swap(function &other) noexcept {
    auto tmp = std::move(*this);
    *this = std::move(other);
    other = std::move(tmp);
  }

  fns::descriptor_base<R, Args...> const *des;
  fns::buffer buf;
};