#pragma once
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <sddl.h>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
namespace fs = std::filesystem;
inline std::string utf8(const std::wstring& s) {
  if(s.empty()) return {};
  int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);
  std::string r(n,0); WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),r.data(),n,nullptr,nullptr); return r;
}
inline std::wstring wide(const char* s) {
  if(!s || !*s) return {};
  int n=MultiByteToWideChar(CP_UTF8,0,s,-1,nullptr,0);
  std::wstring r(n,0); MultiByteToWideChar(CP_UTF8,0,s,-1,r.data(),n); r.pop_back(); return r;
}
inline fs::path moduleDir(HMODULE mod=nullptr) {
  wchar_t path[32768]; DWORD n=GetModuleFileNameW(mod,path,32768);
  if(!n || n>=32768) throw std::runtime_error("module path");
  return fs::path(path).parent_path();
}
inline fs::path userDir() {
  PWSTR p=nullptr;
  if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&p))) throw std::runtime_error("local app data");
  fs::path r=fs::path(p)/L"JianPin"; CoTaskMemFree(p); return r;
}
inline std::wstring userSid() {
  HANDLE token=nullptr; if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) throw std::runtime_error("token");
  DWORD n=0; GetTokenInformation(token,TokenUser,nullptr,0,&n);
  std::vector<BYTE> b(n);
  BOOL ok=GetTokenInformation(token,TokenUser,b.data(),n,&n); CloseHandle(token);
  if(!ok) throw std::runtime_error("token user");
  LPWSTR str=nullptr;
  if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(b.data())->User.Sid,&str)) throw std::runtime_error("SID");
  std::wstring r=str; LocalFree(str); return r;
}
// An optional numeric test namespace lets integration tests coexist with an installed IME.
inline std::wstring instanceSuffix() {
  wchar_t value[64]={};DWORD n=GetEnvironmentVariableW(L"JIANPIN_TEST_INSTANCE",value,64);
  if(!n || n>=64)return {};
  std::wstring text(value);if(!std::all_of(text.begin(),text.end(),[](wchar_t c){return (c>=L'0'&&c<=L'9')||c==L'-';}))return {};
  return L".test."+text;
}
inline std::wstring pipeName() { return L"\\\\.\\pipe\\JianPin.v1."+userSid()+instanceSuffix(); }
constexpr uint32_t kMagic=0x4A503031;
enum class Op:uint32_t { Key=1,Clear,Destroy,Status,Learning,Erase,Font,Shutdown };
struct Request { uint32_t magic=kMagic; Op op=Op::Status; uint64_t client=0; int32_t key=0,mask=0,value=0; };
struct Response {
  uint32_t magic=kMagic; int32_t ok=0,eaten=0,learning=1,font=18,page=0,last=1,count=0,highlight=0;
  int32_t storageError=0;
  wchar_t preedit[512]={},commit[1024]={},candidates[9][256]={};
};
template<size_t N> inline void copyText(wchar_t (&out)[N],const std::wstring& s) { wcsncpy_s(out,N,s.c_str(),_TRUNCATE); }
inline bool exchange(const Request& q,Response& r,DWORD timeout=600) {
  auto name=pipeName();
  HANDLE h=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
  if(h==INVALID_HANDLE_VALUE && GetLastError()==ERROR_PIPE_BUSY && WaitNamedPipeW(name.c_str(),timeout))
    h=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
  if(h==INVALID_HANDLE_VALUE) return false;
  DWORD mode=PIPE_READMODE_MESSAGE; SetNamedPipeHandleState(h,&mode,nullptr,nullptr);
  OVERLAPPED ov={}; ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr); DWORD bytes=0;
  BOOL ok=TransactNamedPipe(h,(void*)&q,sizeof(q),&r,sizeof(r),&bytes,&ov);
  if(!ok && GetLastError()==ERROR_IO_PENDING) {
    if(WaitForSingleObject(ov.hEvent,timeout)==WAIT_OBJECT_0) ok=GetOverlappedResult(h,&ov,&bytes,FALSE);
    else { CancelIoEx(h,&ov); GetOverlappedResult(h,&ov,&bytes,TRUE); }
  }
  bool received=ok && bytes==sizeof(r) && r.magic==kMagic;
  if(received) {
    // Acknowledge receipt so the server never disconnects with an unread buffered response.
    ResetEvent(ov.hEvent); BYTE ack=1;DWORD sent=0;
    if(!WriteFile(h,&ack,1,&sent,&ov) && GetLastError()==ERROR_IO_PENDING) {
      if(WaitForSingleObject(ov.hEvent,200)!=WAIT_OBJECT_0)CancelIoEx(h,&ov);
      GetOverlappedResult(h,&ov,&sent,TRUE);
    }
  }
  CloseHandle(ov.hEvent); CloseHandle(h);
  return received && r.ok;
}
inline void launchBroker(const fs::path& dir) {
  std::wstring cmd=L"\""+(dir/L"JianPinBroker.exe").wstring()+L"\"";
  STARTUPINFOW si={sizeof(si)}; si.dwFlags=STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE; PROCESS_INFORMATION pi={};
  if(CreateProcessW((dir/L"JianPinBroker.exe").c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,dir.c_str(),&si,&pi)) { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
}
