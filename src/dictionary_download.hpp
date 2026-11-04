#pragma once
#include "profile.hpp"
#include <winhttp.h>
constexpr wchar_t DictionaryUrl[]=L"https://raw.githubusercontent.com/iDvel/rime-ice/main/cn_dicts/ext.dict.yaml";
struct InternetHandle {
  HINTERNET h=nullptr;
  explicit InternetHandle(HINTERNET value):h(value){if(!h)throw std::runtime_error("Network operation failed");}
  ~InternetHandle(){WinHttpCloseHandle(h);}
  InternetHandle(const InternetHandle&)=delete;
};
inline std::string downloadDictionary(){
  InternetHandle session(WinHttpOpen(L"ChaoxiPinyin/0.5",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
  if(!WinHttpSetTimeouts(session.h,10000,10000,10000,10000))throw std::runtime_error("Network timeout setup failed");
  InternetHandle connection(WinHttpConnect(session.h,L"raw.githubusercontent.com",INTERNET_DEFAULT_HTTPS_PORT,0));
  InternetHandle request(WinHttpOpenRequest(connection.h,L"GET",L"/iDvel/rime-ice/main/cn_dicts/ext.dict.yaml",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
  DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
  if(!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect)))throw std::runtime_error("Network policy setup failed");
  DWORD compression=WINHTTP_DECOMPRESSION_FLAG_GZIP|WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
  if(!WinHttpSetOption(request.h,WINHTTP_OPTION_DECOMPRESSION,&compression,sizeof(compression)))throw std::runtime_error("Network compression setup failed");
  auto start=GetTickCount64();
  if(!WinHttpSendRequest(request.h,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))throw std::runtime_error("Download failed");
  DWORD status=0,size=sizeof(status);
  if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)||status!=200)throw std::runtime_error("HTTP status failure");
  std::string data;char buffer[65536];DWORD read=0;
  for(;;){if(GetTickCount64()-start>180000)throw std::runtime_error("Download exceeded time limit");
    if(!WinHttpReadData(request.h,buffer,sizeof(buffer),&read))throw std::runtime_error("Download interrupted");if(!read)break;
    if(data.size()+read>16*1024*1024)throw std::runtime_error("Download too large");data.append(buffer,read);}
  return data;
}
