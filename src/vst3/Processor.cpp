#include "vst3/Processor.hpp"
#include "vst3/PluginIDs.hpp"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace fruity {
Processor::Processor(){setControllerClass(kControllerCID);}
Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context){
 const auto rc=AudioEffect::initialize(context);
 if(rc!=Steinberg::kResultOk)return rc;
 addAudioOutput(STR16("MAIN"),Steinberg::Vst::SpeakerArr::kStereo);
 addEventInput(STR16("MIDI"),16);
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
 Steinberg::Vst::SpeakerArrangement* input,Steinberg::int32 numInputs,
 Steinberg::Vst::SpeakerArrangement* output,Steinberg::int32 numOutputs){
 (void)input;
 if(numInputs!=0||numOutputs!=1||!output||
    output[0]!=Steinberg::Vst::SpeakerArr::kStereo)return Steinberg::kResultFalse;
 return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup){
 if(!std::isfinite(setup.sampleRate)||setup.sampleRate<=0.0||
    setup.maxSamplesPerBlock<=0||setup.maxSamplesPerBlock>ipc::kMaxFrames)
   return Steinberg::kInvalidArgument;
 const auto rc=AudioEffect::setupProcessing(setup);
 if(rc!=Steinberg::kResultOk)return rc;
 latency_=static_cast<Steinberg::uint32>(setup.maxSamplesPerBlock);
 configured_=false;
 // Actual original engine is external. Never substitute a mock in a VST3.
 // This is a best-effort development candidate; the unvalidated bridge may
 // legitimately fail to initialize and then outputs silence.
 if(setup.sampleRate>=8000 && setup.sampleRate<=192000 &&
    std::floor(setup.sampleRate)==setup.sampleRate){
   configured_=bridge_.start(static_cast<std::uint32_t>(setup.sampleRate));
 }
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool active){
 return AudioEffect::setActive(active);
}
Steinberg::tresult PLUGIN_API Processor::setProcessing(Steinberg::TBool){
 return Steinberg::kResultOk; // Unlike AudioEffect's kNotImplemented default.
}
Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 format){
 return (format==Steinberg::Vst::kSample32||format==Steinberg::Vst::kSample64)?
  Steinberg::kResultTrue:Steinberg::kResultFalse;
}
Steinberg::uint32 PLUGIN_API Processor::getLatencySamples(){return latency_;}
Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data){
 if(data.numSamples<0)return Steinberg::kInvalidArgument;
 const int frames=data.numSamples;
 if(frames>ipc::kMaxFrames)return Steinberg::kResultFalse;
 int count=0;
 if(data.inputEvents){
   const int total=data.inputEvents->getEventCount();
   for(int i=0;i<total;i++){
     Steinberg::Vst::Event ev{};
     if(data.inputEvents->getEvent(i,ev)!=Steinberg::kResultOk)continue;
     unsigned status=0,note=0,velocity=0;
     if(ev.type==Steinberg::Vst::Event::kNoteOnEvent){
       status=0x90U|(unsigned(ev.noteOn.channel)&15U);
       note=unsigned(std::clamp(int(ev.noteOn.pitch),0,127));
       velocity=unsigned(std::clamp(int(std::lround(double(ev.noteOn.velocity)*127.0)),0,127));
     }else if(ev.type==Steinberg::Vst::Event::kNoteOffEvent){
       status=0x80U|(unsigned(ev.noteOff.channel)&15U);
       note=unsigned(std::clamp(int(ev.noteOff.pitch),0,127));
       velocity=unsigned(std::clamp(int(std::lround(double(ev.noteOff.velocity)*127.0)),0,127));
     }else continue;
     if(count>=ipc::kMaxMidi)continue; // bounded; overflow blocks release
     auto& item=events_[static_cast<size_t>(count++)];
     item.sampleOffset=std::clamp(ev.sampleOffset,0,std::max(0,frames-1));
     item.packed=status|(note<<8U)|(velocity<<16U);
   }
   // Ordered by the host sampleOffset, preserving order for equal offsets.
   for(int i=1;i<count;i++){
     auto ev=events_[static_cast<size_t>(i)];
     int j=i;
     while(j>0&&events_[static_cast<size_t>(j-1)].sampleOffset>ev.sampleOffset){
       events_[static_cast<size_t>(j)]=events_[static_cast<size_t>(j-1)];--j;
     }
     events_[static_cast<size_t>(j)]=ev;
   }
 }
 if(frames>0) {
   if(configured_)bridge_.process(frames,events_.data(),count,mix_.data());
   else std::fill_n(mix_.data(),static_cast<size_t>(frames)*2U,0.0f);
 }
 if(data.numOutputs<1||!data.outputs)return Steinberg::kResultOk;
 auto& bus=data.outputs[0];
 if(bus.numChannels!=2)return Steinberg::kResultFalse;
 if(data.symbolicSampleSize==Steinberg::Vst::kSample32){
   if(!bus.channelBuffers32||!bus.channelBuffers32[0]||!bus.channelBuffers32[1])return Steinberg::kResultFalse;
   for(int i=0;i<frames;i++){
     bus.channelBuffers32[0][i]=mix_[size_t(i)*2U];
     bus.channelBuffers32[1][i]=mix_[size_t(i)*2U+1U];
   }
 }else if(data.symbolicSampleSize==Steinberg::Vst::kSample64){
   if(!bus.channelBuffers64||!bus.channelBuffers64[0]||!bus.channelBuffers64[1])return Steinberg::kResultFalse;
   for(int i=0;i<frames;i++){
     bus.channelBuffers64[0][i]=mix_[size_t(i)*2U];
     bus.channelBuffers64[1][i]=mix_[size_t(i)*2U+1U];
   }
 }else return Steinberg::kResultFalse;
 bus.silenceFlags=0;
 return Steinberg::kResultOk;
}
Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* stream){
 if(!stream)return Steinberg::kInvalidArgument;
 Steinberg::uint32 magic=0x31534641U;
 Steinberg::int32 written=0;
 return (stream->write(&magic,4,&written)==Steinberg::kResultOk && written==4)?Steinberg::kResultOk:Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* stream){
 if(!stream)return Steinberg::kInvalidArgument;
 Steinberg::uint32 magic=0;
 Steinberg::int32 bytesRead=0;
 if(stream->read(&magic,4,&bytesRead)!=Steinberg::kResultOk||bytesRead!=4||magic!=0x31534641U)return Steinberg::kResultFalse;
 return Steinberg::kResultOk;
}
} // namespace fruity
