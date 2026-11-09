#!/usr/bin/env python3
"""125A Fruity SoundFont PCM continuity gate: strict, independent WAV byte oracle.

Rejects the original captured defect: active waveform abruptly jumps to an
entirely zeroed 512-sample block at an exact audio block boundary.
The oracle reports only continuity evidence; it cannot prove tonal fidelity.
"""
import argparse
import math
from pathlib import Path
import struct
import sys
import wave

RATE=44100
CHANNELS=2
WIDTH=3
BLOCK=512
CUTOFF_PCM=round(0.02*(2**23-1))
PROBE_FRAMES=64

def signed24(buf,at):
    value=buf[at] | (buf[at+1]<<8) | (buf[at+2]<<16)
    return value-(1<<24) if value & (1<<23) else value

def inspect(path):
    with wave.open(str(path),"rb") as input_file:
        nch=input_file.getnchannels()
        width=input_file.getsampwidth()
        sr=input_file.getframerate()
        n=input_file.getnframes()
        if (nch,width,sr)!=(CHANNELS,WIDTH,RATE):
            raise ValueError("Require PCM24 44.1kHz stereo")
        raw=input_file.readframes(n)
    if len(raw)!=n*CHANNELS*WIDTH or n < 2*BLOCK:
        raise ValueError("WAV data incomplete or insufficient")
    framebytes=CHANNELS*WIDTH
    audible=sum(any(raw[i:i+framebytes]) for i in range(0,len(raw),framebytes))
    cuts=[]
    starts=[]
    for frame in range(BLOCK,n-PROBE_FRAMES,BLOCK):
        prev=(frame-1)*framebytes
        new=frame*framebytes
        prevpeak=max(abs(signed24(raw,prev)),abs(signed24(raw,prev+WIDTH)))
        newpeak=max(abs(signed24(raw,new)),abs(signed24(raw,new+WIDTH)))
        if prevpeak>CUTOFF_PCM and not any(raw[new:new+PROBE_FRAMES*framebytes]):
            cuts.append(frame)
        if newpeak>CUTOFF_PCM and not any(raw[prev-PROBE_FRAMES*framebytes:prev]):
            starts.append(frame)
    return dict(frames=n,audible_frames=audible,hard_silence_cuts=len(cuts),
                hard_silence_starts=len(starts),hard_cut_offsets=cuts[:12])

def _selftest():
    import tempfile
    import math
    with tempfile.TemporaryDirectory() as root:
        path=Path(root)/"test.wav"
        def make_wave(muted=False, silent=False):
            with wave.open(str(path),"wb") as out:
                out.setnchannels(CHANNELS)
                out.setsampwidth(WIDTH)
                out.setframerate(RATE)
                data=bytearray()
                for frame in range(8192):
                    v=0 if silent or (muted and BLOCK*3 <= frame < BLOCK*5) else int(0.2*(2**23-1)*math.sin(2*math.pi*440*frame/RATE))
                    encoded=v.to_bytes(3,"little",signed=True)
                    data.extend(encoded+encoded)
                out.writeframes(data)
        make_wave()
        good=inspect(path)
        if good["hard_silence_cuts"] or good["audible_frames"]<3000:
            raise AssertionError("continuous control unexpectedly failed")
        make_wave(muted=True)
        bad=inspect(path)
        if bad["hard_silence_cuts"]<1:
            raise AssertionError("injected 512-frame hard mute was not detected")
        make_wave(silent=True)
        none=inspect(path)
        if none["audible_frames"]:
            raise AssertionError("completely silent negative control accepted")
    print("PASS: independent PCM glitch detector catches injected dropout and silence")
    return 0

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--wav",type=Path)
    p.add_argument("--max-cuts",type=int,default=3)
    p.add_argument("--min-audible-frames",type=int,default=1000)
    p.add_argument("--selftest",action="store_true")
    a=p.parse_args()
    if a.selftest:return _selftest()
    if not a.wav or a.max_cuts<0 or a.min_audible_frames<0:
        p.error("Invalid WAV or threshold")
    try:
        metrics=inspect(a.wav)
    except (OSError,ValueError,EOFError) as err:
        print("FAIL: WAV contract:",err)
        return 1
    print("125A_PCM_CONTINUITY",metrics)
    if metrics["hard_silence_cuts"]>a.max_cuts or metrics["audible_frames"]<a.min_audible_frames:
        print("FAIL: interprocess audio discontinuities or silence")
        return 1
    print("PASS: bounded hard silent block edges and audible PCM")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
