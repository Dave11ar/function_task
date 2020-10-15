#pragma once

#include <type_traits>

struct bad_function_call : std::exception {
  char const *what() const noexcept override {
    return "bad function call";
  }
};

namespace fns {
using buffer = typename std::aligned_storage<sizeof(void *), alignof(void *)>::type;

template<typename T>
constexpr bool is_small = sizeof(T) <= sizeof(void *) &&
    std::is_nothrow_move_assignable_v<T> && alignof(buffer) % alignof(T) == 0;

template<typename T>
T *get_pointer(buffer *buf) noexcept {
  if constexpr (is_small<T>) {
    return reinterpret_cast<T *>(buf);
  } else {
    return *reinterpret_cast<T **>(buf);
  }
}

template<typename T>
T const *get_pointer(buffer const *buf) noexcept {
  if constexpr (is_small<T>) {
    return reinterpret_cast<T const *>(buf);
  } else {
    return *reinterpret_cast<T *const *>(buf);
  }
}

template<typename R, typename ...Args>
struct descriptor_base {
  virtual R invoke(buffer const *buf, Args... args) const = 0;
  virtual void destroy(buffer *buf) const= 0;
  virtual void copy(buffer *dst, buffer const *src) const = 0;
};

template<typename T, typename R, typename ...Args>
struct descriptor_t : descriptor_base<R, Args...> {
  R invoke(buffer const *buf, Args... args) const {
    return (*get_pointer<T>(buf))(std::forward<Args>(args)...);
  }

  void destroy(buffer *buf) const {
    if constexpr (is_small<T>) {
      get_pointer<T>(buf)->~T();
    } else {
      delete get_pointer<T>(buf);
    }
  }

  void copy(buffer *dest, buffer const *src) const {
    if constexpr (is_small<T>) {
      new(dest) T(*get_pointer<T>(src));
    } else {
      *reinterpret_cast<T **>(dest) = new T(*get_pointer<T>(src));
    }
  }
};

template<typename T, typename R, typename ...Args>
descriptor_t<T, R, Args...> descriptor;

template<typename R, typename ...Args>
struct empty_descriptor_t : descriptor_base<R, Args...> {
  R invoke(buffer const *buf, Args... args) const {
    throw bad_function_call();
  }

  void destroy(buffer *buf) const {}
  void copy(buffer *dest, buffer const *src) const {}
};

template<typename R, typename ...Args>
empty_descriptor_t<R, Args...> empty_descriptor;
}