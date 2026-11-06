#include "engine.hpp"
#include <iostream>
int wmain(int argc,wchar_t** argv) {
  try {
    bool isolated=argc==3 && std::wstring(argv[1])==L"--data-dir";
    fs::path home=isolated?fs::absolute(argv[2]):userDir();
    auto sid=userSid(); std::wstring sddl=L"D:P(A;;GA;;;"+sid+L")";
    PSECURITY_DESCRIPTOR sd=nullptr;
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&sd,nullptr)) return 2;
    SECURITY_ATTRIBUTES sa={sizeof(sa),sd,FALSE};
    HANDLE mutex=CreateMutexW(&sa,TRUE,(L"Local\\ChaoxiPinyin.Broker.v5."+sid+instanceSuffix()).c_str());
    if(!mutex)throw std::runtime_error("Cannot create user mutex; run in a normal Windows user session");
    if(GetLastError()==ERROR_ALREADY_EXISTS) { CloseHandle(mutex);LocalFree(sd);return 0; }
    if(!isolated)migrateLegacyProfile(home,legacyUserDir());
    Engine engine(moduleDir(),home);
    bool running=true;
    HANDLE pipe=CreateNamedPipeW(pipeName().c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,sizeof(Response),sizeof(Request),1000,&sa);
    if(pipe==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create local pipe");
    while(running) {
      OVERLAPPED ov={}; ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
      BOOL connected=ConnectNamedPipe(pipe,&ov); DWORD error=GetLastError(),bytes=0;
      if(!connected && error==ERROR_IO_PENDING) { WaitForSingleObject(ov.hEvent,INFINITE); connected=GetOverlappedResult(pipe,&ov,&bytes,FALSE); }
      else if(!connected && error==ERROR_PIPE_CONNECTED) connected=TRUE;
      auto io=[&](bool read,void* data,DWORD size) {
        ResetEvent(ov.hEvent); bytes=0;
        BOOL ok=read?ReadFile(pipe,data,size,&bytes,&ov):WriteFile(pipe,data,size,&bytes,&ov);
        if(!ok && GetLastError()==ERROR_IO_PENDING) {
          if(WaitForSingleObject(ov.hEvent,1500)==WAIT_OBJECT_0) ok=GetOverlappedResult(pipe,&ov,&bytes,FALSE);
          else { CancelIoEx(pipe,&ov); GetOverlappedResult(pipe,&ov,&bytes,TRUE); }
        }
        return ok && bytes==size;
      };
      Request q; Response r;
      if(connected && io(true,&q,sizeof(q)) && q.magic==kMagic) {
        ULONG pid=0; GetNamedPipeClientProcessId(pipe,&pid);
        // Sessions are namespaced by the actual connecting process, not a claimed PID.
        q.client=(uint64_t(pid)<<32)|(q.client&0xffffffff);
        try { r=engine.handle(q); if(q.op==Op::Shutdown) running=false; }
        catch(...) { r.ok=0; }
        if(io(false,&r,sizeof(r))) { BYTE ack=0;io(true,&ack,1); }
      }
      DisconnectNamedPipe(pipe); CloseHandle(ov.hEvent);
    }
    CloseHandle(pipe);ReleaseMutex(mutex); CloseHandle(mutex); LocalFree(sd); return 0;
  } catch(const std::exception& e) { std::cerr<<"ChaoxiPinyin startup failed: "<<e.what()<<"\n"; return 1; }
}
