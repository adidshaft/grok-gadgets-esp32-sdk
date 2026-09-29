// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace grok {
constexpr const char *ProtocolVersion = "0.1.0";
constexpr size_t MaxFrame = 2047;
// A bounded parser that resumes only after the next LF on overflow.
template <size_t Limit = MaxFrame> class Lines {
public:
  enum class Result { Pending, Ready, Overflow };
  Result feed(char ch) {
    if (ch == '\n') {
      if (dropping_) {
        reset();
        return Result::Overflow;
      }
      data_[used_] = 0;
      used_ = 0;
      return Result::Ready;
    }
    if (dropping_)
      return Result::Pending;
    if (used_ == Limit) {
      dropping_ = true;
      return Result::Pending;
    }
    data_[used_++] = ch;
    return Result::Pending;
  }
  const char *value() const { return data_; }
  void reset() {
    used_ = 0;
    dropping_ = false;
    data_[0] = 0;
  }

private:
  char data_[Limit + 1] = {};
  size_t used_ = 0;
  bool dropping_ = false;
};
class Button {
public:
  explicit Button(uint32_t debounceMs = 30) : debounce_(debounceMs) {}
  bool sample(bool pressed, uint32_t now) {
    if (pressed != candidate_) {
      candidate_ = pressed;
      since_ = now;
    }
    if (stable_ != candidate_ && uint32_t(now - since_) >= debounce_) {
      stable_ = candidate_;
      return true;
    }
    return false;
  }
  bool pressed() const { return stable_; }

private:
  uint32_t debounce_, since_ = 0;
  bool candidate_ = false, stable_ = false;
};
template <typename T, size_t Capacity> class Queue {
public:
  bool push(const T &value) {
    if (count_ == Capacity)
      return false;
    entries_[(head_ + count_) % Capacity] = value;
    ++count_;
    return true;
  }
  const T *front() const { return count_ ? &entries_[head_] : nullptr; }
  void pop() {
    if (count_) {
      head_ = (head_ + 1) % Capacity;
      --count_;
    }
  }
  size_t size() const { return count_; }

private:
  T entries_[Capacity] = {};
  size_t head_ = 0, count_ = 0;
};
} // namespace grok
