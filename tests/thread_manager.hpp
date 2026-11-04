#pragma once
#include <msctf.h>
#include <ctfutb.h>
// Keyboard and language-bar subscriptions are isolated for an unregistered TIP.
// All text stores, edit sessions, ranges and composition notifications remain real TSF.
// This is not a test of system registration or actual browser key routing.
class TestManager final:public ITfThreadMgr,public ITfKeystrokeMgr,public ITfLangBarItemMgr {
  LONG refs_=1;ITfThreadMgr* real_;ITfLangBarItem* item_=nullptr;
public:
  explicit TestManager(ITfThreadMgr* m):real_(m){m->AddRef();}
  ~TestManager(){if(item_)item_->Release();real_->Release();}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID i,void** p)override{
    if(i==IID_IUnknown||i==IID_ITfThreadMgr){*p=static_cast<ITfThreadMgr*>(this);AddRef();return S_OK;}
    if(i==IID_ITfKeystrokeMgr){*p=static_cast<ITfKeystrokeMgr*>(this);AddRef();return S_OK;}
    if(i==IID_ITfLangBarItemMgr){*p=static_cast<ITfLangBarItemMgr*>(this);AddRef();return S_OK;}
    return real_->QueryInterface(i,p);
  }
  ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
  ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
  HRESULT STDMETHODCALLTYPE Activate(TfClientId* p)override{return real_->Activate(p);}
  HRESULT STDMETHODCALLTYPE Deactivate()override{return real_->Deactivate();}
  HRESULT STDMETHODCALLTYPE CreateDocumentMgr(ITfDocumentMgr** p)override{return real_->CreateDocumentMgr(p);}
  HRESULT STDMETHODCALLTYPE EnumDocumentMgrs(IEnumTfDocumentMgrs** p)override{return real_->EnumDocumentMgrs(p);}
  HRESULT STDMETHODCALLTYPE GetFocus(ITfDocumentMgr** p)override{return real_->GetFocus(p);}
  HRESULT STDMETHODCALLTYPE SetFocus(ITfDocumentMgr* p)override{return real_->SetFocus(p);}
  HRESULT STDMETHODCALLTYPE AssociateFocus(HWND h,ITfDocumentMgr* p,ITfDocumentMgr** q)override{return real_->AssociateFocus(h,p,q);}
  HRESULT STDMETHODCALLTYPE IsThreadFocus(BOOL* p)override{return real_->IsThreadFocus(p);}
  HRESULT STDMETHODCALLTYPE GetFunctionProvider(REFCLSID c,ITfFunctionProvider** p)override{return real_->GetFunctionProvider(c,p);}
  HRESULT STDMETHODCALLTYPE EnumFunctionProviders(IEnumTfFunctionProviders** p)override{return real_->EnumFunctionProviders(p);}
  HRESULT STDMETHODCALLTYPE GetGlobalCompartment(ITfCompartmentMgr** p)override{return real_->GetGlobalCompartment(p);}
  HRESULT STDMETHODCALLTYPE EnumItems(IEnumTfLangBarItems**)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetItem(REFGUID guid,ITfLangBarItem** item)override{*item=nullptr;if(!item_)return E_FAIL;TF_LANGBARITEMINFO info={};item_->GetInfo(&info);if(info.guidItem!=guid)return E_FAIL;*item=item_;item_->AddRef();return S_OK;}
  HRESULT STDMETHODCALLTYPE AddItem(ITfLangBarItem* item)override{if(item_)return E_FAIL;item_=item;item_->AddRef();return S_OK;}
  HRESULT STDMETHODCALLTYPE RemoveItem(ITfLangBarItem* item)override{if(item!=item_)return E_FAIL;item_->Release();item_=nullptr;return S_OK;}
  HRESULT STDMETHODCALLTYPE AdviseItemSink(ITfLangBarItemSink*,DWORD*,REFGUID)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE UnadviseItemSink(DWORD)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetItemFloatingRect(DWORD,REFGUID,RECT*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetItemsStatus(ULONG,const GUID*,DWORD*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetItemNum(ULONG* count)override{*count=item_?1:0;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetItems(ULONG,ITfLangBarItem**,TF_LANGBARITEMINFO*,DWORD*,ULONG*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE AdviseItemsSink(ULONG,ITfLangBarItemSink**,const GUID*,DWORD*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE UnadviseItemsSink(ULONG,DWORD*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE AdviseKeyEventSink(TfClientId,ITfKeyEventSink*,BOOL)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE UnadviseKeyEventSink(TfClientId)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE GetForeground(CLSID*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE TestKeyDown(WPARAM,LPARAM,BOOL*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE TestKeyUp(WPARAM,LPARAM,BOOL*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE KeyDown(WPARAM,LPARAM,BOOL*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE KeyUp(WPARAM,LPARAM,BOOL*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetPreservedKey(ITfContext*,const TF_PRESERVEDKEY*,GUID*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE IsPreservedKey(REFGUID,const TF_PRESERVEDKEY*,BOOL*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE PreserveKey(TfClientId,REFGUID,const TF_PRESERVEDKEY*,const WCHAR*,ULONG)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE UnpreserveKey(REFGUID,const TF_PRESERVEDKEY*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE SetPreservedKeyDescription(REFGUID,const WCHAR*,ULONG)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetPreservedKeyDescription(REFGUID,BSTR*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE SimulatePreservedKey(ITfContext*,REFGUID,BOOL*)override{return E_NOTIMPL;}
};
