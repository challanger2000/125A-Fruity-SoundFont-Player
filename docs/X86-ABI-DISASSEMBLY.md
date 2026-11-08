# FLSPro x86 disassembly — verified findings, 2026-11-08

Source: owner-provided `Fruity Soundfont Player.zip`, `FLSPro.dll` SHA-256 `4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f`.
Static examination with GNU `objdump -d -Mintel`, no DLL execution. Image base 0x10000000.

## Initialization: 0x10002FF0

At `0x10003014`, `mov edi, [esp+0x14]` (after pushes) obtains a pointer which is passed to `0x10002AB0`. The function dereferences `[edi]` on a failure path at `0x1000303C`. Another operand is loaded at `0x10002FFC` from `[esp+0x18]`, even though the exported entry terminates in `ret 8` at `0x1000304C`.

**CRITICAL ABI ambiguity:** `ret 8` alone **does not** prove this export takes *exactly two* arguments: its body reads from a stack position beyond those two nominal stack parameters. A possible explanation is nonstandard caller/callee argument division, but this is not established. **Do not compile or invoke a guessed two-argument prototype.** Reverse-engineer the authentic player call site first.

The code checks `0x10002AB0`'s return value and may call `0x100074A0` with `[edi]`. This is not yet enough to prove object layout or the lifetime of the pointer.

## Instance lifetime: 0x10003050

`FreeSynthInstance` loads its first parameter from `[esp+8]` after `push edi`, checks that index against `0 <= index < 0x80` and indexes `0x1002A1F0 + 4*index`. A null slot returns -1; an out-of-range index returns -2. The function calls `0x10007560`, loops 0x80 times calling exported `0x1000E310`, then calls `0x10002D90`, and returns 0 on the normal path.

**Meaningful deduction:** The "instance" parameter is **an integer slot index (0–127)**, not an opaque engine pointer, for this export. Independent per-instance behavior must still be established; multiple active contexts and global state need review.

## MIDI dispatch: 0x10003110

`QueueShortMessage` forwards three stack arguments plus a constant zero to `0x10002E80` and terminates with `ret 0x0C`. Its internal callee uses its first stack argument (at `[esp+8]` after `push ebp`) as an index into the SAME `0x1002A1F0` instance table, then checks for a null slot and returns error 5 if absent.

This establishes the presence of an instance-index argument, but the exact exported argument order, MIDI packing and timestamp semantics remain **unverified**.

## PCM rendering: 0x100030C0

`RenderVoices` forwards the nine exported stack fields to `0x100023B0` with additional constants and returns via `ret 0x24`. It inserts a zero and a one and consumes 0x30 bytes of temporary stack arguments from the inner call. **Nine stack fields does not prove nine independent buffers nor their types.** Output type, channels, scale, and sample-rate semantics are not established.

## Reproduce

```bash
objdump -d -Mintel FLSPro.dll --start-address=0x10002ff0 --stop-address=0x10003130
objdump -d -Mintel FLSPro.dll --start-address=0x10002e80 --stop-address=0x10002ff0
```

## Next hard gate

Find the exported `InitSynthInstance` and `RenderVoices` calls in the **player DLL** (including indirect GetProcAddress dispatch), reconstruct actual stack/register arguments and buffer contracts, and only then write an x86 harness. Unknown torrent-sourced binaries must be executed only in a disposable isolated Windows environment. No fabricated audio-test PASS.
