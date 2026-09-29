// SPDX-License-Identifier: Apache-2.0
#include "GrokGadgets.h"
#include "C124.h"
#include <iostream>
grok::Rgb rgb;
void state(JsonObject target,void*){grok::writeRgb(target,rgb);target.createNestedObject("button")["pressed"]=false;}
grok::Error handler(JsonObjectConst args,void*){return grok::readRgb(args,rgb);}
void emit(const JsonDocument& doc){std::string text;serializeJson(doc,text);std::cout<<text<<'\n';}
int main(){
 grok::Device device(state,nullptr);device.capability("rgb.set",handler,nullptr);
 DynamicJsonDocument doc(4096),command(4096);
 doc["type"]="hello";doc["protocol_version"]=grok::ProtocolVersion;
 auto d=doc.createNestedObject("device");d["device_id"]="atoms3-lite-1";d["model"]=grok::c124::Model;
 d["firmware_version"]="0.1.0";d["boot_id"]="boot-host-test";d["simulated"]=true;
 auto caps=d.createNestedArray("capabilities");device.capabilities(caps);caps.add("button");caps.add("state");device.state(d.createNestedObject("state"));emit(doc);
 deserializeJson(command,R"({"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":0,"g":255,"b":0,"on":true}})");
 device.execute(command.as<JsonObjectConst>(),doc);emit(doc);
 doc.clear();doc["type"]="event";doc["event_id"]="boot-host-test-1";doc["name"]="button";doc.createNestedObject("data")["pressed"]=true;emit(doc);
 doc.clear();doc["type"]="poll";emit(doc);
 doc.clear();doc["type"]="state";device.state(doc.createNestedObject("state"));emit(doc);
}
