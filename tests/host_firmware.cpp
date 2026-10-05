// SPDX-License-Identifier: Apache-2.0
// Unix host harness: actual firmware loop, simulated board, NDJSON stdio.
#define GROK_HOST_SIMULATION
#include "../src/main.cpp"
#include "host_board.h"
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
int main() {
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  // Separate test-only GPIO input: never injected into the device protocol.
  const char *buttonFdEnv = std::getenv("GROK_TEST_BUTTON_FD");
  int buttonFd = buttonFdEnv ? std::atoi(buttonFdEnv) : -1;
  if (buttonFd >= 0)
    fcntl(buttonFd, F_SETFL, O_NONBLOCK);
  auto start = std::chrono::steady_clock::now();
  setup();
  while (true) {
    char buf[4096];
    auto n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n == 0)
      return 0;
    if (n > 0)
      Serial.input.append(buf, n);
    if (buttonFd >= 0) {
      char states[64];
      auto count = read(buttonFd, states, sizeof(states));
      for (ssize_t i = 0; i < count; ++i)
        if (states[i] == '0' || states[i] == '1')
          fake::pin = states[i] - '0';
    }
    fake::now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start)
                    .count();
    loop();
    if (!Serial.output.empty()) {
      std::cout << Serial.output << std::flush;
      Serial.output.clear();
    }
    usleep(1000);
  }
}
