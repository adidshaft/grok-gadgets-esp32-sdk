// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "GrokCore.h"
#include <ArduinoJson.h>
namespace grok {
struct Error { const char* code = nullptr; const char* message = nullptr; };
using Handler = Error (*)(JsonObjectConst arguments, void* context);
using StateWriter = void (*)(JsonObject state, void* context);
inline bool validId(const char* id) {
 if (!id || !*id || strlen(id)>64) return false;
 for (size_t i=0; id[i]; ++i) {
  char c=id[i]; bool alnum=(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9');
  if (!alnum && (i==0 || (c!='.'&&c!='_'&&c!=':'&&c!='-'))) return false;
 }
 return true;
}
class Device {
 public:
  Device(StateWriter writer, void* context) : writer_(writer), context_(context) {}
  bool capability(const char* name, Handler handler, void* context) {
   if (!validId(name) || !handler || count_==16) return false;
   for(size_t i=0;i<count_;++i) if(!strcmp(entries_[i].name,name)) return false;
   entries_[count_++]={name,handler,context}; return true;
  }
  void capabilities(JsonArray array) const { for(size_t i=0;i<count_;++i) array.add(entries_[i].name); }
  void state(JsonObject target) const { writer_(target,context_); }
  // Never re-execute a retained ID. Changed arguments produce an explicit conflict.
  void execute(JsonObjectConst command, JsonDocument& ack) {
   ack.clear(); auto out=ack.to<JsonObject>();
   const char* id=command["command_id"] | "";
   const char* cap=command["capability"] | "";
   out["type"]="ack"; out["command_id"]=id;
   Error error;
   char fingerprint[MaxFrame+1];
   if(!validId(id)||!validId(cap)||!command["arguments"].is<JsonObjectConst>()) error={"invalid_command","Invalid command envelope"};
   else if(measureJson(command)>MaxFrame) error={"invalid_command","Command too large"};
   else {
    serializeJson(command,fingerprint,sizeof(fingerprint));
    for(auto& cached: cache_) if(!strcmp(id,cached.id)) {
     if(!strcmp(fingerprint,cached.command)) { deserializeJson(ack,cached.ack); return; }
     error={"duplicate_conflict","Command ID already has different arguments"}; break;
    }
    if(!error.code) {
     error={"unsupported_capability","Capability has no command handler"};
     for(size_t i=0;i<count_;++i) if(!strcmp(cap,entries_[i].name)) { error=entries_[i].handler(command["arguments"],entries_[i].context); break; }
    }
   }
   out["status"]=error.code ? "failed" : "executed";
   state(out.createNestedObject("state"));
   if(error.code) { auto e=out.createNestedObject("error");e["code"]=error.code;e["message"]=error.message; }
   if(validId(id) && !errorConflict(error)) {
    auto& entry=cache_[next_];next_=(next_+1)%8;
    strcpy(entry.id,id);
    if(measureJson(command)<=MaxFrame) serializeJson(command,entry.command,sizeof(entry.command));
    else entry.command[0]=0;
    serializeJson(ack,entry.ack,sizeof(entry.ack));
   }
  }
 private:
  static bool errorConflict(const Error& error) { return error.code && !strcmp(error.code,"duplicate_conflict"); }
  struct Entry {const char* name=nullptr;Handler handler=nullptr;void* context=nullptr;} entries_[16];
  struct Cached {char id[65]={};char command[MaxFrame+1]={};char ack[MaxFrame+1]={};} cache_[8];
  size_t count_=0,next_=0;
  StateWriter writer_;void* context_;
};
struct Rgb {uint8_t r=0,g=0,b=0;bool on=false;};
inline Error readRgb(JsonObjectConst a,Rgb& value) {
 if(a.size()!=4 || !a["on"].is<bool>()) return {"invalid_arguments","Expected r,g,b integers 0..255 and on boolean"};
 for(const char* key:{"r","g","b"}) if(!a[key].is<int>()||a[key].as<int>()<0||a[key].as<int>()>255) return {"invalid_arguments","RGB channel outside 0..255"};
 value={a["r"].as<uint8_t>(),a["g"].as<uint8_t>(),a["b"].as<uint8_t>(),a["on"].as<bool>()};return {};
}
inline void writeRgb(JsonObject state,const Rgb& value) {
 auto rgb=state.createNestedObject("rgb");rgb["r"]=value.r;rgb["g"]=value.g;rgb["b"]=value.b;rgb["on"]=value.on;
}
} // namespace grok
