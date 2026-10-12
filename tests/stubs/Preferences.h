#pragma once
#include <cstring>
#include <map>
#include <string>
namespace fake {
extern std::map<std::string, std::string> nvs; // "namespace/key" to value.
extern bool nvsFails;
} // namespace fake
struct Preferences {
  std::string space;
  bool readOnly = true;
  bool begin(const char *name, bool ro) {
    space = name;
    readOnly = ro;
    return !fake::nvsFails;
  }
  void end() {}
  bool clear() {
    for (auto it = fake::nvs.begin(); it != fake::nvs.end();)
      it = it->first.rfind(space + "/", 0) == 0 ? fake::nvs.erase(it) : std::next(it);
    return !readOnly;
  }
  size_t putString(const char *key, const char *value) {
    if (readOnly)
      return 0;
    fake::nvs[space + "/" + key] = value;
    return strlen(value);
  }
  size_t putUShort(const char *key, uint16_t value) {
    if (readOnly)
      return 0;
    fake::nvs[space + "/" + key] = std::to_string(value);
    return 2;
  }
  size_t getString(const char *key, char *out, size_t size) {
    auto it = fake::nvs.find(space + "/" + key);
    if (it == fake::nvs.end() || it->second.size() >= size)
      return 0;
    memcpy(out, it->second.c_str(), it->second.size() + 1);
    return it->second.size();
  }
  uint16_t getUShort(const char *key, uint16_t fallback) {
    auto it = fake::nvs.find(space + "/" + key);
    return it == fake::nvs.end() ? fallback : uint16_t(std::stoi(it->second));
  }
};
