#include "vst3/WorkerBridge.hpp"
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <string>
namespace fruity {
namespace {
void moduleAnchor(){}
std::wstring q(const std::wstring& s){return L"\""+s+L"\"";}
std::filesystem::path modulePath(){
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(&moduleAnchor),&module))return {};
 wchar_t name[32768]{};
 const DWORD n=GetModuleFileNameW(module,name,DWORD(std::size(name)));
 if(n==0||n>=std::size(name))return {};
 return std::filesystem::path(std::wstring(name,n));
}
long read(const volatile LONG* ptr)noexcept{
 return InterlockedCompareExchange(const_cast<volatile LONG*>(ptr),0,0);
}
}
bool WorkerBridge::start(std::uint32_t sampleRate){
 stop();
 if(sampleRate<8000||sampleRate>192000)return false;
 auto binary=modulePath();
 if(binary.empty())return false;
 // Windows VST3 bundle: Contents/x86_64-win/plugin.vst3
 const auto resources=binary.parent_path().parent_path()/L"Resources";
 const auto helper=resources/L"125A-Fruity-Helper32.exe";
 const auto engine=resources/L"FLSPro.dll";
 const auto bank=resources/L"SynthBank.sf2";
 if(!std::filesystem::is_regular_file(helper)||
    !std::filesystem::is_regular_file(engine)||
    !std::filesystem::is_regular_file(bank))return false;
 static std::atomic<std::uint32_t> serial{0};
 const auto id=std::to_wstring(GetCurrentProcessId())+L"_"+
               std::to_wstring(++serial);
 const std::wstring mapName=L"Local\\125A_Fruity_M_"+id;
 const std::wstring eventName=L"Local\\125A_Fruity_E_"+id;
 mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,
                             static_cast<DWORD>(sizeof(ipc::Shared)),mapName.c_str());
 if(!mapping_){stop();return false;}
 shared_=static_cast<ipc::Shared*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,sizeof(ipc::Shared)));
 if(!shared_){stop();return false;}
 ZeroMemory(shared_,sizeof(*shared_));
 event_=CreateEventW(nullptr,FALSE,FALSE,eventName.c_str());
 if(!event_){stop();return false;}
 std::wstring args=q(helper.wstring())+L" "+q(mapName)+L" "+q(eventName)+L" "+
                   q(engine.wstring())+L" "+q(bank.wstring())+L" "+
                   std::to_wstring(sampleRate);
 STARTUPINFOW startup{};startup.cb=sizeof(startup);
 PROCESS_INFORMATION pi{};
 if(!CreateProcessW(helper.c_str(),args.data(),nullptr,nullptr,FALSE,
                    CREATE_NO_WINDOW,nullptr,resources.c_str(),&startup,&pi)){
   stop();return false;
 }
 process_=pi.hProcess;thread_=pi.hThread;
 requested_=0;played_=0;
 // setupProcessing is a control-thread operation. Do not lose the first MIDI
 // block while Windows starts the helper or finishes loading the SF2.
 for(unsigned tries=0;tries<200;tries++){
   if(isReady())return true;
   if(read(&shared_->status)<0 || WaitForSingleObject(process_,0)==WAIT_OBJECT_0){
     stop();return false;
   }
   Sleep(5); // NEVER wait from process() or another realtime callback.
 }
 stop();return false;
}
bool WorkerBridge::isReady() const noexcept{
 return shared_ && read(&shared_->status)==ipc::kReady;
}
bool WorkerBridge::process(int frames,const ipc::MidiMessage* events,int eventCount,
                           float* destination) noexcept{
 if(frames<0||frames>ipc::kMaxFrames||!destination)return false;
 std::fill_n(destination,static_cast<std::size_t>(frames)*2U,0.0f);
 if(!isReady())return false;
 if(eventCount<0||eventCount>ipc::kMaxMidi)return false;
 const long completed=read(&shared_->responseSequence);
 bool audible=false;
 if(completed!=played_){
   // Do not publish the next request until PCM has been copied from the old one.
   if(shared_->responseFrames==frames && shared_->responseError==0){
     std::copy_n(shared_->responsePCM,static_cast<std::size_t>(frames)*2U,destination);
     audible=true;
   }
   played_=completed;
 }
 if(completed != read(&shared_->requestSequence))return audible; // worker busy
 shared_->requestFrames=frames;
 shared_->requestEvents=eventCount;
 for(int i=0;i<eventCount;i++)shared_->midi[i]=events[i];
 requested_++;
 InterlockedExchange(&shared_->requestSequence,requested_);
 SetEvent(event_);
 return audible;
}
void WorkerBridge::stop() noexcept{
 if(shared_)InterlockedExchange(&shared_->stopRequested,1);
 if(event_)SetEvent(event_);
 if(process_){
   if(WaitForSingleObject(process_,200)!=WAIT_OBJECT_0)TerminateProcess(process_,0xDEAD);
 }
 if(thread_){CloseHandle(thread_);thread_=nullptr;}
 if(process_){CloseHandle(process_);process_=nullptr;}
 if(shared_){UnmapViewOfFile(shared_);shared_=nullptr;}
 if(event_){CloseHandle(event_);event_=nullptr;}
 if(mapping_){CloseHandle(mapping_);mapping_=nullptr;}
 requested_=0;played_=0;
}
} // namespace fruity
