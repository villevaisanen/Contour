// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen
// Optional OpenGL host; internal APIs are used only to construct a test window.
#include <engine/Engine.hpp>
#include <app/Scene.hpp>
#include <window/Window.hpp>
#include <widget/FramebufferWidget.hpp>
#undef PRIVATE
#include <rack.hpp>
#include <dlfcn.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
using namespace rack;
struct Layers : widget::Widget {
 void draw(const DrawArgs& args) override { Widget::draw(args);Widget::drawLayer(args,1); }
};
std::vector<unsigned char> rgba(widget::FramebufferWidget* fb) {
 nvgluBindFramebuffer(fb->getFramebuffer());
 int w,h;nvgImageSize(APP->window->vg,fb->getImageHandle(),&w,&h);
 std::vector<unsigned char> out(w*h*4);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,out.data());
 nvgluBindFramebuffer(nullptr);return out;
}
int main(int argc,char** argv) {
 std::cout<<std::unitbuf;
 assert(argc==3||argc==4||argc==5); // system directory and isolated user directory
 auto* ctx=new Context;ctx->event=new widget::EventState;contextSet(ctx);
 asset::systemDir=argv[1];asset::userDir=argv[2];
 assert(glfwInit());glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
 ctx->window=new window::Window;glfwHideWindow(ctx->window->win);
 ctx->engine=new engine::Engine;ctx->scene=new app::Scene;
 void* library=dlopen("./plugin.so",RTLD_NOW|RTLD_LOCAL);assert(library);
 auto entry=reinterpret_cast<void(*)(Plugin*)>(dlsym(library,"init"));assert(entry);
 auto* plugin=new Plugin;plugin->path=".";entry(plugin);assert(plugin->models.size()==2);
 for(auto* model:plugin->models) {
  if(argc==5 && std::string(argv[4])=="--benchmark")continue;
  if(argc>=4&&model->slug!=argv[3])continue;
  auto* m=model->createModule();ctx->engine->addModule(m);auto* widget=model->createModuleWidget(m);
  auto* panel=dynamic_cast<app::ThemedSvgPanel*>(widget->getPanel());assert(panel&&panel->lightSvg&&panel->darkSvg);
  assert(std::abs(widget->box.size.x-(model->slug=="Contour"?20:18)*RACK_GRID_WIDTH)<.01);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  engine::Module::ProcessArgs args{};args.sampleRate=48000;args.sampleTime=1.f/48000;
  for(int i=0;i<4096;++i)m->process(args);
  auto* fb=new widget::FramebufferWidget;fb->oversample=2;
  auto* layers=new Layers;fb->addChild(layers);layers->addChild(widget);
  layers->box.size=fb->box.size=widget->box.size;
  // Run real DSP and UI frames together so screenshots include live displays.
  for(int frame=0;frame<120;++frame) {
   for(int sample=0;sample<800;++sample)m->process(args);
   fb->step();fb->setDirty();fb->render(math::Vec(1,1));
   std::this_thread::sleep_for(std::chrono::milliseconds(17));
  }
  for(int i=0;i<3;++i) {
   settings::preferDarkPanels=(i==1);fb->step();
   assert(panel->svg==(i==1?panel->darkSvg:panel->lightSvg));
   // Settle nested panel framebuffer invalidations after theme/scale changes.
   for(int settle=0;settle<3;++settle) {fb->step();fb->setDirty();fb->render(math::Vec(4,4));}
   nvgluBindFramebuffer(fb->getFramebuffer());
   int width,height;nvgImageSize(ctx->window->vg,fb->getImageHandle(),&width,&height);assert(width>0&&height>0);
   std::vector<unsigned char> pixels(width*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);
   glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
   std::ofstream out("build/tests/"+model->slug+"-theme-"+std::to_string(i)+".ppm",std::ios::binary);
   out<<"P6\n"<<width<<' '<<height<<"\n255\n";
   for(int y=height-1;y>=0;--y)out.write(reinterpret_cast<char*>(pixels.data()+y*width*3),width*3);
   nvgluBindFramebuffer(nullptr);
   if(model->slug=="Orbitals" && i<2 && argc!=5) {
    widget::Widget* display=nullptr;
    for(auto* child:widget->children)
     if(std::abs(child->box.size.x-mm2px(59.f))<.01f)display=child;
    assert(display);
    auto painted=rgba(fb);
    // Repainting an identical frame must not accumulate alpha or stale pixels.
    fb->setDirty();fb->render(math::Vec(4,4));assert(rgba(fb)==painted);
    display->visible=false;fb->setDirty();fb->render(math::Vec(4,4));auto backing=rgba(fb);
    const int x0=int(display->box.pos.x*4),y0=int(display->box.pos.y*4);
    const int x1=int((display->box.pos.x+display->box.size.x)*4)+1;
    const int y1=int((display->box.pos.y+display->box.size.y)*4)+1;
    int unchanged=0,inside=0,outsideChanges=0;
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
     int offset=((height-1-y)*width+x)*4;
     bool equal=std::equal(painted.begin()+offset,painted.begin()+offset+4,backing.begin()+offset);
     if(x>=x0&&x<x1&&y>=y0&&y<y1) {++inside;unchanged+=equal;}
     else outsideChanges+=!equal;
    }
    assert(outsideChanges==0);assert(unchanged>inside*.55);assert(unchanged<inside*.995);
    std::cout<<"PASS Orbitals transparent composite: "<<unchanged<<" / "<<inside
             <<" panel pixels unchanged, zero pixels changed outside display, stable redraw\n";
    display->visible=true;
   }
  }
  if(model->slug=="Orbitals") {
   // Exercise distinct fields and extreme scan radii with actual widgets and DSP.
   for(int state : {0,6,9,13})for(int radius : {0,6,24}) {
    m->params[0].setValue(state);m->params[1].setValue(state);m->params[4].setValue(radius);
    for(int sample=0;sample<4800;++sample)m->process(args);
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    for(int dark=0;dark<2;++dark) {
     settings::preferDarkPanels=dark;
     for(int settle=0;settle<3;++settle) {fb->step();fb->setDirty();fb->render(math::Vec(2,2));}
     auto pixels=rgba(fb);int w,h;nvgImageSize(ctx->window->vg,fb->getImageHandle(),&w,&h);
     std::ofstream out("build/tests/Orbitals-field-"+std::to_string(state)+"-radius-"+std::to_string(radius)+"-"+std::to_string(dark)+".ppm",std::ios::binary);
     out<<"P6\n"<<w<<' '<<h<<"\n255\n";
     for(int y=h-1;y>=0;--y)for(int x=0;x<w;++x)out.write(reinterpret_cast<char*>(pixels.data()+(y*w+x)*4),3);
    }
   }
  }
  delete fb; // ModuleWidget owns, unregisters and deletes its module.
 }
 // Eight independent Orbitals widgets, animated in one clipped 4x2 framebuffer.
 // Time UI step + real GPU completion, excluding DSP preparation and frame pacing.
 auto* orbitalModel=plugin->getModel("Orbitals");assert(orbitalModel);
 for(int instances : {1,8}) {
  std::cout<<"GUI benchmark start "<<instances<<" instances\n";
  std::vector<engine::Module*> modules;
  auto* fb=new widget::FramebufferWidget;auto* layers=new Layers;fb->addChild(layers);
  layers->box.size=fb->box.size=math::Vec(270*(instances==1?1:4),380*(instances==1?1:2));
  for(int i=0;i<instances;++i) {
   auto* m=orbitalModel->createModule();m->id=i+1;ctx->engine->addModule(m);modules.push_back(m);
   auto* mw=orbitalModel->createModuleWidget(m);mw->box.pos=math::Vec((i%4)*270,(i/4)*380);layers->addChild(mw);
  }
  std::vector<double> times;
  engine::Module::ProcessArgs args{};args.sampleRate=48000;args.sampleTime=1.f/48000;
  for(int frame=0;frame<90;++frame) {
   for(auto* m:modules)for(int s=0;s<800;++s)m->process(args);
   auto start=std::chrono::steady_clock::now();
   fb->step();fb->setDirty();fb->render(math::Vec(2,2));glFinish();
   if(frame>=30)times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
   std::this_thread::sleep_for(std::chrono::milliseconds(17));
  }
  std::sort(times.begin(),times.end());
  std::cout<<"GUI_PERF "<<instances<<' '<<times[times.size()/2]<<' '<<times.back()<<" ms/frame (median/max)\n";
  delete fb;
 }
 delete plugin;dlclose(library);delete ctx;glfwTerminate();
 std::cout<<"PASS live module widgets, runtime SVG/control loading and light/dark/light switching for Contour and Orbitals\n";
}
