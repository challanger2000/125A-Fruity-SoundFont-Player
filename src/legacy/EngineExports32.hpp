#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

// Offsets recovered from original Fruity x86 player at VA 0x005CA68C.
// Raw address layout ONLY. Calling prototypes must be verified separately.
// Explicit uint32_t fields allow inspection/serialization on any build host.
namespace fruity::legacy {
struct EngineExports32 final {
    std::uint32_t module;
    std::uint32_t initSynthInstance;
    std::uint32_t freeSynthInstance;
    std::uint32_t loadSoundFont;
    std::uint32_t clearSoundFont;
    std::uint32_t getInstrPatchName;
    std::uint32_t getSubBankCount;
    std::uint32_t getSubBanksFilled;
    std::uint32_t getBankName;
    std::uint32_t setUseReverb;
    std::uint32_t setUseChorus;
    std::uint32_t setAttenuation;
    std::uint32_t setPolyphony;
    std::uint32_t allocateNote;
    std::uint32_t renderVoices;
    std::uint32_t freeNote;
    std::uint32_t queueShortMessage;
    std::uint32_t getNRPN;
    std::uint32_t setNRPN;
};
static_assert(sizeof(EngineExports32) == 0x4C);
static_assert(offsetof(EngineExports32, initSynthInstance) == 0x04);
static_assert(offsetof(EngineExports32, loadSoundFont) == 0x0C);
static_assert(offsetof(EngineExports32, renderVoices) == 0x38);
static_assert(offsetof(EngineExports32, queueShortMessage) == 0x40);
static_assert(std::is_standard_layout_v<EngineExports32>);
} // namespace fruity::legacy
