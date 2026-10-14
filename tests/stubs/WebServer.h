#pragma once
#include "Arduino.h"
#include <functional>
#include <map>
#include <string>
// The parts of the core's WebServer the setup page uses. Tests call request() to run a handler
// as if a phone had sent it, then read the response fields.
enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST };
struct WebServer {
  using Handler = std::function<void()>;
  explicit WebServer(int) {}
  std::map<std::pair<std::string, int>, Handler> routes;
  Handler notFound;
  std::map<std::string, std::string> args, headers;
  int status = 0;
  std::string type, body;
  bool running = false;
  void on(const char *uri, HTTPMethod method, Handler handler) { routes[{uri, method}] = handler; }
  void onNotFound(Handler handler) { notFound = handler; }
  void begin() { running = true; }
  void stop() { running = false; }
  void handleClient() {}
  std::string arg(const char *name) const {
    auto found = args.find(name);
    return found == args.end() ? std::string() : found->second;
  }
  void sendHeader(const char *name, const char *value, bool = false) { headers[name] = value; }
  void send(int code, const char *contentType, const std::string &content) {
    status = code;
    type = contentType;
    body = content;
  }
  // Test helper: one request; returns the status code (404 when the server is stopped).
  int request(HTTPMethod method, const std::string &uri,
              const std::map<std::string, std::string> &values = {}) {
    status = 0;
    headers.clear();
    body.clear();
    if (!running)
      return 404;
    args = values;
    auto found = routes.find({uri, method});
    if (found != routes.end())
      found->second();
    else if (notFound)
      notFound();
    args.clear();
    return status;
  }
};
