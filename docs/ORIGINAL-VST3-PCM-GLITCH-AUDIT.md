# P### Original Fruity SoundFont VST3 audio continuity failure and repair

## Original uploaded proof and interpretation

The user uploaded `VST3-TEST-ERGEBNIS.txt`, `VST3-Positive.wav` and `VST3-Negative-No-Engine.wav` on 2026-11-09. The genuine source DLL was pinned as:

- FLSPro.dll SHA-256: `4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f`
- User VST3 MODULE_SHA256: `6f71fabbacd4830fe7252dee1b553455788806daf5af0331504937b451fc53db`
- Native Lab Host SHA256: `c7b4ff4bfd1b8ed6c677183a6fa0002456995388eec99cd09618b619ba178cc5`
- Lab reported `STATUS=PASS`, `ENGINE_IDENTITY=USER_ORIGINAL_MATCH`, `MIDI_RENDER_NONZERO_PCM_BYTES=594424`, `Rendered peak: 0.385841`.
- The negative without engine contained exactly zero nonzero PCM bytes.

**However the user-visible sound was heavily distorted. Independent WAV byte inspection** found 352,800 frames (8 seconds, 44.1 kHz, stereo PCM24), 689 complete 512-frame chunks; only 207 contained any nonzero PCM. At least **131 block-aligned discontinuities cut from a signal greater than 0.02 to an entire next 64-frame silent region**. The original `STATUS=PASS` had therefore established *nonzero output*, not *correct or continuous audio*. Do NOT count that report as musical playback QA PASS.

## Root cause and mitigation

`WorkerBridge::process` originally submitted a request to the x86 helper and, if processing was still outstanding, wrote all-zero PCM for the entire next block. The generic Lab host's `--render` intentionally drove VST3 `kRealtime` in an unpaced, faster-than-wall-clock offline test, creating an unrealistic workload for a nonblocking interprocess audio proxy. The asynchronous worker could not respond before the next callback; whole zeroed blocks, dropped MIDI events and hard cut edges appeared.

The development fix now adds an **explicit VST3 `kOffline` path**: the host requests kOffline, each PCM chunk is sent to the original x86 worker, and the Lab waits with a bounded 5-second response budget *outside the realtime callback*. The existing realtime path retains its nonblocking constraints. This is **not** permission to wait on IPC in a realtime DAW callback.

The generic central 125A Plugin Test Lab was extended in an isolated branch with:
- `--render-offline`: exact MIDI/audio kOffline timing, with processor setup and callback both in kOffline;
- `--render-offline-no-midi`: identical host setup with zero MIDI events as independent negative;
- `--render-realtime-paced`: kRealtime but source host waits until nominal block arrival time, allowing valid physical-time realtime comparisons.

The product oracle `tests/check_pcm_glitches.py` detects these precise 512-aligned cut-to-zero faults, and has synthetic positive, deliberate glitch and silence fixtures.

## Verified CI result with source-built mock ONLY

In GitHub Actions run [42541701486](https://github.com/challanger2000/125A-Fruity-SoundFont-Player/actions/runs/42541701486), x86 helper build, x64 VST3 build and independently compiled universal Test Lab all PASS.

- `--render-realtime-paced` native VST3 with fake FLSPro export DLL: 352,800 frames, 69,023 nonzero frames, **0 hard cuts**, 0 hard starts.
- The actual source-built user-facing BAT/PowerShell test kit with fake FLSPro: `STATUS=PASS`, `MIDI_RENDER_NONZERO_PCM_BYTES=418402`, `OFFLINE_HARD_SILENCE_CUTS=0`, `NO_MIDI_NONZERO_PCM_BYTES=0`, `NO_ENGINE_NONZERO_PCM_BYTES=0`. Engine identity correctly `MOCK_ONLY`.
- The true original `FLSPro.dll` **has NOT** been run against the new fixed VST3 build in a disposable isolated Windows machine. Therefore original-engine patched VST3 audio and full runtime QA remain **NOT RUN**.
- The realtime path is **not guaranteed free of loss** under scheduling jitter/CPU stress because the one-block nonblocking queue lacks a sufficient backlog buffer. Offline fix does not resolve this architectural limitation.

**Release approval: NO.** Preset/GUI/automation, multiple banks, hardware/DAW testing and licensing distribution remain open.
