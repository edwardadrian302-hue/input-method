#pragma once
// Platform support for the shared engine. Never implements Windows IPC on Android.
#include <chrono>
#include <codecvt>
#include <locale>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
inline std::string utf8(const std::wstring& text) {
  std::string result;
  for(wchar_t ch:text) {
    uint32_t cp=static_cast<uint32_t>(ch);
    if(cp>0x10ffff || (cp>=0xd800&&cp<=0xdfff))throw std::runtime_error("Invalid Unicode");
    if(cp<0x80)result.push_back(static_cast<char>(cp));
    else {
      int count=cp<0x800?2:cp<0x10000?3:4;
      result.push_back(static_cast<char>((count==2?0xc0:count==3?0xe0:0xf0)|(cp>>(6*(count-1)))));
      for(int n=count-2;n>=0;--n)result.push_back(static_cast<char>(0x80|((cp>>(6*n))&0x3f)));
    }
  }
  return result;
}
inline std::wstring wide(const char* text) {
  std::wstring result;if(!text)return result;
  auto data=reinterpret_cast<const unsigned char*>(text);
  while(*data) {
    uint32_t cp=*data++;int count=0;uint32_t minimum=0;
    if(cp<0x80){result.push_back(static_cast<wchar_t>(cp));continue;}
    if(cp>=0xc2&&cp<=0xdf){count=1;minimum=0x80;cp&=0x1f;}
    else if(cp>=0xe0&&cp<=0xef){count=2;minimum=0x800;cp&=0xf;}
    else if(cp>=0xf0&&cp<=0xf4){count=3;minimum=0x10000;cp&=7;}
    else throw std::runtime_error("Invalid UTF-8");
    for(int n=0;n<count;++n){if((*data&0xc0)!=0x80)throw std::runtime_error("Invalid UTF-8");cp=(cp<<6)|(*data++&0x3f);}
    if(cp<minimum||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))throw std::runtime_error("Invalid UTF-8");
    result.push_back(static_cast<wchar_t>(cp));
  }
  return result;
}
inline uint64_t portableMonotonicMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void portableWriteFile(const fs::path& path,const std::string& data) {
  auto pattern=path.string()+".chaoxi-XXXXXX";
  std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
  int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("Cannot create temporary file");
  bool ok=true;size_t offset=0;
  while(offset<data.size()) {
    auto n=::write(fd,data.data()+offset,data.size()-offset);
    if(n<0&&errno==EINTR)continue;
    if(n<=0){ok=false;break;}offset+=static_cast<size_t>(n);
  }
  if(ok&&fsync(fd)!=0)ok=false;
  if(close(fd)!=0)ok=false;
  if(!ok||::rename(name.data(),path.c_str())!=0){::unlink(name.data());throw std::runtime_error("Cannot save file");}
  // Flush directory metadata where supported; the replaced file is already complete.
  fd=::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY);
  if(fd>=0){fsync(fd);close(fd);}
}
