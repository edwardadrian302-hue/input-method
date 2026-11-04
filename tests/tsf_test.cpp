#include "common.hpp"
#include <msctf.h>
#include <textstor.h>
#include <iostream>
#include "thread_manager.hpp"
// A real TSF text store: exercises Windows edit sessions, ranges and composition,
// without registering the IME or changing the user's keyboard settings.
class Store final:public ITextStoreACP {
  LONG refs_=1;
public:
  ITextStoreACPSink* sink=nullptr;std::wstring text;LONG start=0,end=0;HWND window;
  Store(){window=CreateWindowW(L"EDIT",L"",WS_POPUP,20,20,500,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);}
  ~Store(){if(sink)sink->Release();DestroyWindow(window);}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID i,void** p)override{*p=nullptr;if(i==IID_IUnknown||i==IID_ITextStoreACP){*p=this;AddRef();return S_OK;}return E_NOINTERFACE;}
  ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
  ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
  HRESULT STDMETHODCALLTYPE AdviseSink(REFIID i,IUnknown* p,DWORD)override{if(i!=IID_ITextStoreACPSink)return E_NOINTERFACE;if(sink)sink->Release();return p->QueryInterface(i,(void**)&sink);}
  HRESULT STDMETHODCALLTYPE UnadviseSink(IUnknown*)override{if(sink){sink->Release();sink=nullptr;}return S_OK;}
  HRESULT STDMETHODCALLTYPE RequestLock(DWORD f,HRESULT* result)override{*result=sink?sink->OnLockGranted(f):E_FAIL;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetStatus(TS_STATUS* s)override{s->dwDynamicFlags=0;s->dwStaticFlags=TS_SS_NOHIDDENTEXT;return S_OK;}
  HRESULT STDMETHODCALLTYPE QueryInsert(LONG a,LONG b,ULONG,LONG* x,LONG* y)override{*x=a;*y=b;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetSelection(ULONG,ULONG n,TS_SELECTION_ACP* s,ULONG* count)override{*count=n?1:0;if(n)*s={start,end,{TS_AE_END,FALSE}};return S_OK;}
  HRESULT STDMETHODCALLTYPE SetSelection(ULONG n,const TS_SELECTION_ACP* s)override{if(n){start=s->acpStart;end=s->acpEnd;}return S_OK;}
  HRESULT STDMETHODCALLTYPE GetText(LONG a,LONG b,WCHAR* out,ULONG cap,ULONG* read,TS_RUNINFO* run,ULONG runs,ULONG* used,LONG* next)override{
    if(b<0)b=(LONG)text.size();ULONG n=std::min<ULONG>(cap,std::max(0L,b-a));if(n)memcpy(out,text.data()+a,n*2);*read=n;*used=runs?1:0;if(runs)run[0]={n,TS_RT_PLAIN};*next=a+n;return S_OK;
  }
  HRESULT STDMETHODCALLTYPE SetText(DWORD,LONG a,LONG b,const WCHAR* s,ULONG n,TS_TEXTCHANGE* c)override{if(a<0||b<a||b>(LONG)text.size())return E_INVALIDARG;text.replace(a,b-a,s?s:L"",n);if(c)*c={a,b,a+(LONG)n};start=end=a+n;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetFormattedText(LONG,LONG,IDataObject**)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetEmbedded(LONG,REFGUID,REFIID,IUnknown**)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(const GUID*,const FORMATETC*,BOOL* yes)override{*yes=FALSE;return S_OK;}
  HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD,LONG,LONG,IDataObject*,TS_TEXTCHANGE*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE InsertTextAtSelection(DWORD f,const WCHAR* s,ULONG n,LONG* a,LONG* b,TS_TEXTCHANGE* c)override{if(a)*a=start;if(b)*b=end;if(f&TS_IAS_QUERYONLY)return S_OK;LONG old=start;auto hr=SetText(0,start,end,s,n,c);if(a)*a=old;if(b)*b=old+n;return hr;}
  HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(DWORD,IDataObject*,LONG*,LONG*,TS_TEXTCHANGE*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD,ULONG,const TS_ATTRID*)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG,ULONG,const TS_ATTRID*,DWORD)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG,ULONG,const TS_ATTRID*,DWORD)override{return S_OK;}
  HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG,LONG halt,ULONG,const TS_ATTRID*,DWORD,LONG* next,BOOL* found,LONG* offset)override{*next=halt;*found=FALSE;*offset=0;return S_OK;}
  HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG,TS_ATTRVAL*,ULONG* n)override{*n=0;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetEndACP(LONG* n)override{*n=(LONG)text.size();return S_OK;}
  HRESULT STDMETHODCALLTYPE GetActiveView(TsViewCookie* v)override{*v=0;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie,const POINT*,DWORD,LONG*)override{return E_NOTIMPL;}
  HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie,LONG,LONG,RECT* r,BOOL* clipped)override{*r={20,20,200,44};*clipped=FALSE;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie,RECT* r)override{*r={0,0,1000,800};return S_OK;}
  HRESULT STDMETHODCALLTYPE GetWnd(TsViewCookie,HWND* w)override{*w=window;return S_OK;}
};
static void check(bool good,const char* m){if(!good)throw std::runtime_error(m);}
static void pump(){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}
int wmain(){
  PROCESS_INFORMATION process={};HMODULE dll=nullptr;ITfThreadMgr* mgr=nullptr;ITfDocumentMgr* doc=nullptr;ITfContext* ctx=nullptr;ITfTextInputProcessor* tip=nullptr;ITfKeyEventSink* keys=nullptr;Store* store=nullptr;bool active=false,ownBroker=false;int result=1;
  auto testInstance=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
  SetEnvironmentVariableW(L"JIANPIN_TEST_INSTANCE",testInstance.c_str());
  CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  try{
    Response r;Request q;
    check(!exchange(q,r),"Test namespace is already in use");
    auto root=moduleDir();auto home=root.parent_path()/L"test-output"/(L"tsf-"+std::to_wstring(GetTickCount64()));
    auto exe=root/L"JianPinBroker.exe";std::wstring cmd=L"\""+exe.wstring()+L"\" --data-dir \""+home.wstring()+L"\"";
    STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;
    check(CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.c_str(),&si,&process),"start test broker");
    bool ready=false;for(int i=0;i<300;++i){check(WaitForSingleObject(process.hProcess,0)==WAIT_TIMEOUT,"test broker exited during startup (restricted sandbox or another instance)");if(exchange(q,r)){ready=true;break;}Sleep(100);}check(ready,"broker ready within 30s");
    auto name=pipeName();WaitNamedPipeW(name.c_str(),1000);HANDLE connection=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);ULONG server=0;
    if(connection!=INVALID_HANDLE_VALUE){GetNamedPipeServerProcessId(connection,&server);CloseHandle(connection);}ownBroker=server==process.dwProcessId;check(ownBroker,"test is connected to its own isolated broker");
    // Stress response delivery before involving TSF.
    for(int i=0;i<100;++i){check(exchange(q,r),"100 consecutive IPC responses");}
    check(SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr,nullptr,CLSCTX_INPROC_SERVER,IID_ITfThreadMgr,(void**)&mgr)),"TSF manager");
    TfClientId id;check(SUCCEEDED(mgr->Activate(&id)),"TSF activation");
    dll=LoadLibraryW((root/L"JianPin.dll").c_str());check(dll!=nullptr,"load IME DLL");
    auto factory=(HRESULT(__stdcall*)(REFCLSID,REFIID,void**))GetProcAddress(dll,"DllGetClassObject");
    CLSID clsid;CLSIDFromString(L"{3719C10E-98BF-4B9E-A4F2-D3CEBC8FB692}",&clsid);IClassFactory* cf=nullptr;
    check(factory && SUCCEEDED(factory(clsid,IID_IClassFactory,(void**)&cf)),"class factory");
    HRESULT hr=cf->CreateInstance(nullptr,IID_ITfTextInputProcessor,(void**)&tip);cf->Release();check(SUCCEEDED(hr),"create text service");
    auto proxy=new TestManager(mgr);hr=tip->Activate(proxy,id);proxy->Release();if(FAILED(hr))std::cerr<<"Activate HRESULT=0x"<<std::hex<<(unsigned long)hr<<" client="<<id<<std::dec<<"\n";check(SUCCEEDED(hr),"activate text service");active=true;
    check(SUCCEEDED(tip->QueryInterface(IID_ITfKeyEventSink,(void**)&keys)),"key event sink");
    check(SUCCEEDED(mgr->CreateDocumentMgr(&doc)),"create document");store=new Store;TfEditCookie cookie;
    check(SUCCEEDED(doc->CreateContext(id,0,store,&ctx,&cookie)),"create context");check(SUCCEEDED(doc->Push(ctx)),"push context");mgr->SetFocus(doc);pump();
    auto key=[&](WPARAM vk){BOOL eaten=FALSE;keys->OnTestKeyDown(ctx,vk,0,&eaten);check(eaten,"test key accepted");keys->OnKeyDown(ctx,vk,0,&eaten);pump();check(eaten,"edit session handled key");};
    for(char c:std::string("XUE"))key(c);
    check(store->text.find(L"xue")!=std::wstring::npos,"third full-pinyin letter reaches TSF composition");key(VK_ESCAPE);
    for(char c:std::string("NIHAO"))key(c);
    check(!store->text.empty(),"composition inserted into TSF text store");
    key(VK_SPACE);check(store->text==L"你好","composition commits exactly once");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_ESCAPE);check(store->text==L"你好","escape removes preedit only");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);check(store->text==L"你好中国","second composition appends");
    for(char c:std::string("NI"))key(c);keys->OnSetFocus(FALSE);pump();check(store->text==L"你好中国","focus loss cancels old composition");keys->OnSetFocus(TRUE);pump();
    ITfCompartmentMgr* cm=nullptr;ctx->QueryInterface(IID_ITfCompartmentMgr,(void**)&cm);ITfCompartment* disabled=nullptr;cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_DISABLED,&disabled);VARIANT value;VariantInit(&value);value.vt=VT_I4;value.lVal=1;disabled->SetValue(id,&value);
    BOOL guarded=TRUE;keys->OnTestKeyDown(ctx,'N',0,&guarded);check(!guarded,"disabled/password context bypasses IME");value.lVal=0;disabled->SetValue(id,&value);disabled->Release();cm->Release();
    keys->OnKeyDown(ctx,VK_SHIFT,0,(BOOL*)&result);keys->OnKeyUp(ctx,VK_SHIFT,0,(BOOL*)&result);
    BOOL eaten=TRUE;keys->OnTestKeyDown(ctx,'A',0,&eaten);check(!eaten,"Shift switches to English passthrough");
    std::cout<<"PASS: TSF composition/commit/cancel/focus/disabled context/English mode and 100 IPC requests (keyboard subscription mocked).\n";result=0;
  }catch(const std::exception& e){std::cerr<<"FAIL TSF: "<<e.what()<<"\n";result=1;}
  if(keys)keys->Release();if(active)tip->Deactivate();if(tip)tip->Release();if(doc)doc->Pop(TF_POPF_ALL);if(ctx)ctx->Release();if(doc)doc->Release();if(store)store->Release();if(mgr){mgr->Deactivate();mgr->Release();}pump();
  if(dll){auto unload=(HRESULT(__stdcall*)())GetProcAddress(dll,"DllCanUnloadNow");if(unload && unload()!=S_OK){std::cerr<<"FAIL: leaked TSF object\n";result=1;}else FreeLibrary(dll);}
  if(process.hProcess){if(ownBroker){Request q;q.op=Op::Shutdown;Response r;exchange(q,r,3000);}if(WaitForSingleObject(process.hProcess,5000)!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,1);}CloseHandle(process.hProcess);CloseHandle(process.hThread);}
  CoUninitialize();return result;
}
