// SPDX-License-Identifier: Apache-2.0
#include "GrokGadgets.h"
#include <cassert>
#include <iostream>
struct App {grok::Rgb rgb;int executions=0;};
void state(JsonObject target,void* raw){grok::writeRgb(target,static_cast<App*>(raw)->rgb);}
grok::Error rgb(JsonObjectConst args,void* raw){auto& app=*static_cast<App*>(raw);grok::Rgb next;auto e=grok::readRgb(args,next);if(!e.code){app.rgb=next;++app.executions;}return e;}
grok::Error counter(JsonObjectConst args,void* raw){if(args.size()!=0)return {"invalid_arguments","No args"};++*static_cast<int*>(raw);return {};}
int main(){
 App app;int count=0;grok::Device device(state,&app);
 assert(device.capability("rgb.set",rgb,&app));assert(!device.capability("rgb.set",rgb,&app));
 assert(device.capability("counter.bump",counter,&count));assert(!device.capability("bad name",counter,&count));
 DynamicJsonDocument cmd(2048),ack(2048);
 deserializeJson(cmd,R"({"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":0,"g":255,"b":0,"on":true}})");
 device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["status"]=="executed");assert(app.rgb.g==255&&app.executions==1);
 device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["status"]=="executed"&&app.executions==1);
 cmd["arguments"]["r"]=3;device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["error"]["code"]=="duplicate_conflict");assert(app.executions==1);
 for(auto invalid:{"-1","256","1.5","true","\"4\""}){
  std::string json=std::string("{\"command_id\":\"temporary\",\"capability\":\"rgb.set\",\"arguments\":{\"r\":")+invalid+",\"g\":0,\"b\":0,\"on\":true}}";
  // Each invalid channel has an independent command ID.
  assert(!deserializeJson(cmd,json));cmd["command_id"]=std::string("invalid-")+std::to_string(count++);
  device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["status"]=="failed");assert(app.executions==1);
 }
 deserializeJson(cmd,R"({"command_id":"custom-1","capability":"counter.bump","arguments":{}})");
 int before=count;device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["status"]=="executed"&&count==before+1);
 cmd["command_id"]="custom-2";cmd["capability"]="unknown";device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["error"]["code"]=="unsupported_capability");
 cmd["command_id"]="invalid id";device.execute(cmd.as<JsonObjectConst>(),ack);assert(ack["error"]["code"]=="invalid_command");
 std::cout<<"device: RGB bounds/types, extension handler, malformed IDs, unsupported capability and dedup/conflict passed\n";
}
