#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
namespace fruity {
class Controller final:public Steinberg::Vst::EditController {
public:
 static Steinberg::FUnknown* createInstance(void*){
  return static_cast<Steinberg::Vst::IEditController*>(new Controller());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
  return EditController::initialize(context);
 }
 Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) override {
  // The development prototype has no user-adjustable parameters.
  return Steinberg::kResultOk;
 }
 Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString) override {
  return nullptr; // no misleading non-functional custom GUI
 }
};
}
