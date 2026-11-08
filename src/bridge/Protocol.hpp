#pragma once
#include <cstddef>
#include <cstdint>
// Cross-process POD: x86 helper and x64 VST3 must use identical layouts.
// Windows LONG is exactly 32 bits in both ABIs.
namespace fruity::ipc {
constexpr std::int32_t kMaxFrames=4096;
constexpr std::int32_t kMaxMidi=256;
constexpr std::int32_t kReady=1;
struct MidiMessage {std::int32_t sampleOffset;std::uint32_t packed;};
struct Shared final {
 volatile long requestSequence;
 volatile long responseSequence;
 volatile long stopRequested;
 volatile long status; // 0=initialising, 1=ready; <0=error
 long requestFrames;
 long requestEvents;
 long responseFrames;
 long responseError;
 MidiMessage midi[kMaxMidi];
 float responsePCM[2*kMaxFrames]; // interleaved L/R
};
static_assert(sizeof(long)==4,"Windows protocol requires 32-bit LONG");
static_assert(sizeof(MidiMessage)==8);
static_assert(offsetof(Shared,midi)==32);
static_assert(offsetof(Shared,responsePCM)==32+8*kMaxMidi);
static_assert(sizeof(Shared)==32+8*kMaxMidi+8*kMaxFrames);
} // namespace fruity::ipc
