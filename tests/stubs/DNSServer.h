#pragma once
#include "Arduino.h"
// Captive DNS: records whether it answers every name, and how often the loop served it.
struct DNSServer {
  bool running = false;
  int served = 0;
  bool start(uint16_t, const char *domain, IPAddress) { return running = domain[0] == '*'; }
  void processNextRequest() { served += running; }
  void stop() { running = false; }
};
