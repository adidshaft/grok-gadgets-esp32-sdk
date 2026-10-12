#pragma once
#include "Arduino.h"
#include <map>
#include <string>
// mDNS: `hosts` maps a name without .local to its address; unknown names answer 0.0.0.0.
struct FakeMDNS {
  bool started = false;
  std::string name;
  std::map<std::string, IPAddress> hosts;
  int queries = 0;
  bool begin(const char *hostName) {
    started = true;
    name = hostName;
    return true;
  }
  IPAddress queryHost(const char *host, uint32_t = 2000) {
    ++queries;
    auto found = hosts.find(host);
    return found == hosts.end() ? IPAddress() : found->second;
  }
};
extern FakeMDNS MDNS;
