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
- This paragraph describes the **earlier CI-only checkpoint**. Genuine original-engine acceptance results arrived later (below), so no longer regard the original-engine offline or paced 44.1 kHz test cases as NOT RUN; comprehensive runtime QA remains NOT VERIFIED.
- The realtime path is **not guaranteed free of loss** under scheduling jitter/CPU stress because the one-block nonblocking queue lacks a sufficient backlog buffer. Offline fix does not resolve this architectural limitation.

**Release approval: NO.** Preset/GUI/automation, multiple banks, hardware/DAW testing and licensing distribution remain open.

## New original-engine Windows evidence, user-provided 2026-11-09

1. First owner-local corrected VST3 file `VST3-TEST-ERGEBNIS(1).txt`: `STATUS=PASS`, original engine identity confirmed, **offline 0 hard cuts**, 1,912,191 nonzero PCM bytes, no MIDI and no engine both 0 nonzero bytes. This does **not** cover realtime.
2. Follow-up `VST3-TEST-ERGEBNIS(2).txt`: actual FLSPro.dll original SHA-256 `4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f` and VST3 module SHA-256 `b1390dadf88f48a506ccd80df10f0783f42e80721c9d84b8deb3f542283bdba2`; 44,100 Hz, stereo PCM24, 512 samples/block, 4 bars 120 BPM, 352,800 frames, 32 MIDI events. Offline: 1,912,191 nonzero PCM bytes and 0 block-aligned hard cuts. Paced realtime: 1,910,199 nonzero PCM bytes, 0 block-aligned hard cuts, `REALTIME_PACED_ENGINE_GATE=PASS`. Both peak 0.263827. No MIDI: 0; no engine: 0. Host execution exit code 0 in all four cases. `STATUS=PASS`, `RELEASE_APPROVED=NO`.

### Coverage boundaries

The zero-hard-cut metric comes from a selective detector targeting 512-frame-aligned sudden mute of the next 64 frames, not a full waveform differential against a gold reference. The small difference in nonzero PCM bytes is not enough to establish exact sample equivalence or synchronization. A single paced host cycle also does not prove realtime reliability during CPU/DPC interruptions, especially given the single outstanding worker request and silent MIDI drop when busy; the independent mock-based paced CI previously measured isolated hard cuts (1–2) even with the same architecture. DAW/ASIO, varying buffer sizes, 48/96 kHz, preset changes, automation and repeated stress tests remain NOT RUN/NOT VERIFIED. Retain [Issue #1](https://github.com/challanger2000/125A-Fruity-SoundFont-Player/issues/1) as OPEN until these conditions are properly addressed, not only until the single successful user test.

**Current release status: NO.** This is a validated prototype for the specified host modes, not a production-ready plugin.
