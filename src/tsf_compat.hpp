#pragma once
#include <msctf.h>
// Public Windows SDK ABI declarations missing from the MinGW header.
// Source: microsoft/win32metadata, generation/WinSDK/RecompiledIdlHeaders/um/msctf.idl
#ifndef __ITfTextInputProcessorEx_INTERFACE_DEFINED__
static const IID IID_ITfTextInputProcessorEx={0x6e4e2102,0xf9cd,0x433d,{0xb4,0x96,0x30,0x3c,0xe0,0x3a,0x65,0x07}};
struct ITfTextInputProcessorEx:public ITfTextInputProcessor {
  virtual HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr*,TfClientId,DWORD)=0;
};
#endif
// Category value verified against Google's open-source Mozc TSF registrar.
inline constexpr GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT={0x25504fb4,0x7bab,0x4bc1,{0x9c,0x69,0xcf,0x81,0x89,0x0f,0x0e,0xf5}};
#ifndef TF_CLIENTID_NULL
#define TF_CLIENTID_NULL 0
#endif
