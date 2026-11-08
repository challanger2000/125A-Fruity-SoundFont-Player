#include "vst3/PluginIDs.hpp"
#include "vst3/Processor.hpp"
#include "vst3/Controller.hpp"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
BEGIN_FACTORY_DEF("125A Systems","","")
DEF_CLASS2(INLINE_UID_FROM_FUID(fruity::kProcessorCID),
 Steinberg::PClassInfo::kManyInstances,
 kVstAudioEffectClass,
 "125A Fruity SoundFont Player",
 Steinberg::Vst::kDistributable,
 "Instrument|Sampler",
 fruity::kVersion,kVstVersionString,
 fruity::Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(fruity::kControllerCID),
 Steinberg::PClassInfo::kManyInstances,
 kVstComponentControllerClass,
 "125A Fruity SoundFont Player Controller",
 0,"",fruity::kVersion,kVstVersionString,
 fruity::Controller::createInstance)
END_FACTORY
