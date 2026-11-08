# Player ↔ FLSPro dynamic binding — disassembly evidence

2026-11-08. Static reverse engineering only. Exact original DLL: SHA-256 `46772506fa91a124dedc93c10e4974305aeec35010154fd8a41dcd040ce2979d`.

## Decisive linkage: original player already provides an engine adapter

The original x86 player does **not** list `FLSPro.dll` among static PE imports. However, it dynamically resolves its actual exported interface:

- Source **VA `0x005CA68C`** accepts an engine module handle in `EAX` and a writable function-table pointer in `EDX`.
- At `0x005CA698`, it invokes a helper at `0x00425E7C` to obtain the module handle; `0x005CA6A1` stores that handle in `[EBX]`.
- Each name (UTF-16) is pushed with the module handle to a resolver at **VA `0x00410F28`**; returned addresses populate **`[EBX+0x04]` through `[EBX+0x48]`**, consecutively.
- At **`0x005DCCA7`**, the engine-table initializer is called using `EDX = [this+0x94]`, so **the table begins at the player object's offset +0x94** (module handle at +0x94, exports at +0x98 onward).
- The nearby UTF-16 `FLSPro.dll` literal resides at **VA `0x005DCE4C`**, passed into the initialization path at **`0x005DCC94`**.

This is the **real bridge** between proprietary Fruity wrapper and LiveSynth engine, not a hypothetical connection based on unrelated names.

## Resolved native ABI function-pointer table

Offsets below are relative to the beginning of the module/exports table passed as `EDX` to `0x005CA68C`; add **0x94** to obtain offsets relative to the containing player object.

| Table offset | Native export |
| --- | --- |
| 0x00 | FLSPro module handle |
| 0x04 | InitSynthInstance |
| 0x08 | FreeSynthInstance |
| 0x0C | LULoadSoundFont |
| 0x10 | LUClearSoundFont |
| 0x14 | GetInstrPatchName |
| 0x18 | GetSubBankCount |
| 0x1C | GetSubBanksFilled |
| 0x20 | GetBankName |
| 0x24 | SetUseReverb |
| 0x28 | SetUseChorus |
| 0x2C | SetAttenuation |
| 0x30 | SetPolyphony |
| 0x34 | AllocateNote |
| 0x38 | RenderVoices |
| 0x3C | FreeNote |
| 0x40 | QueueShortMessage |
| 0x44 | GetNRPN |
| 0x48 | SetNRPN |

These offsets follow from **consecutive exact PUSH string address → resolver call → MOV to [EBX+offset] pairs** in the disassembly at `0x005CA6AB`–`0x005CA7C6`, with UTF-16 names at `0x005CA7D0`–`0x005CA9D0`. This is a much stronger result than simply seeing names in a DLL.

## Calling sites to recover next

Search x86 player disassembly for calls through a function-pointer table tied to an object whose **table begins at object+0x94**:
- InitSynthInstance: +0x98
- LULoadSoundFont: +0xA0
- RenderVoices: +0xCC
- QueueShortMessage: +0xD4

Delphi-compiled code commonly uses `EAX`/`EDX`/`ECX` for its *own* helper calls, but the DLL exports can use their own stack convention. Do not mistake the caller's Delphi register convention for the DLL's ABI.

## Reproduction (no execution)

```bash
strings -el -t x "Fruity Soundfont Player.dll" | grep -E "InitSynthInstance|RenderVoices|FLSPro.dll"
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5ca68c --stop-address=0x5ca7cd
objdump -d -Mintel "Fruity Soundfont Player.dll" --start-address=0x5dcc83 --stop-address=0x5dccae
```

**Status:** full export binding mapped; argument contracts and genuine PCM rendering still NOT VERIFIED. None of these experiments requires executing or distributing the untrusted binary.
