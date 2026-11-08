# Original player engine invocation: concrete call sites

Source: `Fruity Soundfont Player.dll` (PE x86); disassembled statically, no execution. This file contains the **actual API callers**, not only export names.

## InitSynthInstance call at VA 0x005DD500

At `0x005DD503`, `EAX = [EBX+0x9C]`; at `0x005DD509`, `EAX = [EAX+4]`; that value is **pushed first** (export argument #2).
At `0x005DD50D`, `EAX = &([EBX+0xF0])`; that address is **pushed second** (export argument #1).
At `0x005DD51B`, it calls `[[global player]+0x98]` = exported `InitSynthInstance`.
Immediately after, it reads the resulting `[EBX+0xF0]` and pushes it to an indirect call at `+0xC0` (exported `SetPolyphony`) with an additional `0` argument.

**Established call signature shape:**
```c
// Arguments and return are 32 bits; exact pointee type for config not established.
// This is a reverse-engineered declaration for analysis, not an executable interface.
int32_t __stdcall InitSynthInstance(int32_t* out_instance_slot, void* configuration);
```
The first parameter is **an output slot index**, not a pre-existing opaque object. The second comes from a configuration object's field `+4`; its layout and ownership need reconstruction. DLL export ends with `ret 8`.

## RenderVoices call at VA 0x005DE854

The player pushes **nine 32-bit parameters** for the exported `RenderVoices` in right-to-left stack order:
- argument 9: `[EDI+0x114]`
- argument 8: `[EDI+0x108]`
- argument 7: `[EBP-0x4]`
- argument 6: `[EBP-0x8]`
- argument 5: immediate `1`
- argument 4: `*([EBP+0x10])`
- argument 3: `[EBP+0x0C]`
- argument 2: immediate `-1`
- argument 1: `[EDI+0xF0]` (the instance-slot output of Init)

The call's instruction is exactly `call DWORD PTR [EAX+0xCC]`. DLL `RenderVoices` has `ret 0x24` (36 bytes). Thus we have **exact stack argument count and all caller-side origins**. Audio buffer element type and the roles of arguments #3/#4/#6–#9 are not yet proven.

## QueueShortMessage call at VA 0x005DE2DA

```asm
push 0
mov edx, [player+0xf8]
add edx, 0xb0
add edx, 0x100
mov ecx, [player+0xcc]
shl ecx, 16
add edx, ecx
push edx
push [player+0xf0]      ; instance slot
call [engine+0xd4]       ; QueueShortMessage
```

This verifies **three 32-bit parameters** and identifies:
- argument 1: instance-slot integer;
- argument 2: 32-bit packed value derived from `[player+0xF8] + 0x1B0 + ([player+0xCC] << 16)`;
- argument 3: zero.

The +0xB0/+0x100 constants are identical to arithmetic `+0x1B0`; the packed bytes should be interpreted only after tracing the behavior at call sites. **Do not call argument #2 plain MIDI without that verification.** Export ends with `ret 0x0C`.

## SoundFont loading at VA 0x005DD7DE

At `0x005DD7C1`–`0x005DD7DE`, the caller pushes five stack arguments:
1. slot = `[EBX+0xF0]`
2. `0`
3. `0`
4. a pointer produced by `0x00408CA8` from a prepared path/string at `[EBP-8]`
5. `0`

Engine entry = `+0xA0` = `LULoadSoundFont`. The full path and its text encoding remain under investigation; likely this is a native Delphi string conversion. Don't hard-code ANSI/UTF-16 without verifying.

## Consequences for implementation

1. A standalone x86 harness can avoid reimplementing the FL Studio hosting API, **provided** the initialization configuration object, PCM buffer contract and path encoding are fully resolved.
2. The existing original player demonstrates a concrete end-to-end invocation chain: **Init output slot → SoundFont load / MIDI dispatch → RenderVoices**, all through FLSPro's 19-function table.
3. Do not call the untrusted DLL in a normal Windows workstation process. Begin dynamic experiments only after ABI reconstruction, in a disposable isolated VM with a trust/rights decision.

## Reproduction

```bash
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5dd500 --stop-address=0x5dd540
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5de819 --stop-address=0x5de85b
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5de2ac --stop-address=0x5de2e1
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5dd7c1 --stop-address=0x5dd7e5
```

**This is static ABI evidence, not a PCM playback test or Windows x64 VST3 build.**
