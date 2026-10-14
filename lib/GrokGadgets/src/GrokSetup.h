// SPDX-License-Identifier: Apache-2.0
// Setup hotspot with a captive page (hub ADR 010): a person joins the gadget to home Wi-Fi
// with a phone alone. The hotspot uses the per-device WPA2 passphrase from the label, opens
// only while the gadget has no home Wi-Fi, and closes after 10 minutes or a successful join.
// The page asks for the label's setup code before anything else. It never shows a stored
// value: not the home Wi-Fi, the token, the passphrase or the setup code.
#pragma once
#include "GrokWifi.h"
#include <DNSServer.h>
#include <WebServer.h>

namespace grok {
constexpr uint32_t SetupWindowMs = 10UL * 60 * 1000;
constexpr uint32_t SetupJoinMs = 20000; // One join attempt from the page.
// After a join the hotspot stays up long enough for the page to show Online, then closes.
constexpr uint32_t SetupOnlineGraceMs = 30000, SetupJoinedGraceMs = 180000;
// Wrong setup codes: 5 free, then a 1-minute lock that doubles; 20 close the hotspot.
constexpr uint8_t SetupFreeCodes = 5, SetupMaxCodes = 20;
constexpr uint32_t SetupLockMs = 60000;
constexpr size_t SetupMaxNetworks = 16;
constexpr const char *SetupNamePrefix = "GrokGadget-";

// `GrokGadget-` and the last four characters of the device ID, in upper case.
inline void setupHotspotName(const char *deviceId, char (&out)[16]) {
  size_t length = strlen(deviceId);
  const char *tail = deviceId + (length > 4 ? length - 4 : 0);
  snprintf(out, sizeof(out), "%s%s", SetupNamePrefix, tail);
  for (char *c = out + strlen(SetupNamePrefix); *c; ++c)
    if (*c >= 'a' && *c <= 'z')
      *c = char(*c - 'a' + 'A');
}
// What a person typed, as the label prints it: upper case; spaces and dashes dropped.
inline bool normalizeSetupCode(const char *typed, char (&out)[SetupCodeLength + 1]) {
  size_t used = 0;
  memset(out, 0, sizeof(out));
  for (const char *c = typed; *c; ++c) {
    if (*c == ' ' || *c == '-')
      continue;
    if (used == SetupCodeLength)
      return false;
    out[used++] = (*c >= 'a' && *c <= 'z') ? char(*c - 'a' + 'A') : *c;
  }
  return used == SetupCodeLength;
}
// Compare in constant time over the code length.
inline bool sameSetupCode(const char *expected, const char *typed) {
  uint8_t diff = 0;
  for (size_t i = 0; i < SetupCodeLength; ++i)
    diff |= uint8_t(expected[i] ^ typed[i]);
  return diff == 0 && strlen(expected) == SetupCodeLength;
}

// Rate limits for setup codes. Pure logic, so host tests can drive the clock.
class SetupGuard {
public:
  enum class Verdict { Ok, Wrong, Locked, Closed };
  Verdict check(const char *expected, const char *typed, uint32_t now) {
    if (closed_)
      return Verdict::Closed;
    if (locked(now))
      return Verdict::Locked;
    char code[SetupCodeLength + 1];
    bool ok = normalizeSetupCode(typed, code) && sameSetupCode(expected, code);
    memset(code, 0, sizeof(code));
    if (ok)
      return Verdict::Ok;
    if (++wrong_ >= SetupMaxCodes) {
      closed_ = true;
      return Verdict::Closed;
    }
    if (wrong_ >= SetupFreeCodes) {
      uint8_t doublings = wrong_ - SetupFreeCodes < 6 ? wrong_ - SetupFreeCodes : 6;
      lockedUntil_ = now + (SetupLockMs << doublings);
    }
    return Verdict::Wrong;
  }
  bool locked(uint32_t now) const { return lockedUntil_ && int32_t(lockedUntil_ - now) > 0; }
  uint32_t retryAfterSeconds(uint32_t now) const {
    return locked(now) ? (lockedUntil_ - now + 999) / 1000 : 0;
  }
  bool closed() const { return closed_; }

private:
  uint8_t wrong_ = 0;
  uint32_t lockedUntil_ = 0;
  bool closed_ = false;
};

// The setup page: a code step, a network step and live status, in one small page.
constexpr const char *SetupPage = R"(<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Set up your gadget</title>
<style>body{font:17px -apple-system,system-ui,sans-serif;margin:0;padding:20px;max-width:28em;
color:#111;background:#fff}h1{font-size:22px}label{display:block;margin:14px 0 4px}
input,select,button{font:inherit;width:100%;box-sizing:border-box;padding:10px;border:1px solid #888;
border-radius:8px}button{background:#111;color:#fff;border:0;margin-top:16px}
#msg{margin-top:16px;min-height:3em}.hide{display:none}
@media(prefers-color-scheme:dark){body{background:#111;color:#eee}button{background:#eee;color:#111}}
</style></head><body><h1>Set up your gadget</h1>
<form id="s1"><label for="code">Setup code from the label</label>
<input id="code" autocomplete="off" autocapitalize="characters" spellcheck="false" placeholder="XXXX-XXXX" required>
<button>Continue</button></form>
<form id="s2" class="hide"><label for="net">Your home Wi-Fi (2.4 GHz)</label><select id="net"></select>
<label for="other" id="otherl" class="hide">Network name</label><input id="other" class="hide" autocomplete="off">
<label for="pw">Wi-Fi password</label><input id="pw" type="password" autocomplete="off">
<button>Join</button></form><p id="msg" role="status" aria-live="polite"></p>
<script>
const $=id=>document.getElementById(id),msg=t=>$('msg').textContent=t;
const say={open:'',joining:'Joining your Wi-Fi. This page can lose the connection for a few seconds; the gadget display shows the result too.',
joined:'Joined your Wi-Fi. Connecting to the relay...',online:'Done. The gadget is Online. You can close this page.',
failed:'',closed:'Setup has closed. Turn the gadget off and on to open it again.',
wrong_password:'The network rejected the password. Check it and try again.',
no_network:'Network not found. Use a 2.4 GHz network and check the name.',
join_timeout:'No answer from the network. Check that the router is on and in range.',
wrong_code:'That setup code is not right. Check the label.',locked:'Too many wrong codes. Wait',
invalid_settings:'Check the network name and the password (8 to 63 characters, or empty for an open network).'};
async function post(path,body){const r=await fetch(path,{method:'POST',body:new URLSearchParams(body)});
return r.json().catch(()=>({ok:false,error:'closed'}))}
function err(j){msg(j.error=='locked'?say.locked+' '+j.retry_after+' s.':(say[j.error]||say.closed))}
$('s1').onsubmit=async e=>{e.preventDefault();msg('Checking...');const j=await post('/unlock',{code:$('code').value});
if(!j.ok)return err(j);const n=$('net');n.innerHTML='';for(const w of j.networks){const o=document.createElement('option');
o.value=o.textContent=w.name;n.append(o)}const o=document.createElement('option');o.value='';o.textContent='Other network...';n.append(o);
$('s1').classList.add('hide');$('s2').classList.remove('hide');n.onchange();msg(j.networks.length?'':'No networks found yet. Pick Other network, or reload.')};
$('net').onchange=()=>{const other=$('net').value=='';$('other').classList.toggle('hide',!other);$('otherl').classList.toggle('hide',!other)};
$('s2').onsubmit=async e=>{e.preventDefault();const ssid=$('net').value||$('other').value;
const j=await post('/join',{code:$('code').value,ssid:ssid,password:$('pw').value});if(!j.ok)return err(j);
msg(say.joining);poll()};
async function poll(){let j;try{j=await(await fetch('/status')).json()}catch(e){return setTimeout(poll,1500)}
if(j.state=='failed'){msg(say[j.reason]||say.join_timeout);return}msg(say[j.state]||'');
if(j.state!='online'&&j.state!='closed')setTimeout(poll,1000)}
</script></body></html>)";

class SetupHotspot {
public:
  enum class State { Off, Open, Joining, Failed, Joined, Online, Expired, Locked };
  SetupHotspot(SettingsStore &store, WifiSettings &settings, const FactorySettings &factory)
      : store_(store), settings_(settings), factory_(factory) {}

  // Open the hotspot when the gadget has a factory identity and label values but no home
  // Wi-Fi: on first power-on, or after a BOOT reset. Returns true when it opened.
  bool begin(const char *deviceId, uint32_t now) {
    if (!factory_.setupReady() || settings_.complete())
      return false;
    setupHotspotName(deviceId, name_);
    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent([this](arduino_event_id_t,
                        arduino_event_info_t info) { reason_ = info.wifi_sta_disconnected.reason; },
                 ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    if (!WiFi.softAP(name_, factory_.passphrase, 1, 0, 1))
      return false;
    dns_.start(53, "*", WiFi.softAPIP());
    server_.on("/", HTTP_GET, [this]() { page(); });
    server_.on("/unlock", HTTP_POST, [this]() { unlock(); });
    server_.on("/join", HTTP_POST, [this]() { join(); });
    server_.on("/status", HTTP_GET, [this]() { status(); });
    server_.onNotFound([this]() { redirect(); }); // Captive checks on both phones land here.
    server_.begin();
    WiFi.scanNetworks(true);
    openedAt_ = now_ = now;
    state_ = State::Open;
    hotspotUp_ = true;
    return true;
  }
  void loop(uint32_t now, bool online) {
    now_ = now;
    if (!hotspotUp_)
      return;
    dns_.processNextRequest();
    server_.handleClient();
    collectScan();
    switch (state_) {
    case State::Open:
    case State::Failed:
      if (guard_.closed())
        close(State::Locked);
      else if (uint32_t(now - openedAt_) >= SetupWindowMs)
        close(State::Expired);
      break;
    case State::Joining:
      followJoin(now);
      break;
    case State::Joined:
      if (online) {
        state_ = State::Online;
        onlineAt_ = now;
      } else if (uint32_t(now - joinedAt_) >= SetupJoinedGraceMs) {
        close(State::Joined); // Wi-Fi is saved; the display keeps reporting the relay.
      }
      break;
    case State::Online:
      if (uint32_t(now - onlineAt_) >= SetupOnlineGraceMs)
        close(State::Online);
      break;
    default:
      break;
    }
  }
  // True once, right after the home Wi-Fi was joined and saved: start the link now.
  bool takeJoined() {
    bool joined = joinedNow_;
    joinedNow_ = false;
    return joined;
  }
  State state() const { return state_; }
#ifdef GROK_SETUP_TESTS
  WebServer &serverForTests() { return server_; } // Host tests act as the phone.
#endif
  // The hotspot is up and the display should show setup.
  bool active() const { return hotspotUp_; }
  JoinFailure failure() const { return failure_; }
  const char *name() const { return name_; }
  uint32_t minutesLeft(uint32_t now) const {
    uint32_t used = now - openedAt_;
    return used >= SetupWindowMs ? 0 : (SetupWindowMs - used + 59999) / 60000;
  }
  static const char *stateName(State state) {
    switch (state) {
    case State::Open:
      return "open";
    case State::Joining:
      return "joining";
    case State::Failed:
      return "failed";
    case State::Joined:
      return "joined";
    case State::Online:
      return "online";
    case State::Expired:
    case State::Locked:
      return "closed";
    default:
      return "off";
    }
  }

private:
  void close(State state) {
    server_.stop();
    dns_.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    wipePending();
    hotspotUp_ = false;
    state_ = state;
  }
  void followJoin(uint32_t now) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!store_.saveWifi(pendingSsid_, pendingPassword_)) {
        failure_ = JoinFailure::Timeout;
        state_ = State::Failed;
        wipePending();
        return;
      }
      memcpy(settings_.ssid, pendingSsid_, sizeof(settings_.ssid));
      memcpy(settings_.password, pendingPassword_, sizeof(settings_.password));
      wipePending();
      failure_ = JoinFailure::None;
      state_ = State::Joined;
      joinedAt_ = now;
      joinedNow_ = true;
      return;
    }
    JoinFailure seen = classifyDisconnect(reason_);
    if (seen == JoinFailure::None && uint32_t(now - joinStartedAt_) < SetupJoinMs)
      return;
    failure_ = seen == JoinFailure::None ? JoinFailure::Timeout : seen;
    WiFi.disconnect();
    wipePending();
    state_ = State::Failed;
  }
  void wipePending() {
    memset(pendingSsid_, 0, sizeof(pendingSsid_));
    memset(pendingPassword_, 0, sizeof(pendingPassword_));
  }
  void collectScan() {
    int16_t found = WiFi.scanComplete();
    if (found < 0)
      return;
    networks_ = 0;
    for (int16_t i = 0; i < found && networks_ < SetupMaxNetworks; ++i) {
      const auto ssid = WiFi.SSID(i);
      const char *text = ssid.c_str();
      size_t length = strlen(text);
      if (!length || length >= sizeof(found_[0].name))
        continue;
      bool duplicate = false;
      for (size_t j = 0; j < networks_ && !duplicate; ++j)
        duplicate = !strcmp(found_[j].name, text);
      if (duplicate)
        continue; // Results come strongest first, so the first copy has the best signal.
      memcpy(found_[networks_].name, text, length + 1);
      found_[networks_].secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      ++networks_;
    }
    WiFi.scanDelete();
    scannedAt_ = now_;
  }
  // The code gate for /unlock and /join. Sends the refusal itself; true when the code is right.
  bool codeAccepted() {
    switch (guard_.check(factory_.setupCode, server_.arg("code").c_str(), now_)) {
    case SetupGuard::Verdict::Ok:
      return true;
    case SetupGuard::Verdict::Locked: {
      char body[64];
      snprintf(body, sizeof(body), R"({"ok":false,"error":"locked","retry_after":%u})",
               unsigned(guard_.retryAfterSeconds(now_)));
      server_.send(429, "application/json", body);
      return false;
    }
    case SetupGuard::Verdict::Closed:
      server_.send(403, "application/json", R"({"ok":false,"error":"closed"})");
      return false;
    default:
      server_.send(403, "application/json", R"({"ok":false,"error":"wrong_code"})");
      return false;
    }
  }
  bool accepting() const { return state_ == State::Open || state_ == State::Failed; }
  void page() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "text/html", SetupPage);
  }
  void redirect() {
    server_.sendHeader("Location", "http://192.168.4.1/", true);
    server_.send(302, "text/plain", "");
  }
  void unlock() {
    if (!accepting()) {
      server_.send(409, "application/json", R"({"ok":false,"error":"closed"})");
      return;
    }
    if (!codeAccepted())
      return;
    if (WiFi.scanComplete() != -1 && uint32_t(now_ - scannedAt_) > 15000)
      WiFi.scanNetworks(true); // Fresh results for the next visit.
    StaticJsonDocument<1536> out;
    out["ok"] = true;
    JsonArray list = out.createNestedArray("networks");
    for (size_t i = 0; i < networks_; ++i) {
      JsonObject entry = list.createNestedObject();
      entry["name"] = found_[i].name; // Live scan results, not stored values.
      entry["secure"] = found_[i].secure;
    }
    sendJson(200, out);
  }
  void join() {
    if (!accepting()) {
      server_.send(409, "application/json", R"({"ok":false,"error":"closed"})");
      return;
    }
    if (!codeAccepted())
      return;
    const auto ssid = server_.arg("ssid");
    const auto password = server_.arg("password");
    size_t ssidLength = strlen(ssid.c_str()), passwordLength = strlen(password.c_str());
    if (!ssidLength || ssidLength >= sizeof(pendingSsid_) ||
        passwordLength >= sizeof(pendingPassword_) || (passwordLength && passwordLength < 8)) {
      server_.send(400, "application/json", R"({"ok":false,"error":"invalid_settings"})");
      return;
    }
    memcpy(pendingSsid_, ssid.c_str(), ssidLength + 1);
    memcpy(pendingPassword_, password.c_str(), passwordLength + 1);
    reason_ = 0;
    failure_ = JoinFailure::None;
    WiFi.disconnect();
    WiFi.begin(pendingSsid_, pendingPassword_);
    joinStartedAt_ = now_;
    state_ = State::Joining;
    server_.send(202, "application/json", R"({"ok":true})");
  }
  void status() {
    StaticJsonDocument<128> out;
    out["state"] = stateName(state_);
    if (state_ == State::Failed)
      out["reason"] = describe(failure_).code;
    if (accepting())
      out["minutes_left"] = minutesLeft(now_);
    sendJson(200, out);
  }
  void sendJson(int code, JsonDocument &out) {
    char text[1600];
    serializeJson(out, text, sizeof(text));
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(code, "application/json", text);
  }

  struct Network {
    char name[33];
    bool secure;
  };
  SettingsStore &store_;
  WifiSettings &settings_;
  const FactorySettings &factory_;
  WebServer server_{80};
  DNSServer dns_;
  SetupGuard guard_;
  State state_ = State::Off;
  JoinFailure failure_ = JoinFailure::None;
  char name_[16] = {};
  char pendingSsid_[33] = {}, pendingPassword_[65] = {};
  Network found_[SetupMaxNetworks] = {};
  size_t networks_ = 0;
  volatile uint8_t reason_ = 0;
  uint32_t openedAt_ = 0, joinStartedAt_ = 0, joinedAt_ = 0, onlineAt_ = 0, scannedAt_ = 0;
  uint32_t now_ = 0;
  bool hotspotUp_ = false, joinedNow_ = false;
};
} // namespace grok
