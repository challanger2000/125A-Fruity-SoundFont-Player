#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "vst3/WorkerBridge.hpp"
#include <array>
namespace fruity {
class Processor final:public Steinberg::Vst::AudioEffect {
public:
 Processor();
 static Steinberg::FUnknown* createInstance(void*) {
  return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
 }
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
 Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
 Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
 Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
 Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
 Steinberg::tresult PLUGIN_API setBusArrangements(
      Steinberg::Vst::SpeakerArrangement*,Steinberg::int32,
      Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) override;
 Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
 Steinberg::uint32 PLUGIN_API getLatencySamples() override;
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
private:
 WorkerBridge bridge_;
 std::array<ipc::MidiMessage,ipc::kMaxMidi> events_{};
 std::array<float,ipc::kMaxFrames*2> mix_{};
 Steinberg::uint32 latency_=0;
 bool configured_=false;
};
}
