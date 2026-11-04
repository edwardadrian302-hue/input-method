#pragma once
#include <ctfutb.h>
#include <olectl.h>
// ABI declarations absent from the MinGW header; Microsoft win32metadata ctfutb.h.
#ifndef __ITfLangBarItemButton_INTERFACE_DEFINED__
inline constexpr IID IID_ModeButton={0x28c7f1d0,0xde25,0x11d2,{0xaf,0xdd,0,0x10,0x5a,0x27,0x99,0xb5}};
struct ITfMenu;
struct ModeButtonInterface:ITfLangBarItem {
  virtual HRESULT STDMETHODCALLTYPE OnClick(int,POINT,const RECT*)=0;
  virtual HRESULT STDMETHODCALLTYPE InitMenu(ITfMenu*)=0;
  virtual HRESULT STDMETHODCALLTYPE OnMenuSelect(UINT)=0;
  virtual HRESULT STDMETHODCALLTYPE GetIcon(HICON*)=0;
  virtual HRESULT STDMETHODCALLTYPE GetText(BSTR*)=0;
};
#endif
inline constexpr GUID InputModeItem={0x2c77a81e,0x41cc,0x4178,{0xa3,0xa7,0x5f,0x8a,0x98,0x75,0x68,0xe6}};
