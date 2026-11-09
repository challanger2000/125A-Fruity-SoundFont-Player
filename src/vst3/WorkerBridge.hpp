#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstdint>
#include "bridge/Protocol.hpp"
namespace fruity {
class WorkerBridge final {
 public:
  WorkerBridge()=default;
  ~WorkerBridge(){stop();}
  WorkerBridge(const WorkerBridge&)=delete;
  WorkerBridge& operator=(const WorkerBridge&)=delete;
  bool start(std::uint32_t sampleRate);
  void stop() noexcept;
  // Realtime: nonblocking; offline host: bounded cross-process synchronous render.
  // The latter must never be entered from the realtime audio callback.
  bool process(int frames,const ipc::MidiMessage* events,int eventCount,
               float* interleavedOutput,bool offline=false) noexcept;
  bool isReady() const noexcept;
 private:
  HANDLE mapping_=nullptr,event_=nullptr,process_=nullptr,thread_=nullptr;
  ipc::Shared* shared_=nullptr;
  long requested_=0,played_=0;
};
} // namespace fruity
