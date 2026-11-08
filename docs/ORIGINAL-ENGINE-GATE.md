# Original-engine compatibility gate

**Goal:** Windows x64 VST3 exposing the actual 32-bit Fruity SoundFont / LiveSynth engine without FL Studio. SFLT or a substitute SF2 synth does not meet this goal.

## Boundary and provenance

The owner-supplied archive was examined statically; it is not trusted for execution. Its history/rights remain unverified. The package also contains potentially unauthorized third-party redistribution indicators. **Never commit, publish, install, execute on a production system, bundle, or distribute its DLLs.** Research from them is not authorization to ship a wrapper dependent on a proprietary runtime. Distribution requires separate rights verification.

Observed archive DLLs (SHA-256):
- `FLSPro.dll`, PE32/x86, 261392 bytes: `4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f`.
- `Fruity Soundfont Player.dll`, PE32/x86, 2709264 bytes: `46772506fa91a124dedc93c10e4974305aeec35010154fd8a41dcd040ce2979d`.
- `Fruity SoundFont Player_x64.dll`, PE32+/x64, 3150096 bytes: `6f20cf8f94614a39d7d5faec78f601e568af5375bc253667c42011e92abab10e`.

The x64 companion appears to be a dummy; this is not an x64 engine. Neither PE validity nor an expected hash authenticates the source.

## Statically identified x86 exports

`FLSPro.dll` exports names including:
- `InitSynthInstance`
- `FreeSynthInstance`
- `LULoadSoundFont`
- `QueueShortMessage`
- `RenderVoices`
- `GetInstrPatchName`
- `SetUseReverb`
- `SetUseChorus`

Tentative x86 return-instruction stack cleanup values: Init 8 bytes, Free 4, Load 20, Queue 12, Render 36. **These do not establish parameter types, calling convention, or ownership. Do not call by guessed prototype.**

## Required work and evidence

1. Recover argument semantics from **actual player-to-FLSPro call sites** using disassembly/data flow, including initialization ownership and exception/crash paths.
2. Recover sample type, channel layout, strides, output scaling and block-size constraints of `RenderVoices`.
3. Independently confirm licensing/provenance and obtain an authorized binary before executable experiments. Isolate all untrusted Windows binaries in a disposable, disconnected VM.
4. Build an **x86-only** harness using verified prototypes; load an authorized SF2, send NoteOn/NoteOff, render finite nonzero PCM with testable pitch/envelopes. Compare repeatability at 44.1/48/96 kHz and varied block sizes.
5. Only after #4: design x64 VST3 to x86-helper IPC with bounded, measured realtime deadlines; test helper death/restart, timing, parallel instances, patch selection, UI, and project state recall.
6. Build and test Windows-x64 VST3 in GitHub Actions **without proprietary DLLs**. Do not label a stub as functional; real engine/DAW tests require an authorized local fixture.

Current gates:
- Static PE export inventory: implementation available, runtime test not yet executed.
- Verified ABI / x86 PCM render: NOT RUN.
- Native VST3 build, validator, Studio Pro host: NOT RUN.
- Distribution authorization: NOT VERIFIED.

## Static inventory

```powershell
python tools/inspect_pe.py "C:\\path\\to\\Fruity Soundfont Player.zip"
```
