#pragma once
#include "language_bar_compat.hpp"
#include <functional>
class ModeButton final:public ModeButtonInterface,public ITfSource {
  LONG refs_=1;ITfLangBarItemSink* sink_=nullptr;bool hidden_=false;
public:
  bool english=false;std::function<void()> toggle,settings;
  ModeButton(){++objects;}~ModeButton(){if(sink_)sink_->Release();--objects;}
  void update(){if(sink_)sink_->OnUpdate(7);}
  void detach(){toggle={};settings={};}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID i,void** p)override{if(!p)return E_POINTER;*p=nullptr;if(i==IID_IUnknown||i==IID_ITfLangBarItem||i==IID_ModeButton)*p=static_cast<ModeButtonInterface*>(this);else if(i==IID_ITfSource)*p=static_cast<ITfSource*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
  ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
  ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
  HRESULT STDMETHODCALLTYPE GetInfo(TF_LANGBARITEMINFO* p)override{if(!p)return E_POINTER;*p={};p->clsidService=CLSID_ChaoxiPinyin;p->guidItem=InputModeItem;p->dwStyle=0x10000|2;wcscpy_s(p->szDescription,L"潮汐拼音 · 中英文切换");return S_OK;}
  HRESULT STDMETHODCALLTYPE GetStatus(DWORD* p)override{if(!p)return E_POINTER;*p=hidden_?1:0;return S_OK;}
  HRESULT STDMETHODCALLTYPE Show(BOOL show)override{hidden_=!show;if(sink_)sink_->OnUpdate(0x10000);return S_OK;}
  HRESULT STDMETHODCALLTYPE GetTooltipString(BSTR* p)override{if(!p)return E_POINTER;*p=SysAllocString(english?L"潮汐拼音：英文 · 点击或单按 Shift 切换；右键设置":L"潮汐拼音：中文 · 点击或单按 Shift 切换；右键设置");return *p?S_OK:E_OUTOFMEMORY;}
  HRESULT STDMETHODCALLTYPE OnClick(int click,POINT,const RECT*)override{if(click==2){if(toggle)toggle();}else if(click==1&&settings)settings();return S_OK;}
  HRESULT STDMETHODCALLTYPE InitMenu(ITfMenu*)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE OnMenuSelect(UINT)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE GetIcon(HICON* p)override{
    if(!p)return E_POINTER;HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen),maskDC=CreateCompatibleDC(screen);
    HBITMAP color=CreateCompatibleBitmap(screen,32,32),mask=CreateBitmap(32,32,1,1,nullptr);auto old=SelectObject(dc,color);auto oldMask=SelectObject(maskDC,mask);PatBlt(maskDC,0,0,32,32,BLACKNESS);
    RECT rect={0,0,32,32};auto bg=CreateSolidBrush(RGB(32,108,122));FillRect(dc,&rect,bg);DeleteObject(bg);SetTextColor(dc,RGB(255,255,255));SetBkMode(dc,TRANSPARENT);
    HFONT font=CreateFontW(-25,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Microsoft YaHei UI");auto oldFont=SelectObject(dc,font);DrawTextW(dc,english?L"英":L"中",1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,oldFont);DeleteObject(font);
    ICONINFO info={TRUE,0,0,mask,color};*p=CreateIconIndirect(&info);SelectObject(dc,old);SelectObject(maskDC,oldMask);DeleteObject(color);DeleteObject(mask);DeleteDC(dc);DeleteDC(maskDC);ReleaseDC(nullptr,screen);return *p?S_OK:E_FAIL;
  }
  HRESULT STDMETHODCALLTYPE GetText(BSTR* p)override{if(!p)return E_POINTER;*p=SysAllocString(english?L"英":L"中");return *p?S_OK:E_OUTOFMEMORY;}
  HRESULT STDMETHODCALLTYPE AdviseSink(REFIID i,IUnknown* p,DWORD* cookie)override{if(i!=IID_ITfLangBarItemSink)return CONNECT_E_CANNOTCONNECT;if(!p||!cookie)return E_POINTER;if(sink_)return CONNECT_E_ADVISELIMIT;auto hr=p->QueryInterface(i,(void**)&sink_);if(SUCCEEDED(hr))*cookie=1;return hr;}
  HRESULT STDMETHODCALLTYPE UnadviseSink(DWORD cookie)override{if(cookie!=1||!sink_)return CONNECT_E_NOCONNECTION;sink_->Release();sink_=nullptr;return S_OK;}
};
