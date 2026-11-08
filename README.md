# 125A Fruity SoundFont Player – original-engine bridge

**Windows x64 VST3 DEVELOPMENT CANDIDATE. NOT YET FUNCTIONAL OR RELEASE READY.**

This project contains a real Steinberg VST3 instrument processor, an x86 Windows helper and a shared-memory bridge to dynamically call the original 32-bit FLSPro SoundFont engine. It does not replace the requested engine with SFLT or another player.

The original proprietary DLL is deliberately absent from the public repository and GitHub Actions artifacts. It must be supplied locally, together with a user-provided SoundFont bank. The user-supplied binary came from an unverified torrent; execute it only within a disposable isolated Windows VM.

## Bundle layout

    125A Fruity SoundFont Player.vst3/
      Contents/
        x86_64-win/125A Fruity SoundFont Player.vst3
        Resources/125A-Fruity-Helper32.exe
        Resources/FLSPro.dll           (supplied locally; NOT included)
        Resources/SynthBank.sf2        (supplied locally; NOT included)

Run the included prepare-local-bundle.bat only in an isolated VM to add your local DLL and SF2 to a development bundle.

## Build

Windows Visual Studio 2022, CMake 3.25+, C++23. Win32: cmake -S . -B build32 -A Win32; cmake --build build32 --config Release; ctest --test-dir build32 -C Release --output-on-failure.

Win64: CMake with -A x64 and -DVST3_SDK_ROOT pointing to an official recursively checked-out Steinberg SDK 3.8.1. Build target FruitySoundFont.

The branch workflow compiles the x86 worker and the x64 native VST3 without the proprietary engine. Its fake FLSPro test is only an ABI and WAV-output harness regression; it cannot prove the original SoundFont engine produces sound.

## Explicit blockers

The real original x86 engine has **NOT** rendered verified audio in an isolated Windows environment. The four optional RenderVoices parameters require a real test. The first bridge uses a one-block pipeline, drops MIDI on worker congestion and cannot guarantee audio continuity across varying host blocks. NoteOff retry, realtime deadlines, host latency accuracy, plugin presets/state, sample-rate behavior, GUI, DAW/ASIO testing and distribution rights are NOT VERIFIED. Never call a passing mock test proof of actual synthesis. No release approval.

Follow the 125A Engineering START-HERE.md and its complete mandatory guardrails. Code development takes place on the research/original-engine-abi branch, not main.
