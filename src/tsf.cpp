#include "common.hpp"
#include "candidate.hpp"
#include <msctf.h>
#include "tsf_compat.hpp"
#include <inputscope.h>
#include <atomic>
extern "C" const GUID GUID_PROP_INPUTSCOPE={0x1713dd5a,0x68e7,0x4a5b,{0x9a,0xf6,0x59,0x2a,0x59,0x5c,0x77,0x8d}};
extern "C" const IID IID_ITfInputScope={0xfde1eaee,0x6924,0x4cdf,{0x91,0xe7,0xda,0x38,0xcf,0xf5,0x55,0x9d}};

// Stable identity: never reuse a third-party IME's registration.
static const CLSID CLSID_ChaoxiPinyin={0x3719c10e,0x98bf,0x4b9e,{0xa4,0xf2,0xd3,0xce,0xbc,0x8f,0xb6,0x92}};
static const GUID PROFILE_ChaoxiPinyin={0x3a63f58a,0xb3f1,0x41d3,{0x8c,0xc8,0x50,0xbd,0xf7,0x1a,0x04,0x18}};
static HMODULE instance;
static std::atomic<long> objects{0};
static std::atomic<uint32_t> ids{1};
static constexpr LANGID language=0x0804;
#include "language_bar.hpp"
class Service;
class Edit final:public ITfEditSession {
  LONG refs_=1; Service* owner_; ITfContext* context_; ITfComposition* target_=nullptr; int key_,mask_; bool cancel_,probe_;
public:
  bool eaten=false,allowed=true;
  Edit(Service*,ITfContext*,int,int,bool=false,bool=false);
  ~Edit();
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** p) override { if(!p)return E_POINTER; *p=nullptr; if(riid==IID_IUnknown || riid==IID_ITfEditSession) { *p=static_cast<ITfEditSession*>(this); AddRef(); return S_OK; } return E_NOINTERFACE; }
  ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs_); }
  ULONG STDMETHODCALLTYPE Release() override { auto n=InterlockedDecrement(&refs_); if(!n) delete this; return n; }
  HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override;
};
class Service final:public ITfTextInputProcessorEx,public ITfKeyEventSink,public ITfThreadMgrEventSink,public ITfCompositionSink,public ITfTextEditSink {
  LONG refs_=1;
  ITfThreadMgr* manager_=nullptr; TfClientId client_=TF_CLIENTID_NULL; DWORD sink_=TF_INVALID_COOKIE;
  ITfComposition* composition_=nullptr; ITfContext* composingContext_=nullptr;
  DWORD editSink_=TF_INVALID_COOKIE;
  HWND popup_=nullptr; CandidatePanel panel_; Response view_;
  ITfLangBarItemMgr* bar_=nullptr;ModeButton* mode_=nullptr;
  bool english_=false,shiftOnly_=false; uint32_t id_=ids++;
  ULONGLONG lastLaunch_=0;
  friend class Edit;
  void unwatch() {
    if(composingContext_ && editSink_!=TF_INVALID_COOKIE) {
      ITfSource* source=nullptr;if(SUCCEEDED(composingContext_->QueryInterface(IID_ITfSource,(void**)&source))){source->UnadviseSink(editSink_);source->Release();}
    }
    editSink_=TF_INVALID_COOKIE;
  }
  static LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    auto self=reinterpret_cast<Service*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE) { self=static_cast<Service*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams); SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)self); }
    if(msg==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if(msg==WM_PAINT && self) {
      PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);self->panel_.paint(dc,self->view_);EndPaint(hwnd,&ps);return 0;
    }
    if(msg==WM_ERASEBKGND)return 1;
    return DefWindowProcW(hwnd,msg,wp,lp);
  }
  void show(ITfContext* ctx,TfEditCookie cookie) {
    if(!*view_.preedit) { if(popup_) ShowWindow(popup_,SW_HIDE); return; }
    if(!popup_) {
      WNDCLASSW wc={}; wc.style=CS_DROPSHADOW; wc.lpfnWndProc=WindowProc; wc.hInstance=instance; wc.lpszClassName=L"ChaoxiPinyin.Candidate.v1";
      RegisterClassW(&wc);
      popup_=CreateWindowExW(WS_EX_TOPMOST|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,wc.lpszClassName,L"潮汐拼音",WS_POPUP,0,0,1,1,nullptr,nullptr,instance,this);
    }
    ITfContextView* cv=nullptr; RECT anchor={}; BOOL clipped=FALSE; bool located=false;
    if(SUCCEEDED(ctx->GetActiveView(&cv))) {
      if(composition_) { ITfRange* range=nullptr; if(SUCCEEDED(composition_->GetRange(&range))) { located=SUCCEEDED(cv->GetTextExt(cookie,range,&anchor,&clipped)); range->Release(); } }
      if(!located) { HWND wnd=nullptr; cv->GetWnd(&wnd); if(wnd) { GetWindowRect(wnd,&anchor); anchor.bottom=anchor.top+40; located=true; } }
      cv->Release();
    }
    if(!located) { POINT pt; GetCursorPos(&pt); anchor={pt.x,pt.y,pt.x,pt.y+20}; }
    MONITORINFO mi={sizeof(mi)}; HMONITOR mon=MonitorFromRect(&anchor,MONITOR_DEFAULTTONEAREST); GetMonitorInfoW(mon,&mi);
    UINT dpi=96; HMODULE shcore=LoadLibraryW(L"shcore.dll");
    if(shcore) { auto get=(HRESULT(WINAPI*)(HMONITOR,int,UINT*,UINT*))GetProcAddress(shcore,"GetDpiForMonitor"); UINT y=96; if(get)get(mon,0,&dpi,&y); FreeLibrary(shcore); }
    HDC dc=GetDC(popup_);panel_.layout(dc,view_,dpi,mi.rcWork.right-mi.rcWork.left,mi.rcWork.bottom-mi.rcWork.top);ReleaseDC(popup_,dc);
    int width=panel_.width,height=panel_.height;
    SetWindowRgn(popup_,CreateRoundRectRgn(0,0,width+1,height+1,panel_.px(16),panel_.px(16)),FALSE);
    int x=std::clamp((int)anchor.left,(int)mi.rcWork.left,(int)mi.rcWork.right-width);
    int y=anchor.bottom; if(y+height>mi.rcWork.bottom)y=anchor.top-height; y=std::max(y,(int)mi.rcWork.top);
    SetWindowPos(popup_,HWND_TOPMOST,x,y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW); InvalidateRect(popup_,nullptr,TRUE);
  }
  bool call(Request q,Response& r) {
    q.client=id_;
    if(exchange(q,r))return true;
    auto now=GetTickCount64(); if(now-lastLaunch_>5000) { launchBroker(moduleDir(instance)); lastLaunch_=now; }
    return false;
  }
  void cancel() {
    if(popup_)ShowWindow(popup_,SW_HIDE); view_=Response{};
    Request q; q.op=Op::Clear; Response r; call(q,r);
    if(composingContext_) {
      auto ctx=composingContext_; ctx->AddRef(); auto edit=new Edit(this,ctx,0,0,true); HRESULT result;
      ctx->RequestEditSession(client_,edit,TF_ES_ASYNCDONTCARE|TF_ES_READWRITE,&result); edit->Release(); ctx->Release();
    }
  }
  bool disabled(ITfContext* ctx) {
    ITfCompartmentMgr* cm=nullptr; bool off=false;
    if(SUCCEEDED(ctx->QueryInterface(IID_ITfCompartmentMgr,(void**)&cm))) {
      for(const GUID* g:{&GUID_COMPARTMENT_KEYBOARD_DISABLED,&GUID_COMPARTMENT_EMPTYCONTEXT}) {
        ITfCompartment* c=nullptr;
        if(SUCCEEDED(cm->GetCompartment(*g,&c))) { VARIANT v; VariantInit(&v); if(SUCCEEDED(c->GetValue(&v)) && v.vt==VT_I4 && v.lVal)off=true; VariantClear(&v); c->Release(); }
      }
      cm->Release();
    }
    return off;
  }
  int translate(WPARAM vk,LPARAM lp) {
    switch(vk) {
      case VK_BACK:return 0xff08; case VK_TAB:return 0xff09; case VK_RETURN:return 0xff0d;
      case VK_ESCAPE:return 0xff1b; case VK_DELETE:return 0xffff;
      case VK_LEFT:return 0xff51; case VK_UP:return 0xff52; case VK_RIGHT:return 0xff53; case VK_DOWN:return 0xff54;
      case VK_PRIOR:return 0xff55; case VK_NEXT:return 0xff56; case VK_HOME:return 0xff50; case VK_END:return 0xff57;
    }
    BYTE state[256]; GetKeyboardState(state); wchar_t buf[8]={};
    int n=ToUnicode((UINT)vk,(UINT)((lp>>16)&0xff),state,buf,8,4);
    return n==1 && buf[0]>=32 && buf[0]<127?buf[0]:0;
  }
  bool wants(ITfContext* ctx,WPARAM vk,LPARAM lp) {
    if(disabled(ctx))return false;
    if(GetKeyState(VK_CONTROL)&0x8000 || GetKeyState(VK_MENU)&0x8000 || GetKeyState(VK_LWIN)&0x8000 || GetKeyState(VK_RWIN)&0x8000)return false;
    if(vk==VK_SHIFT)return true;
    if(english_ || (GetKeyState(VK_CAPITAL)&1))return false;
    int key=translate(vk,lp);
    if(*view_.preedit)return key!=0;
    return (key>='a' && key<='z') || (key>='A' && key<='Z') || (key && std::string(",.!?:;\"'()[]/\\<>_").find((char)key)!=std::string::npos);
  }
  void toggleMode() {
    if(composingContext_ && *view_.preedit){
      auto edit=new Edit(this,composingContext_,' ',0);HRESULT result=E_FAIL;
      auto hr=composingContext_->RequestEditSession(client_,edit,TF_ES_SYNC|TF_ES_READWRITE,&result);
      bool ok=SUCCEEDED(hr)&&SUCCEEDED(result)&&edit->eaten;edit->Release();if(!ok)return;
    }
    english_=!english_;if(mode_){mode_->english=english_;mode_->update();}
  }
