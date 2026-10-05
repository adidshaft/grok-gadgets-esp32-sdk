// SPDX-License-Identifier: Apache-2.0
#undef NDEBUG // Tests are assertions.
#include "GrokCore.h"
#include <cassert>
#include <iostream>
#include <string>
int main() {
  grok::Lines<8> lines;
  for (char c : std::string("12345678"))
    assert(lines.feed(c) == decltype(lines)::Result::Pending);
  assert(lines.feed('\n') == decltype(lines)::Result::Ready);
  assert(std::string(lines.value()) == "12345678");
  for (char c : std::string("oversized garbage"))
    lines.feed(c);
  assert(lines.feed('\n') == decltype(lines)::Result::Overflow);
  lines.feed('x');
  assert(lines.feed('\n') == decltype(lines)::Result::Ready);
  assert(std::string(lines.value()) == "x");
  grok::Button button;
  assert(!button.sample(true, 1));
  assert(!button.sample(false, 10));
  assert(!button.sample(true, 11));
  assert(!button.sample(true, 40));
  assert(button.sample(true, 41));
  assert(button.pressed());
  assert(!button.sample(true, 90));
  assert(!button.sample(false, 100));
  assert(button.sample(false, 130));
  assert(!button.pressed());
  grok::Button wrap;
  assert(!wrap.sample(true, UINT32_MAX - 10));
  assert(wrap.sample(true, 20));
  grok::Queue<int, 2> q;
  assert(q.push(1));
  assert(q.push(2));
  assert(!q.push(3));
  assert(*q.front() == 1);
  q.pop();
  assert(q.push(3));
  assert(*q.front() == 2);
  q.pop();
  assert(*q.front() == 3);
  q.pop();
  assert(!q.front());
  std::cout << "core: bounded framing, overflow resync, debounce edges, timer wrap, queue overflow "
               "passed\n";
}
