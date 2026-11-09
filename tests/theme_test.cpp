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
using namespace rack;
struct Layers : widget::Widget {
 void draw(const DrawArgs& args) override { Widget::draw(args);Widget::drawLayer(args,1); }
};
int main(int argc,char** argv) {
 assert(argc==3); // system directory and isolated user directory
 auto* ctx=new Context;ctx->event=new widget::EventState;contextSet(ctx);
 asset::systemDir=argv[1];asset::userDir=argv[2];
 assert(glfwInit());glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
 ctx->window=new window::Window;glfwHideWindow(ctx->window->win);
 ctx->engine=new engine::Engine;ctx->scene=new app::Scene;
 void* library=dlopen("./plugin.so",RTLD_NOW|RTLD_LOCAL);assert(library);
 auto entry=reinterpret_cast<void(*)(Plugin*)>(dlsym(library,"init"));assert(entry);
 auto* plugin=new Plugin;plugin->path=".";entry(plugin);assert(plugin->models.size()==2);
 for(auto* model:plugin->models) {
  auto* m=model->createModule();ctx->engine->addModule(m);auto* widget=model->createModuleWidget(m);
  auto* panel=dynamic_cast<app::ThemedSvgPanel*>(widget->getPanel());assert(panel&&panel->lightSvg&&panel->darkSvg);
  assert(std::abs(widget->box.size.x-(model->slug=="Contour"?20:18)*RACK_GRID_WIDTH)<.01);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  engine::Module::ProcessArgs args{};args.sampleRate=48000;args.sampleTime=1.f/48000;
  for(int i=0;i<4096;++i)m->process(args);
  auto* fb=new widget::FramebufferWidget;fb->oversample=2;
  auto* layers=new Layers;fb->addChild(layers);layers->addChild(widget);
  layers->box.size=fb->box.size=widget->box.size;
  for(int i=0;i<3;++i) {
   settings::preferDarkPanels=(i==1);fb->step();
   assert(panel->svg==(i==1?panel->darkSvg:panel->lightSvg));
   fb->setDirty();fb->render(math::Vec(2,2));nvgluBindFramebuffer(fb->getFramebuffer());
   int width,height;nvgImageSize(ctx->window->vg,fb->getImageHandle(),&width,&height);assert(width>0&&height>0);
   std::vector<unsigned char> pixels(width*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);
   glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
   std::ofstream out("build/tests/"+model->slug+"-theme-"+std::to_string(i)+".ppm",std::ios::binary);
   out<<"P6\n"<<width<<' '<<height<<"\n255\n";
   for(int y=height-1;y>=0;--y)out.write(reinterpret_cast<char*>(pixels.data()+y*width*3),width*3);
   nvgluBindFramebuffer(nullptr);
  }
  delete fb; // ModuleWidget owns, unregisters and deletes its module.
 }
 delete plugin;dlclose(library);delete ctx;glfwTerminate();
 std::cout<<"PASS live module widgets, runtime SVG/control loading and light/dark/light switching for Contour and Orbitals\n";
}
