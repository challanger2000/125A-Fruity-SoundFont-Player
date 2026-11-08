// Win32/x86 original-engine sound probe. NEVER execute a torrent DLL outside an isolated VM.
// This binary tests the genuine DLL supplied locally; it is NOT a replacement synth.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
static_assert(sizeof(void*)==4,"FLSPro.dll requires an x86 process.");
namespace fs=std::filesystem;
using Init=int(__stdcall*)(int*,unsigned);
using Load=int(__stdcall*)(int,int,int,const char*,int);
using Midi=int(__stdcall*)(int,unsigned,int);
using Render=void(__stdcall*)(int,int,float*,int,int,void*,void*,unsigned,unsigned);
using Poly=int(__stdcall*)(int,int);
using Free=int(__stdcall*)(int);
static void le16(std::ostream& s,unsigned n){s.put(char(n));s.put(char(n>>8));}
static void le32(std::ostream& s,unsigned n){for(int i=0;i<4;i++)s.put(char(n>>(8*i)));}
static std::string ansi(const std::wstring& w){
  BOOL fallback=FALSE;
  int n=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,w.data(),int(w.size()),nullptr,0,nullptr,&fallback);
  if(n<=0||fallback)return {};
  std::string r(size_t(n),'\0');fallback=FALSE;
  if(WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,w.data(),int(w.size()),r.data(),n,nullptr,&fallback)!=n||fallback)return {};
  return r;
}
int wmain(int argc,wchar_t** argv){
  if(argc!=4){std::fputs("usage: probe32.exe FLSPro.dll bank.sf2 output.wav\n",stderr);return 2;}
  const fs::path dll=fs::absolute(argv[1]), bank=fs::absolute(argv[2]), destination=fs::absolute(argv[3]);
  if(!fs::is_regular_file(dll)||!fs::is_regular_file(bank)){std::fputs("missing DLL or bank\n",stderr);return 2;}
  const std::string path=ansi(bank.wstring());
  if(path.empty()){std::fputs("SF2 path not representable in ANSI\n",stderr);return 2;}
  HMODULE module=LoadLibraryExW(dll.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
  if(!module){std::fprintf(stderr,"LoadLibrary failed %lu\n",GetLastError());return 3;}
  auto sym=[&](const char* n)->FARPROC{return GetProcAddress(module,n);};
  auto init=reinterpret_cast<Init>(sym("InitSynthInstance"));
  auto load=reinterpret_cast<Load>(sym("LULoadSoundFont"));
  auto midi=reinterpret_cast<Midi>(sym("QueueShortMessage"));
  auto render=reinterpret_cast<Render>(sym("RenderVoices"));
  auto poly=reinterpret_cast<Poly>(sym("SetPolyphony"));
  auto release=reinterpret_cast<Free>(sym("FreeSynthInstance"));
  if(!init||!load||!midi||!render||!poly||!release){std::fputs("missing exports\n",stderr);FreeLibrary(module);return 3;}
  int slot=-1;int rc=init(&slot,44100);
  std::printf("Init=%d slot=%d\n",rc,slot);
  if(rc||slot<0||slot>=128){FreeLibrary(module);return 4;}
  std::printf("Polyphony=%d\n",poly(slot,64));
  rc=load(slot,0,0,path.c_str(),0);
  std::printf("Load=%d\n",rc);
  if(rc){release(slot);FreeLibrary(module);return 5;}
  rc=midi(slot,0x000000C0U,0); // bank0/program0
  rc|=midi(slot,0x00643C90U,0); // note 60, velocity 100
  if(rc){release(slot);FreeLibrary(module);return 6;}
  constexpr unsigned rate=44100, frames=rate*3, batch=256, bytes=frames*8;
  std::vector<float> signal(size_t(frames)*2U,0.0f);
  std::vector<float> block(size_t(batch)*2U,0.0f);
  size_t nonzero=0;double energy=0;bool valid=true;bool off=false;
  for(unsigned position=0;position<frames;){
    if(!off && position>=rate){midi(slot,0x00003C80U,0);off=true;}
    const int count=int(std::min<unsigned>(batch,frames-position));
    std::fill(block.begin(),block.end(),0.0f);
    // Parameters 6-9 are suspected optional auxiliary outputs; null means
    // not requested. This must be confirmed by the first isolated probe.
    render(slot,-1,block.data(),count,1,nullptr,nullptr,0U,0U);
    for(int j=0;j<count*2;j++){
      const float x=block[size_t(j)];
      if(!std::isfinite(x))valid=false;
      if(x!=0.0f)++nonzero;
      if(std::isfinite(x))energy+=double(x)*double(x);
      signal[size_t(position)*2U+size_t(j)]=x;
    }
    position+=unsigned(count);
  }
  release(slot);FreeLibrary(module);
  std::printf("frames=%u nonzero=%zu RMS=%f finite=%d\n",frames,nonzero,
              std::sqrt(energy/signal.size()),int(valid));
  if(!valid||!nonzero||!(energy>0)){std::fputs("FAIL: no valid original PCM\n",stderr);return 7;}
  std::ofstream out(destination,std::ios::binary|std::ios::trunc);
  if(!out)return 8;
  out.write("RIFF",4);le32(out,bytes+36);out.write("WAVEfmt ",8);
  le32(out,16);le16(out,3);le16(out,2);le32(out,rate);le32(out,rate*8);
  le16(out,8);le16(out,32);out.write("data",4);le32(out,bytes);
  out.write(reinterpret_cast<const char*>(signal.data()),bytes);
  if(!out)return 8;
  std::puts("PASS: finite nonzero PCM from dynamically loaded engine");
  return 0;
}
