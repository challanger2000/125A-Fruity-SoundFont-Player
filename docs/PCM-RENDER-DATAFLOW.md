# PCM output dataflow — original FLSPro engine
Date: 2026-11-08. Static inspection on the supplied 32-bit `FLSPro.dll`, no binary execution.

## Exact instructions

In `FLSPro.dll` at VA `0x10002370`, the helper is a float32 **additive mix** primitive:

```asm
10002373  mov eax,[ebp+8]    ; source pointer
10002378  mov edx,[ebp+16]   ; frame count
1000237b  imul edx,edx,8    ; 8 bytes per frame
10002380  mov edx,[ebp+12]   ; destination pointer
10002385  fld dword ptr [eax]
10002387  fld dword ptr [edx]
10002389  faddp st(1),st
1000238b  fstp dword ptr [edx]
1000238d  add eax,4
10002390  add edx,4
10002393  cmp eax,ecx
```

This **proves** the mix operation reads/writes IEEE-754 32-bit floats and spans 8 bytes for each frame, **consistent with stereo float32**. The helper also writes the updated destination pointer via argument #4. Whether the full export expects one interleaved stereo destination buffer, two planar stereo buffers, or additional auxiliary/effect buffers still depends on the surrounding call sites.

In `RenderVoices` internal renderer `0x100023B0`:
- `[EBP+0x08]` is the slot index; it indexes `0x1002A1F0`.
- `[EBP+0x18]` is used as a frame-count-like quantity.
- `[EBP+0x14]` is a pointer used as a destination for a frame copy: `REP MOVSD` + `REP MOVSB` for `8 * ESI` bytes at `0x1000257D`–`0x10002594`.
- `[EBP+0x24]` selects copy versus additive mix at `0x10002573`. When zero, direct copy; otherwise invokes `0x10002370`.
- `[EBP+0x28]` and `[EBP+0x2C]` trigger additional mixes and have a pointer-like role; they are not proven to be independent audio buses.

**Important:** calling this with arbitrarily guessed pointers is unsafe. `REP MOVS` writes unboundedly relative to a wrong frame count. The x86 harness must use guarded allocations, a real source-derived prototype, and isolated execution.

## Reproduce

```sh
objdump -d -Mintel FLSPro.dll --start-address=0x10002370 --stop-address=0x100023A4
objdump -d -Mintel FLSPro.dll --start-address=0x100023B0 --stop-address=0x100025C8
```

## Status
Float32 sample operations and 8-byte per-frame layout: **confirmed statically**.
Channel/buffer mapping, MIDI triggering and live PCM: **not yet confirmed**.