public:
  Service(){++objects;}
  ~Service(){Deactivate();--objects;}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** p) override {
    if(!p)return E_POINTER; *p=nullptr;
    if(riid==IID_IUnknown || riid==IID_ITfTextInputProcessor || riid==IID_ITfTextInputProcessorEx)*p=static_cast<ITfTextInputProcessorEx*>(this);
    else if(riid==IID_ITfKeyEventSink)*p=static_cast<ITfKeyEventSink*>(this);
    else if(riid==IID_ITfThreadMgrEventSink)*p=static_cast<ITfThreadMgrEventSink*>(this);
    else if(riid==IID_ITfCompositionSink)*p=static_cast<ITfCompositionSink*>(this);
    else if(riid==IID_ITfTextEditSink)*p=static_cast<ITfTextEditSink*>(this);
    else return E_NOINTERFACE; AddRef();return S_OK;
  }
  ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&refs_);}
  ULONG STDMETHODCALLTYPE Release() override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
  HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr* m,TfClientId id) override{return ActivateEx(m,id,0);}
  HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr* m,TfClientId id,DWORD) override {
    manager_=m;manager_->AddRef();client_=id; ITfKeystrokeMgr* keys=nullptr;
    HRESULT hr=m->QueryInterface(IID_ITfKeystrokeMgr,(void**)&keys);
    if(SUCCEEDED(hr)){hr=keys->AdviseKeyEventSink(id,this,TRUE);keys->Release();}
    if(FAILED(hr)){Deactivate();return hr;}
    ITfSource* source=nullptr;
    if(SUCCEEDED(m->QueryInterface(IID_ITfSource,(void**)&source))){hr=source->AdviseSink(IID_ITfThreadMgrEventSink,static_cast<ITfThreadMgrEventSink*>(this),&sink_);source->Release();}
    if(FAILED(hr)){Deactivate();return hr;}
    if(SUCCEEDED(m->QueryInterface(IID_ITfLangBarItemMgr,(void**)&bar_))){
      mode_=new ModeButton;mode_->toggle=[this]{toggleMode();};
      mode_->settings=[] { auto exe=moduleDir(instance)/L"ChaoxiPinyinSettings.exe";ShellExecuteW(nullptr,L"open",exe.c_str(),nullptr,exe.parent_path().c_str(),SW_SHOWNORMAL); };
      if(FAILED(bar_->AddItem(mode_))){mode_->detach();mode_->Release();mode_=nullptr;bar_->Release();bar_=nullptr;}
    }
    launchBroker(moduleDir(instance));lastLaunch_=GetTickCount64();return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Deactivate() override {
    if(mode_){mode_->detach();if(bar_)bar_->RemoveItem(mode_);mode_->Release();mode_=nullptr;}
    if(bar_){bar_->Release();bar_=nullptr;}
    if(!manager_)return S_OK;
    // End composition synchronously where possible, never retain a browser context after deactivation.
    if(composingContext_ && refs_>0) { auto e=new Edit(this,composingContext_,0,0,true);HRESULT hr;composingContext_->RequestEditSession(client_,e,TF_ES_SYNC|TF_ES_READWRITE,&hr);e->Release(); }
    unwatch();if(composition_){composition_->Release();composition_=nullptr;}
    if(composingContext_){composingContext_->Release();composingContext_=nullptr;}
    ITfKeystrokeMgr* keys=nullptr;if(SUCCEEDED(manager_->QueryInterface(IID_ITfKeystrokeMgr,(void**)&keys))){keys->UnadviseKeyEventSink(client_);keys->Release();}
    ITfSource* source=nullptr;if(SUCCEEDED(manager_->QueryInterface(IID_ITfSource,(void**)&source))){if(sink_!=TF_INVALID_COOKIE)source->UnadviseSink(sink_);source->Release();}
    manager_->Release();manager_=nullptr;client_=TF_CLIENTID_NULL;sink_=TF_INVALID_COOKIE;
    Request q;q.op=Op::Destroy;q.client=id_;Response r;exchange(q,r);
    if(popup_){DestroyWindow(popup_);popup_=nullptr;}return S_OK;
  }
  HRESULT STDMETHODCALLTYPE OnSetFocus(BOOL foreground) override{if(!foreground){shiftOnly_=false;cancel();}return S_OK;}
  HRESULT STDMETHODCALLTYPE OnTestKeyDown(ITfContext* ctx,WPARAM vk,LPARAM lp,BOOL* eaten) override {
    if(vk!=VK_SHIFT)shiftOnly_=false;
    *eaten=wants(ctx,vk,lp);
    if(*eaten){auto e=new Edit(this,ctx,0,0,false,true);HRESULT hr=E_FAIL;ctx->RequestEditSession(client_,e,TF_ES_SYNC|TF_ES_READ,&hr);*eaten=SUCCEEDED(hr)&&e->allowed;e->Release();}
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE OnKeyDown(ITfContext* ctx,WPARAM vk,LPARAM lp,BOOL* eaten) override {
    *eaten=FALSE;
    if(vk==VK_SHIFT){shiftOnly_=wants(ctx,vk,lp);*eaten=shiftOnly_;return S_OK;}
    shiftOnly_=false;
    if(!wants(ctx,vk,lp)){if(*view_.preedit)cancel();return S_OK;}
    auto edit=new Edit(this,ctx,translate(vk,lp),0);HRESULT result=E_FAIL;
    HRESULT hr=ctx->RequestEditSession(client_,edit,TF_ES_SYNC|TF_ES_READWRITE,&result);
    *eaten=SUCCEEDED(hr)&&SUCCEEDED(result)&&edit->eaten;edit->Release();return S_OK;
  }
  HRESULT STDMETHODCALLTYPE OnTestKeyUp(ITfContext*,WPARAM vk,LPARAM,BOOL* eaten) override{*eaten=vk==VK_SHIFT&&shiftOnly_;return S_OK;}
  HRESULT STDMETHODCALLTYPE OnKeyUp(ITfContext*,WPARAM vk,LPARAM,BOOL* eaten) override{*eaten=FALSE;if(vk==VK_SHIFT&&shiftOnly_){toggleMode();shiftOnly_=false;*eaten=TRUE;}return S_OK;}
  HRESULT STDMETHODCALLTYPE OnPreservedKey(ITfContext*,REFGUID,BOOL* eaten) override{*eaten=FALSE;return S_OK;}
  HRESULT STDMETHODCALLTYPE OnInitDocumentMgr(ITfDocumentMgr*) override{return S_OK;}
  HRESULT STDMETHODCALLTYPE OnUninitDocumentMgr(ITfDocumentMgr*) override{return S_OK;}
  HRESULT STDMETHODCALLTYPE OnSetFocus(ITfDocumentMgr*,ITfDocumentMgr*) override{cancel();return S_OK;}
  HRESULT STDMETHODCALLTYPE OnPushContext(ITfContext*) override{cancel();return S_OK;}
  HRESULT STDMETHODCALLTYPE OnPopContext(ITfContext*) override{cancel();return S_OK;}
  HRESULT STDMETHODCALLTYPE OnCompositionTerminated(TfEditCookie,ITfComposition* comp) override {
    if(comp==composition_) { unwatch();composition_->Release();composition_=nullptr;if(composingContext_){composingContext_->Release();composingContext_=nullptr;} }
    if(popup_)ShowWindow(popup_,SW_HIDE);view_=Response{};
    Request q;q.op=Op::Clear;Response r;call(q,r);return S_OK;
  }
  HRESULT STDMETHODCALLTYPE OnEndEdit(ITfContext* ctx,TfEditCookie cookie,ITfEditRecord* record) override {
    BOOL moved=FALSE;if(!composition_||ctx!=composingContext_||FAILED(record->GetSelectionStatus(&moved))||!moved)return S_OK;
    TF_SELECTION sel={};ULONG fetched=0;ITfRange* range=nullptr;
    if(SUCCEEDED(ctx->GetSelection(cookie,TF_DEFAULT_SELECTION,1,&sel,&fetched))&&fetched){
      if(SUCCEEDED(composition_->GetRange(&range))){LONG a=0,b=0;sel.range->CompareStart(cookie,range,TF_ANCHOR_END,&a);sel.range->CompareEnd(cookie,range,TF_ANCHOR_END,&b);range->Release();if(a||b)cancel();}sel.range->Release();
    }return S_OK;
  }
};
Edit::Edit(Service* s,ITfContext* c,int k,int m,bool cancel,bool probe):owner_(s),context_(c),key_(k),mask_(m),cancel_(cancel),probe_(probe){s->AddRef();c->AddRef();if(cancel && s->composition_){target_=s->composition_;target_->AddRef();}++objects;}
Edit::~Edit(){if(target_)target_->Release();context_->Release();owner_->Release();--objects;}
HRESULT Edit::DoEditSession(TfEditCookie cookie) {
  auto s=owner_;
  if(cancel_){
    if(s->composition_ && s->composition_==target_ && s->composingContext_==context_){s->unwatch();auto comp=s->composition_;comp->AddRef();ITfRange* r=nullptr;if(SUCCEEDED(comp->GetRange(&r))){r->SetText(cookie,0,L"",0);r->Release();}comp->EndComposition(cookie);if(s->composition_){s->composition_->Release();s->composition_=nullptr;}if(s->composingContext_){s->composingContext_->Release();s->composingContext_=nullptr;}comp->Release();}
    return S_OK;
  }
  TF_SELECTION selection={};ULONG fetched=0;
  if(FAILED(context_->GetSelection(cookie,TF_DEFAULT_SELECTION,1,&selection,&fetched))||!fetched){allowed=false;return E_FAIL;}
  ITfReadOnlyProperty* scopeProp=nullptr;
  if(SUCCEEDED(context_->GetAppProperty(GUID_PROP_INPUTSCOPE,&scopeProp))){
    VARIANT v;VariantInit(&v);
    if(SUCCEEDED(scopeProp->GetValue(cookie,selection.range,&v))&&v.vt==VT_UNKNOWN&&v.punkVal){ITfInputScope* scope=nullptr;if(SUCCEEDED(v.punkVal->QueryInterface(IID_ITfInputScope,(void**)&scope))){InputScope* scopes=nullptr;UINT count=0;if(SUCCEEDED(scope->GetInputScopes(&scopes,&count))){for(UINT i=0;i<count;++i)if(scopes[i]==IS_PASSWORD)allowed=false;CoTaskMemFree(scopes);}scope->Release();}}
    VariantClear(&v);scopeProp->Release();
  }
  selection.range->Release();
  if(probe_||!allowed)return S_OK;
  Request q;q.op=Op::Key;q.key=key_;q.mask=mask_;Response next;
  if(!s->call(q,next))return E_FAIL;
  eaten=next.eaten;
  auto finish=[&](const wchar_t* text)->HRESULT {
    if(s->composition_){
      ITfRange* range=nullptr;HRESULT hr=s->composition_->GetRange(&range);if(FAILED(hr))return hr;
      hr=range->SetText(cookie,0,text,(LONG)wcslen(text));
      if(SUCCEEDED(hr)){range->Collapse(cookie,TF_ANCHOR_END);TF_SELECTION sel={range,{TF_AE_NONE,FALSE}};hr=context_->SetSelection(cookie,1,&sel);}
      range->Release();if(FAILED(hr))return hr;
      s->unwatch();auto comp=s->composition_;comp->AddRef();comp->EndComposition(cookie);if(s->composition_){s->composition_->Release();s->composition_=nullptr;}if(s->composingContext_){s->composingContext_->Release();s->composingContext_=nullptr;}comp->Release();return S_OK;
    }
    if(!*text)return S_OK;
    ITfInsertAtSelection* insert=nullptr;HRESULT hr=context_->QueryInterface(IID_ITfInsertAtSelection,(void**)&insert);
    if(SUCCEEDED(hr)){
      ITfRange* range=nullptr;hr=insert->InsertTextAtSelection(cookie,0,text,(LONG)wcslen(text),&range);insert->Release();
      if(SUCCEEDED(hr)&&range){range->Collapse(cookie,TF_ANCHOR_END);TF_SELECTION sel={range,{TF_AE_NONE,FALSE}};hr=context_->SetSelection(cookie,1,&sel);}
      if(range)range->Release();
    }return hr;
  };
  if(*next.commit){HRESULT hr=finish(next.commit);if(FAILED(hr))return hr;}
  if(*next.preedit){
    ITfRange* range=nullptr;
    if(!s->composition_){
      ITfInsertAtSelection* insert=nullptr;HRESULT hr=context_->QueryInterface(IID_ITfInsertAtSelection,(void**)&insert);
      if(FAILED(hr))return hr;hr=insert->InsertTextAtSelection(cookie,TF_IAS_QUERYONLY,nullptr,0,&range);insert->Release();if(FAILED(hr))return hr;
      ITfContextComposition* cc=nullptr;hr=context_->QueryInterface(IID_ITfContextComposition,(void**)&cc);
      if(SUCCEEDED(hr)){hr=cc->StartComposition(cookie,range,s,&s->composition_);cc->Release();}
      if(FAILED(hr)||!s->composition_){range->Release();return E_FAIL;}s->composingContext_=context_;context_->AddRef();
      ITfSource* source=nullptr;if(SUCCEEDED(context_->QueryInterface(IID_ITfSource,(void**)&source))){source->AdviseSink(IID_ITfTextEditSink,static_cast<ITfTextEditSink*>(s),&s->editSink_);source->Release();}
    }else s->composition_->GetRange(&range);
    if(!range)return E_FAIL;
    HRESULT hr=range->SetText(cookie,0,next.preedit,(LONG)wcslen(next.preedit));
    if(SUCCEEDED(hr)){range->Collapse(cookie,TF_ANCHOR_END);TF_SELECTION sel={range,{TF_AE_NONE,FALSE}};hr=context_->SetSelection(cookie,1,&sel);}
    range->Release();if(FAILED(hr))return hr;
  }else if(!*next.commit && s->composition_)finish(L"");
  s->view_=next;s->show(context_,cookie);return S_OK;
}
class Factory final:public IClassFactory {
  LONG refs_=1;
public:
  Factory(){++objects;}~Factory(){--objects;}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID i,void** p) override{if(!p)return E_POINTER;*p=nullptr;if(i==IID_IUnknown||i==IID_IClassFactory){*p=this;AddRef();return S_OK;}return E_NOINTERFACE;}
  ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&refs_);}
  ULONG STDMETHODCALLTYPE Release() override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
  HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID i,void** p) override{if(outer)return CLASS_E_NOAGGREGATION;auto s=new Service;HRESULT h=s->QueryInterface(i,p);s->Release();return h;}
  HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override{objects+=lock?1:-1;return S_OK;}
};
extern "C" BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
extern "C" HRESULT __stdcall DllCanUnloadNow(){return objects==0?S_OK:S_FALSE;}
extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID c,REFIID i,void** p){if(c!=CLSID_ChaoxiPinyin)return CLASS_E_CLASSNOTAVAILABLE;auto f=new Factory;HRESULT hr=f->QueryInterface(i,p);f->Release();return hr;}
static std::wstring clsidKey(){wchar_t text[40];StringFromGUID2(CLSID_ChaoxiPinyin,text,40);return std::wstring(L"Software\\Classes\\CLSID\\")+text;}
extern "C" HRESULT __stdcall DllUnregisterServer(){
  HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);ITfInputProcessorProfiles* p=nullptr;
  if(SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_ITfInputProcessorProfiles,(void**)&p))){p->RemoveLanguageProfile(CLSID_ChaoxiPinyin,language,PROFILE_ChaoxiPinyin);p->Unregister(CLSID_ChaoxiPinyin);p->Release();}
  ITfCategoryMgr* c=nullptr;if(SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr,nullptr,CLSCTX_INPROC_SERVER,IID_ITfCategoryMgr,(void**)&c))){for(const GUID* g:{&GUID_TFCAT_TIP_KEYBOARD,&GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT})c->UnregisterCategory(CLSID_ChaoxiPinyin,*g,CLSID_ChaoxiPinyin);c->Release();}
  auto result=RegDeleteTreeW(HKEY_LOCAL_MACHINE,clsidKey().c_str());if(SUCCEEDED(init))CoUninitialize();return result==ERROR_SUCCESS||result==ERROR_FILE_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(result);
}
extern "C" HRESULT __stdcall DllRegisterServer(){
  HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);wchar_t path[32768];GetModuleFileNameW(instance,path,32768);
  HKEY key;auto code=RegCreateKeyExW(HKEY_LOCAL_MACHINE,(clsidKey()+L"\\InprocServer32").c_str(),0,nullptr,0,KEY_WRITE,nullptr,&key,nullptr);
  if(code!=ERROR_SUCCESS){if(SUCCEEDED(init))CoUninitialize();return HRESULT_FROM_WIN32(code);}
  RegSetValueExW(key,nullptr,0,REG_SZ,(BYTE*)path,(DWORD)((wcslen(path)+1)*2));RegSetValueExW(key,L"ThreadingModel",0,REG_SZ,(const BYTE*)L"Apartment",20);RegCloseKey(key);
  ITfInputProcessorProfiles* p=nullptr;HRESULT hr=CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_ITfInputProcessorProfiles,(void**)&p);
  if(SUCCEEDED(hr)){hr=p->Register(CLSID_ChaoxiPinyin);if(SUCCEEDED(hr)){const wchar_t* name=L"潮汐拼音 · 离线拼音";hr=p->AddLanguageProfile(CLSID_ChaoxiPinyin,language,PROFILE_ChaoxiPinyin,name,(ULONG)wcslen(name),path,(ULONG)wcslen(path),0);}if(SUCCEEDED(hr))hr=p->EnableLanguageProfile(CLSID_ChaoxiPinyin,language,PROFILE_ChaoxiPinyin,TRUE);p->Release();}
  ITfCategoryMgr* c=nullptr;if(SUCCEEDED(hr))hr=CoCreateInstance(CLSID_TF_CategoryMgr,nullptr,CLSCTX_INPROC_SERVER,IID_ITfCategoryMgr,(void**)&c);
  if(c){for(const GUID* g:{&GUID_TFCAT_TIP_KEYBOARD,&GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT}){if(SUCCEEDED(hr))hr=c->RegisterCategory(CLSID_ChaoxiPinyin,*g,CLSID_ChaoxiPinyin);}c->Release();}
  if(FAILED(hr))DllUnregisterServer();if(SUCCEEDED(init))CoUninitialize();return hr;
}
