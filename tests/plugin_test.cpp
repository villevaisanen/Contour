// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen
// Minimal host for actual combined-binary factories, patches and both modules.
#include <engine/Engine.hpp>
#undef PRIVATE
#include <rack.hpp>
#include <dlfcn.h>
#include <cassert>
#include <iostream>
using namespace rack;
int main() {
 Context context;context.engine=new engine::Engine;contextSet(&context);
 void* library=dlopen("./plugin.so",RTLD_NOW|RTLD_LOCAL);assert(library);
 {
  Plugin plugin;json_error_t error;
  json_t* manifest=json_load_file("plugin.json",0,&error);assert(manifest);
  plugin.fromJson(manifest);
  auto entry=reinterpret_cast<void(*)(Plugin*)>(dlsym(library,"init"));assert(entry);
  entry(&plugin);plugin.modulesFromJson(json_object_get(manifest,"modules"));json_decref(manifest);
  plugin::plugins.push_back(&plugin);
  assert(plugin.slug=="FilthAssembly"&&plugin.brand=="Filth Assembly"&&plugin.models.size()==2);
  auto* contour=plugin.getModel("Contour");auto* orbitals=plugin.getModel("Orbitals");assert(contour&&orbitals);
  auto* c=contour->createModule();auto* o=orbitals->createModule();
  assert(c->params.size()==9&&c->inputs.size()==6&&c->outputs.size()==8&&c->lights.empty());
  assert(o->params.size()==10&&o->inputs.size()==6&&o->outputs.size()==4&&o->lights.empty());
  const float mins[]={0,0,0,-5,0,-24,0,-1,-1,0};
  const float maxs[]={13,13,1,5,24,24,1,1,1,1};
  for(int i=0;i<10;++i)assert(o->paramQuantities[i]->minValue==mins[i]&&o->paramQuantities[i]->maxValue==maxs[i]);
  for(auto* m:{c,o})for(auto& output:m->outputs)output.channels=1;
  c->params[0].setValue(3);c->params[4].setValue(.42);
  json_t* data=json_object();json_object_set_new(data,"seed",json_integer(123456));c->dataFromJson(data);json_decref(data);
  json_t* saved=c->toJson();assert(std::string(json_string_value(json_object_get(saved,"plugin")))=="FilthAssembly");
  auto* restored=contour->createModule();restored->fromJson(saved);json_decref(saved);
  assert(restored->params[0].getValue()==3&&std::abs(restored->params[4].getValue()-.42)<1e-6);
  data=restored->dataToJson();assert(json_integer_value(json_object_get(data,"seed"))==123456);json_decref(data);delete restored;
  o->params[0].setValue(12);o->params[1].setValue(13);o->params[2].setValue(.3);
  saved=o->toJson();restored=orbitals->createModule();restored->fromJson(saved);json_decref(saved);
  assert(restored->params[0].getValue()==12&&restored->params[1].getValue()==13&&std::abs(restored->params[2].getValue()-.3)<1e-6);delete restored;
  o->onReset({});double energy=0;
  for(double sr:{44100.,48000.,96000.}) {
   engine::Module::SampleRateChangeEvent change{};change.sampleRate=sr;change.sampleTime=1/sr;o->onSampleRateChange(change);
   engine::Module::ProcessArgs args{};args.sampleRate=sr;args.sampleTime=1/sr;
   for(int i=0;i<int(sr);++i) {
    c->process(args);o->process(args);
    for(auto* m:{c,o})for(auto& port:m->outputs)assert(std::isfinite(port.getVoltage())&&std::abs(port.getVoltage())<=5.001);
    energy+=std::pow(o->outputs[0].getVoltage(),2);
   }
  }
  assert(energy>1);delete o;delete c;plugin::plugins.clear();
 }
 dlclose(library);
 std::cout<<"PASS combined FilthAssembly binary: both factories, simultaneous 44.1/48/96kHz, unchanged ports/ranges, Contour seed and both parameter round trips, nonzero Orbitals audio\n";
}
