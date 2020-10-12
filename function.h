#pragma once
#include "descriptor.h"

template<typename F>
struct function;

template<typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept {
    des = &empty_descriptor<R, Args...>;
  }

  function(function const &other) : des(other.des) {
    des->copy(&buf, &other.buf);
  }
  function(function &&other) noexcept : function() {
    swap(other);
  }

  template<typename T>
  function(T val) {
    if (is_small<T>()) {
      new (&buf) T(std::move(val));
    } else {
      *reinterpret_cast<T**>(&buf) = new T(std::move(val));
    }

    des = &descriptor<T, R, Args...>;
  }

  function &operator=(function const &rhs) {
    if (this == &rhs) {
      return *this;
    }

    des = rhs.des;
    des->copy(&buf, &rhs.buf);
    return *this;
  }
  function &operator=(function &&rhs) noexcept {
    swap(rhs);
    return *this;
  }

  ~function() {
    des->destroy(&buf);
  }

  explicit operator bool() const noexcept {
    return des != &empty_descriptor<R, Args...>;
  }

  R operator()(Args... args) const {
    if (!*this == true) {
      throw bad_function_call();
    }

    return des->invoke(&buf, std::forward<Args>(args)...);
  }

  template<typename T>
  T *target() noexcept {
    if (!*this == true || des != &descriptor<T, R, Args...>) {
      return nullptr;
    }
    return get_pointer<T>(&buf);
  }

  template<typename T>
  T const *target() const noexcept {
    if (!*this == true || des != &descriptor<T, R, Args...>) {
      return nullptr;
    }
    return get_const_pointer<T>(&buf);
  }

 private:
  void swap(function &other) {
    using std::swap;
    swap(buf, other.buf);
    swap(des, other.des);
  }

  buffer buf;
  descriptor_base<R, Args...> *des;
};