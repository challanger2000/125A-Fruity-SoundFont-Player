// Test-only x86 ABI stand-in; does NOT synthesize a real SoundFont.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstddef>
static int slot=7;static bool playing=false;static unsigned position=0;
extern "C" int __stdcall InitSynthInstance(int* index,unsigned rate){
 if(!index||rate!=44100)return -1;
 *index=slot;playing=false;position=0;return 0;
}
extern "C" int __stdcall SetPolyphony(int index,int voices){
 return index==slot && voices>=1 ? voices : -1;
}
extern "C" int __stdcall LULoadSoundFont(int index,int,int,const char* filename,int){
 if(index!=slot||!filename)return -1;
 return GetFileAttributesA(filename)==INVALID_FILE_ATTRIBUTES?-1:0;
}
extern "C" int __stdcall QueueShortMessage(int index,unsigned packed,int){
 if(index!=slot)return -1;
 const unsigned status=packed&0xF0U;
 if(status==0x90U)playing=(packed>>16U)!=0U;
 if(status==0x80U)playing=false;
 return 0;
}
extern "C" void __stdcall RenderVoices(int index,int,float* pcm,int frames,
     int,void*,void*,unsigned,unsigned){
 if(index!=slot||!pcm||frames<0||frames>4096)return;
 for(int i=0;i<frames;i++){
   const float s=playing?0.125f*std::sin(float(position)*0.0314f):0.0f;
   pcm[std::size_t(i)*2U]+=s;pcm[std::size_t(i)*2U+1]+=s;
   position++;
 }
}
extern "C" int __stdcall FreeSynthInstance(int index){playing=false;return index==slot?0:-1;}
