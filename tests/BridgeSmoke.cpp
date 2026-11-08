// Full native Windows x64 -> x86 helper -> x86 mock FLSPro -> stereo PCM
// fixture. Test-only, do not substitute this fake synth in a release.
#include "vst3/WorkerBridge.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <windows.h>

int main() {
    fruity::WorkerBridge bridge;
    if (!bridge.start(44100)) {
        std::fputs("FAIL worker startup: check file paths, process exit/status and x86 exports\n", stderr);
        return 2;
    }
    if (!bridge.isReady()) {
        std::fputs("FAIL worker did not acknowledge ready before audio begins\n", stderr);
        return 3;
    }

    constexpr int frames = 512;
    std::array<float, frames * 2> output{};
    const fruity::ipc::MidiMessage noteOn{0, 0x00643C90U};
    const fruity::ipc::MidiMessage noteOff{0, 0x00003C80U};
    int audibleBlocks = 0;
    int unexpectedTailBlocks = 0;
    double energy = 0.0;
    double maxSample = 0.0;
    for (int i = 0; i < 100; ++i) {
        const auto* event = i == 2 ? &noteOn : i == 30 ? &noteOff : nullptr;
        if (!bridge.isReady()) {
            std::fputs("FAIL worker lost readiness during test\n", stderr);
            return 4;
        }
        (void)bridge.process(frames, event, event ? 1 : 0, output.data());
        int active = 0;
        for (float sample : output) {
            if (!std::isfinite(sample)) {
                std::fputs("FAIL nonfinite PCM\n", stderr);
                return 5;
            }
            if (sample != 0) ++active;
            if (i >= 5 && i < 30) {
                energy += double(sample) * double(sample);
                maxSample = std::max(maxSample, std::abs(double(sample)));
            }
        }
        if (i >= 5 && i < 30 && active > 64) ++audibleBlocks;
        if (i >= 50 && i < 90 && active > 0) ++unexpectedTailBlocks;
        Sleep(5); // Pacing belongs only in this independent test, NOT the VST3 audio thread.
    }
    bridge.stop();
    std::printf("MIDI_BRIDGE_PCM audibleBlocks=%d offTailBlocks=%d energy=%f peak=%f\n",
                audibleBlocks, unexpectedTailBlocks, energy, maxSample);
    if (audibleBlocks < 10 || !(energy > 1.) || !(maxSample > 0.01)) {
        std::fputs("FAIL MIDI NoteOn did not produce sustained real interprocess PCM\n", stderr);
        return 6;
    }
    if (unexpectedTailBlocks) {
        std::fputs("FAIL MIDI NoteOff did not stop mock PCM\n", stderr);
        return 7;
    }
    std::puts("PASS source-built x86 helper and fake export DLL, PCM and MIDI lifecycle");
    return 0;
}
