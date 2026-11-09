# Owner-supplied original FLSPro.dll Windows x86 PCM run (2026-11-09)

Uploaded report: `TEST_ERGEBNIS(1).txt`. This is reported from the user's machine, NOT independent GitHub CI evidence.

Observed:
- `CHECK ZIP=OK DLL=OK BANK=OK`
- `Init=0 slot=0`
- `Polyphony=64`
- `Load=0`
- `frames=132300 nonzero=132122 RMS=0.049067 finite=1`
- `PASS: finite nonzero PCM from dynamically loaded engine`
- `PASS ORIGINAL ENGINE PCM PROBE`

**Interpretation:** The actual user-supplied native 32-bit FLSPro engine produced finite nonzero PCM with the bundled default SoundFont. This clears the direct engine-audio milestone.

**Remaining independent gates:** real original engine through the 64-bit VST3/x86-helper bridge; MIDI causes the audio (independent no-note oracle); sample/preset fidelity; continuity and MIDI integrity under realtime scheduling; DAW/ASIO and distribution rights. Original engine PCM evidence is not a VST3 release approval.

**Reporting issue:** the original BAT writes a static initial `status: NOT RUN` header which conflicts with its final PASS. This is a batch reporting defect; the recorded sound measurements and final PASS are meaningful. Correct the header and emit an explicit final status.
