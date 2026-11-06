#include "common.hpp"
#include <msctf.h>
#include <textstor.h>
#include "language_bar_compat.hpp"
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
  PROCESS_INFORMATION process={};HMODULE dll=nullptr;ITfThreadMgr* mgr=nullptr;ITfDocumentMgr* doc=nullptr;ITfContext* ctx=nullptr;ITfTextInputProcessor* tip=nullptr;ITfKeyEventSink* keys=nullptr;Store* store=nullptr;bool active=false,ownBroker=false;ITfLangBarItemMgr* testBar=nullptr;int result=1;
  auto testInstance=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
  SetEnvironmentVariableW(L"CHAOXI_TEST_INSTANCE",testInstance.c_str());
  CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  try{
    Response r;Request q;
    check(!exchange(q,r),"Test namespace is already in use");
    auto root=moduleDir().parent_path().parent_path()/L"dist";auto home=root.parent_path()/L"test-output"/(L"tsf-"+std::to_wstring(GetTickCount64()));
    auto exe=root/L"ChaoxiPinyinBroker.exe";std::wstring cmd=L"\""+exe.wstring()+L"\" --data-dir \""+home.wstring()+L"\"";
    STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;
    check(CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.c_str(),&si,&process),"start test broker");
    bool ready=false;for(int i=0;i<1200;++i){check(WaitForSingleObject(process.hProcess,0)==WAIT_TIMEOUT,"test broker exited during startup (restricted sandbox or another instance)");if(exchange(q,r)){ready=true;break;}Sleep(100);}check(ready,"broker ready within 120s");
    auto name=pipeName();WaitNamedPipeW(name.c_str(),1000);HANDLE connection=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);ULONG server=0;
    if(connection!=INVALID_HANDLE_VALUE){GetNamedPipeServerProcessId(connection,&server);CloseHandle(connection);}ownBroker=server==process.dwProcessId;check(ownBroker,"test is connected to its own isolated broker");
    // Stress response delivery before involving TSF.
    for(int i=0;i<100;++i){check(exchange(q,r),"100 consecutive IPC responses");}
    check(SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr,nullptr,CLSCTX_INPROC_SERVER,IID_ITfThreadMgr,(void**)&mgr)),"TSF manager");
    TfClientId id;check(SUCCEEDED(mgr->Activate(&id)),"TSF activation");
    dll=LoadLibraryW((root/L"ChaoxiPinyin.dll").c_str());check(dll!=nullptr,"load IME DLL");
    auto factory=(HRESULT(__stdcall*)(REFCLSID,REFIID,void**))GetProcAddress(dll,"DllGetClassObject");
    CLSID clsid;CLSIDFromString(L"{3719C10E-98BF-4B9E-A4F2-D3CEBC8FB692}",&clsid);IClassFactory* cf=nullptr;
    check(factory && SUCCEEDED(factory(clsid,IID_IClassFactory,(void**)&cf)),"class factory");
    HRESULT hr=cf->CreateInstance(nullptr,IID_ITfTextInputProcessor,(void**)&tip);cf->Release();check(SUCCEEDED(hr),"create text service");
    auto proxy=new TestManager(mgr);proxy->QueryInterface(IID_ITfLangBarItemMgr,(void**)&testBar);hr=tip->Activate(proxy,id);proxy->Release();if(FAILED(hr))std::cerr<<"Activate HRESULT=0x"<<std::hex<<(unsigned long)hr<<" client="<<id<<std::dec<<"\n";check(SUCCEEDED(hr),"activate text service");active=true;
    check(SUCCEEDED(tip->QueryInterface(IID_ITfKeyEventSink,(void**)&keys)),"key event sink");
    check(SUCCEEDED(mgr->CreateDocumentMgr(&doc)),"create document");store=new Store;TfEditCookie cookie;
    check(SUCCEEDED(doc->CreateContext(id,0,store,&ctx,&cookie)),"create context");check(SUCCEEDED(doc->Push(ctx)),"push context");mgr->SetFocus(doc);pump();
    auto key=[&](WPARAM vk){BOOL eaten=FALSE;keys->OnTestKeyDown(ctx,vk,0,&eaten);check(eaten,"test key accepted");keys->OnKeyDown(ctx,vk,0,&eaten);pump();if(!eaten)std::cerr<<"Unhandled virtual key: "<<vk<<"\n";check(eaten,"edit session handled key");};
    for(char c:std::string("XUE"))key(c);
    check(store->text.find(L"xue")!=std::wstring::npos,"third full-pinyin letter reaches TSF composition");key(VK_ESCAPE);
    for(char c:std::string("NIHAO"))key(c);
    check(!store->text.empty(),"composition inserted into TSF text store");
    key(VK_SPACE);check(store->text==L"你好","composition commits exactly once");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_ESCAPE);check(store->text==L"你好","escape removes preedit only");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);check(store->text==L"你好中国","second composition appends");
    for(char c:std::string("NI"))key(c);keys->OnSetFocus(FALSE);pump();check(store->text==L"你好中国","focus loss cancels old composition");keys->OnSetFocus(TRUE);pump();
    key(VK_OEM_COMMA);check(store->text==L"你好中国，","physical comma key reaches TSF commit");
    key(VK_OEM_1);check(store->text==L"你好中国，；","physical semicolon key reaches TSF commit");
    for(char c:std::string("NIHAO"))key(c);key(VK_OEM_COMMA);check(store->text==L"你好中国，；你好，","composition plus physical comma does not lose text");
    ITfCompartmentMgr* cm=nullptr;ctx->QueryInterface(IID_ITfCompartmentMgr,(void**)&cm);ITfCompartment* disabled=nullptr;cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_DISABLED,&disabled);VARIANT value;VariantInit(&value);value.vt=VT_I4;value.lVal=1;disabled->SetValue(id,&value);
    BOOL guarded=TRUE;keys->OnTestKeyDown(ctx,'N',0,&guarded);check(!guarded,"disabled/password context bypasses IME");value.lVal=0;disabled->SetValue(id,&value);disabled->Release();cm->Release();
    for(char c:std::string("NIHAO"))key(c);
    BOOL shift=FALSE;keys->OnTestKeyDown(ctx,VK_SHIFT,0,&shift);check(shift,"Shift test key accepted");
    keys->OnKeyDown(ctx,VK_SHIFT,0,&shift);check(shift,"Shift down consumed consistently");
    keys->OnTestKeyUp(ctx,VK_SHIFT,0,&shift);check(shift,"Shift key up routed to IME");
    keys->OnKeyUp(ctx,VK_SHIFT,0,&shift);pump();check(shift,"Shift toggle handled");
    check(store->text==L"你好中国，；你好，nihao","Shift commits original letters without generated pinyin spaces or Chinese candidates");
    ITfLangBarItemMgr* bar=testBar;bar->AddRef();
    const GUID inputMode={0x2c77a81e,0x41cc,0x4178,{0xa3,0xa7,0x5f,0x8a,0x98,0x75,0x68,0xe6}};
    ITfLangBarItem* item=nullptr;auto barResult=bar->GetItem(inputMode,&item);bar->Release();check(SUCCEEDED(barResult)&&item,"input mode button registered with language bar");
    BSTR tooltip=nullptr;auto tooltipResult=item->GetTooltipString(&tooltip);bool englishTooltip=SUCCEEDED(tooltipResult)&&tooltip&&std::wstring(tooltip).find(L"英文")!=std::wstring::npos;SysFreeString(tooltip);item->Release();check(englishTooltip,"mode button exposes current English state");
    BOOL punctuation=TRUE;keys->OnTestKeyDown(ctx,VK_OEM_COMMA,0,&punctuation);check(!punctuation,"English mode passes ASCII comma through");
    keys->OnTestKeyDown(ctx,VK_OEM_1,0,&punctuation);check(!punctuation,"English mode passes ASCII semicolon through");
    BOOL eaten=TRUE;keys->OnTestKeyDown(ctx,'A',0,&eaten);check(!eaten,"Shift switches to English passthrough");
    bar=testBar;bar->AddRef();item=nullptr;check(SUCCEEDED(bar->GetItem(inputMode,&item)),"mode button can be retrieved");bar->Release();
    ModeButtonInterface* button=nullptr;check(SUCCEEDED(item->QueryInterface(IID_ModeButton,(void**)&button)),"mode button click interface");
    HICON icon=nullptr;check(SUCCEEDED(button->GetIcon(&icon))&&icon,"mode icon generated");DestroyIcon(icon);
    check(SUCCEEDED(button->OnClick(2,{0,0},nullptr)),"click toggles mode");button->Release();
    tooltip=nullptr;item->GetTooltipString(&tooltip);bool chineseTooltip=tooltip&&std::wstring(tooltip).find(L"中文")!=std::wstring::npos;SysFreeString(tooltip);item->Release();check(chineseTooltip,"click updates Chinese state");
    eaten=FALSE;keys->OnTestKeyDown(ctx,'A',0,&eaten);check(eaten,"click restores Chinese key handling");
    auto shiftToggle=[&](){BOOL handled=FALSE;keys->OnTestKeyDown(ctx,VK_SHIFT,0,&handled);check(handled,"Shift routed");keys->OnKeyDown(ctx,VK_SHIFT,0,&handled);keys->OnTestKeyUp(ctx,VK_SHIFT,0,&handled);check(handled,"Shift release routed");keys->OnKeyUp(ctx,VK_SHIFT,0,&handled);pump();check(handled,"Shift toggled");};
    auto beforeEnglish=store->text;
    for(char c:std::string("HELLO"))key(c);shiftToggle();
    check(store->text==beforeEnglish+L"hello","English word is committed literally by Shift");
    shiftToggle();check(store->text==beforeEnglish+L"hello","empty Shift toggle inserts no duplicate text");
    for(char c:std::string("NIHAOX"))key(c);key(VK_BACK);shiftToggle();
    check(store->text==beforeEnglish+L"hellonihao","Shift commits edited raw input");shiftToggle();
    auto chooseEnglish=[&](const std::string& input,const wchar_t* expected){
      auto before=store->text;
      Request probe;probe.client=987;probe.op=Op::Clear;Response candidates;check(exchange(probe,candidates),"clear English probe session");
      probe.op=Op::Key;for(char c:input){probe.key=c;check(exchange(probe,candidates),"query isolated English candidate order");key((WPARAM)(c-'a'+'A'));}
      int index=-1;for(int i=0;i<candidates.count;++i)if(std::wstring(candidates.candidates[i])==expected)index=i;
      check(index>=0,"English candidate visible");key('1'+index);
      check(store->text==before+expected,"English candidate inserts full text exactly once");
      BOOL chinese=FALSE;keys->OnTestKeyDown(ctx,'A',0,&chinese);check(chinese,"English candidate selection preserves Chinese mode");
    };
    chooseEnglish("build",L"build");chooseEnglish("buil",L"build");chooseEnglish("nihao",L"nihao");
    auto beforeChinese=store->text;for(char c:std::string("NIHAO"))key(c);key(VK_SPACE);check(store->text==beforeChinese+L"你好","Chinese space selection remains available after English candidates");
    auto beforePaging=store->text;
    Request pagingProbe;pagingProbe.client=988;pagingProbe.op=Op::Clear;Response pagingState;
    check(exchange(pagingProbe,pagingState),"clear isolated paging probe");
    pagingProbe.op=Op::Key;pagingProbe.key='y';check(exchange(pagingProbe,pagingState),"get y first page");
    auto firstPagingWord=std::wstring(pagingState.candidates[0]);
    pagingProbe.key=0xff56;check(exchange(pagingProbe,pagingState)&&pagingState.page==1&&pagingState.count==9,"y second page contains nine items over IPC");
    auto ninthPagingWord=std::wstring(pagingState.candidates[8]);
    key('Y');key(VK_NEXT);key('9');
    check(store->text==beforePaging+ninthPagingWord,"TSF PageDown and numeric nine choose the filled second-page item exactly once");
    beforePaging=store->text;key('Y');key(VK_NEXT);key(VK_PRIOR);key(VK_SPACE);
    check(store->text==beforePaging+firstPagingWord,"TSF PageUp restores the original first page and space selection");
    pagingProbe.op=Op::Clear;check(exchange(pagingProbe,pagingState),"clear arrow probe");
    pagingProbe.op=Op::Key;pagingProbe.key='y';check(exchange(pagingProbe,pagingState),"get arrow candidate order");
    auto arrowWord=std::wstring(pagingState.candidates[1]);
    auto beforeArrows=store->text;key('Y');key(VK_DOWN);key(VK_UP);key(VK_DOWN);key(VK_SPACE);
    check(store->text==beforeArrows+arrowWord,"TSF arrows and space commit highlighted merged candidate");
    pagingProbe.key=0xff56;check(exchange(pagingProbe,pagingState),"get arrow page two");arrowWord=pagingState.candidates[1];
    beforeArrows=store->text;key('Y');key(VK_NEXT);key(VK_DOWN);key(VK_SPACE);
    check(store->text==beforeArrows+arrowWord,"TSF arrows preserve second page for space selection");
    q=Request{};q.op=Op::PhrasePut;copyText(q.code,L"zhong guo ce shi hou zhui");copyText(q.text,L"中国测试后缀");check(exchange(q,r),"add isolated prediction fixture");
    q=Request{};q.op=Op::Configure;q.value=OptCompletion|OptLearning|OptPrediction;q.key=12;check(exchange(q,r),"enable offline prediction");
    auto beforePrediction=store->text;
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);
    check(store->text==beforePrediction+L"中国","prediction header never enters the text store");
    BOOL predictionArrow=FALSE;keys->OnTestKeyDown(ctx,VK_DOWN,0,&predictionArrow);check(predictionArrow,"TSF routes prediction arrow");
    key(VK_DOWN);key(VK_UP);check(store->text==beforePrediction+L"中国","prediction arrows never replay committed prefix");
    key('1');check(store->text==beforePrediction+L"中国测试后缀","prediction commits only suffix in a fresh TSF insertion");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);auto afterChina=store->text;key(VK_ESCAPE);
    check(store->text==afterChina,"Escape on prediction preserves already committed text");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);afterChina=store->text;
    keys->OnSetFocus(FALSE);keys->OnSetFocus(TRUE);pump();eaten=TRUE;keys->OnTestKeyDown(ctx,'1',0,&eaten);check(!eaten&&store->text==afterChina,"focus changes dismiss predictions without consuming digits");
    for(char c:std::string("ZHONGGUO"))key(c);key(VK_SPACE);afterChina=store->text;
    keys->OnKeyDown(ctx,VK_SHIFT,0,&shift);keys->OnKeyUp(ctx,VK_SHIFT,0,&shift);pump();
    check(store->text==afterChina,"mode switch dismisses prediction without accepting a suffix");
    std::cout<<"PASS: TSF composition/commit/cancel/focus/disabled context/English mode, punctuation, Shift commit, language bar state, nine-item merged paging and 100 IPC requests (keyboard and language-bar subscriptions mocked).\n";result=0;
  }catch(const std::exception& e){std::cerr<<"FAIL TSF: "<<e.what()<<"\n";result=1;}
  if(testBar)testBar->Release();if(keys)keys->Release();if(active)tip->Deactivate();if(tip)tip->Release();if(doc)doc->Pop(TF_POPF_ALL);if(ctx)ctx->Release();if(doc)doc->Release();if(store)store->Release();if(mgr){mgr->Deactivate();mgr->Release();}pump();
  if(dll){auto unload=(HRESULT(__stdcall*)())GetProcAddress(dll,"DllCanUnloadNow");if(unload && unload()!=S_OK){std::cerr<<"FAIL: leaked TSF object\n";result=1;}else FreeLibrary(dll);}
  if(process.hProcess){if(ownBroker){Request q;q.op=Op::Shutdown;Response r;exchange(q,r,3000);}if(WaitForSingleObject(process.hProcess,5000)!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,1);}CloseHandle(process.hProcess);CloseHandle(process.hThread);}
  CoUninitialize();return result;
}
