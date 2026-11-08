#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
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
  // Realtime path: never waits, allocates, or loads files.
  // One-block pipeline is a candidate, not validated realtime audio.
  bool process(int frames,const ipc::MidiMessage* events,int eventCount,
               float* interleavedOutput) noexcept;
  bool isReady() const noexcept;
 private:
  HANDLE mapping_=nullptr,event_=nullptr,process_=nullptr,thread_=nullptr;
  ipc::Shared* shared_=nullptr;
  long requested_=0,played_=0;
};
} // namespace fruity
