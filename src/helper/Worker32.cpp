// Original 32-bit LiveSynth worker, isolated from the 64-bit VST3 process.
// The worker dynamically loads a user-provided engine; none is redistributed.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <string>
#include "bridge/Protocol.hpp"
static_assert(sizeof(void*)==4,"Use a Win32/x86 compiler for this helper.");
namespace fs=std::filesystem;
using Init=int(__stdcall*)(int*,unsigned);
using Load=int(__stdcall*)(int,int,int,const char*,int);
using Midi=int(__stdcall*)(int,unsigned,int);
using Render=void(__stdcall*)(int,int,float*,int,int,void*,void*,unsigned,unsigned);
using Poly=int(__stdcall*)(int,int);
using Free=int(__stdcall*)(int);
static std::string ansi(const std::wstring& w){
 BOOL fallback=FALSE;
 const int n=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,w.data(),int(w.size()),nullptr,0,nullptr,&fallback);
 if(n<=0||fallback)return {};
 std::string s(static_cast<size_t>(n),'\0');fallback=FALSE;
 if(WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,w.data(),int(w.size()),s.data(),n,nullptr,&fallback)!=n||fallback)return {};
 return s;
}
int wmain(int argc,wchar_t** argv){
 // helper32.exe mappingName eventName originalFLSPro.dll originalBank.sf2 samplerate
 if(argc!=6)return 2;
 const std::wstring mappingName=argv[1],eventName=argv[2];
 HANDLE mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,mappingName.c_str());
 HANDLE event=OpenEventW(SYNCHRONIZE|EVENT_MODIFY_STATE,FALSE,eventName.c_str());
 if(!mapping||!event){if(mapping)CloseHandle(mapping);if(event)CloseHandle(event);return 3;}
 auto* s=static_cast<fruity::ipc::Shared*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(fruity::ipc::Shared)));
 if(!s){CloseHandle(mapping);CloseHandle(event);return 3;}
 auto done=[&](int code){UnmapViewOfFile(s);CloseHandle(mapping);CloseHandle(event);return code;};
 if(!fs::is_regular_file(argv[3])||!fs::is_regular_file(argv[4])){
   InterlockedExchange(&s->status,-1);return done(4);
 }
 wchar_t* end=nullptr;const unsigned long rate=std::wcstoul(argv[5],&end,10);
 if(end==argv[5]||*end!=L'\0'||rate<8000||rate>192000){
   InterlockedExchange(&s->status,-2);return done(4);
 }
 const std::string bank=ansi(fs::absolute(argv[4]).wstring());
 if(bank.empty()){InterlockedExchange(&s->status,-3);return done(4);}
 HMODULE library=LoadLibraryExW(fs::absolute(argv[3]).c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!library){InterlockedExchange(&s->status,-4);return done(4);}
 auto resolve=[&](const char* name){return GetProcAddress(library,name);};
 const auto init=reinterpret_cast<Init>(resolve("InitSynthInstance"));
 const auto load=reinterpret_cast<Load>(resolve("LULoadSoundFont"));
 const auto midi=reinterpret_cast<Midi>(resolve("QueueShortMessage"));
 const auto render=reinterpret_cast<Render>(resolve("RenderVoices"));
 const auto poly=reinterpret_cast<Poly>(resolve("SetPolyphony"));
 const auto release=reinterpret_cast<Free>(resolve("FreeSynthInstance"));
 if(!init||!load||!midi||!render||!poly||!release){
   InterlockedExchange(&s->status,-5);FreeLibrary(library);return done(5);
 }
 int slot=-1;
 if(init(&slot,static_cast<unsigned>(rate))!=0||slot<0||slot>=128){
   InterlockedExchange(&s->status,-6);FreeLibrary(library);return done(6);
 }
 poly(slot,64);
 if(load(slot,0,0,bank.c_str(),0)!=0){
   InterlockedExchange(&s->status,-7);release(slot);FreeLibrary(library);return done(7);
 }
 midi(slot,0xC0U,0); // program 0, bank default
 InterlockedExchange(&s->status,fruity::ipc::kReady);
 long processed=0;
 fruity::ipc::MidiMessage pending[fruity::ipc::kMaxMidi]{};
 for(;;){
   const DWORD wait=WaitForSingleObject(event,500);
   if(InterlockedCompareExchange(&s->stopRequested,0,0)!=0)break;
   if(wait!=WAIT_OBJECT_0) {if(wait==WAIT_FAILED)break;continue;}
   const long req=InterlockedCompareExchange(&s->requestSequence,0,0);
   if(req==processed)continue;
   const long frames=s->requestFrames, count=s->requestEvents;
   if(frames<=0||frames>fruity::ipc::kMaxFrames||count<0||count>fruity::ipc::kMaxMidi){
     s->responseError=1;s->responseFrames=0;
     processed=req;InterlockedExchange(&s->responseSequence,req);continue;
   }
   for(long i=0;i<count;i++)pending[i]=s->midi[i];
   std::fill_n(s->responsePCM,static_cast<size_t>(frames)*2U,0.0f);
   long cursor=0;bool valid=true;
   for(long i=0;i<count;i++){
     const long eventAt=pending[i].sampleOffset;
     if(eventAt<cursor||eventAt>=frames){valid=false;break;}
     if(eventAt>cursor){
       render(slot,-1,s->responsePCM+2*cursor,int(eventAt-cursor),1,
              nullptr,nullptr,0U,0U);
     }
     if(midi(slot,pending[i].packed,0)!=0){valid=false;break;}
     cursor=eventAt;
   }
   if(valid&&frames>cursor){
     render(slot,-1,s->responsePCM+2*cursor,int(frames-cursor),1,
            nullptr,nullptr,0U,0U);
   }
   if(valid){
     for(long i=0;i<frames*2;i++)if(!std::isfinite(s->responsePCM[i])){valid=false;break;}
   }
   if(!valid)std::fill_n(s->responsePCM,static_cast<size_t>(frames)*2U,0.0f);
   s->responseFrames=frames;s->responseError=valid?0:2;
   processed=req;
   InterlockedExchange(&s->responseSequence,req); // publishes PCM before counter
 }
 release(slot);FreeLibrary(library);
 return done(0);
}
