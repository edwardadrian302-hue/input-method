#include "common.hpp"
#include "candidate.hpp"
#include <thread>
#include <memory>
static HWND checks[10]={},fontBox,statusText,previewWindow;
static HFONT uiFont,titleFont;static int dpi=96;static bool ready=false,busy=false;static std::thread worker;
static HBRUSH background=CreateSolidBrush(RGB(245,249,249));
static int px(int n){return MulDiv(n,dpi,96);}
static const int sizes[]={10,11,12,14,16,18,20,24};
static const int fuzzyFlags[]={FuzzyZ,FuzzyC,FuzzyS,FuzzyNL,FuzzyEn,FuzzyAn,FuzzyIn};
static int fontValue(){int i=(int)SendMessageW(fontBox,CB_GETCURSEL,0,0);return i>=0&&i<8?sizes[i]:12;}
static void controls(HWND w,bool enable){for(HWND c:checks)EnableWindow(c,enable);EnableWindow(fontBox,enable);EnableWindow(GetDlgItem(w,103),enable);EnableWindow(GetDlgItem(w,104),enable);}
static void load(const Response& r){
  SendMessageW(checks[0],BM_SETCHECK,r.learning?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[1],BM_SETCHECK,(r.options&OptAsciiPunct)?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[2],BM_SETCHECK,(r.options&OptCompletion)?BST_CHECKED:BST_UNCHECKED,0);
  for(int i=0;i<7;++i)SendMessageW(checks[i+3],BM_SETCHECK,(r.fuzzy&fuzzyFlags[i])?BST_CHECKED:BST_UNCHECKED,0);
  int n=2;for(int i=0;i<8;++i)if(sizes[i]==r.font)n=i;SendMessageW(fontBox,CB_SETCURSEL,n,0);InvalidateRect(previewWindow,nullptr,TRUE);
}
struct Result {bool ok;Response response;};
static void apply(HWND w,Request q){
  busy=true;controls(w,false);SetWindowTextW(statusText,L"正在保存并准备离线索引，请稍候…");
  worker=std::thread([w,q]{auto result=new Result{};result->ok=exchange(q,result->response,120000);if(!PostMessageW(w,WM_APP+1,0,(LPARAM)result))delete result;});
}
static LRESULT CALLBACK preview(HWND w,UINT m,WPARAM wp,LPARAM lp){
  if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT rect;GetClientRect(w,&rect);FillRect(dc,&rect,background);
    Response r;r.font=fontValue();r.count=9;r.last=0;copyText(r.preedit,L"qi shang ba xia");
    const wchar_t* words[]={L"七上八下",L"七上",L"其",L"起",L"期",L"气",L"器",L"七",L"齐"};for(int i=0;i<9;++i)copyText(r.candidates[i],words[i]);
    CandidatePanel panel;panel.layout(dc,r,dpi,rect.right,rect.bottom);panel.paint(dc,r);EndPaint(w,&ps);return 0;}
  return DefWindowProcW(w,m,wp,lp);
}
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
  if(m==WM_CREATE){
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(width),px(height),w,(HMENU)(INT_PTR)id,nullptr,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)uiFont,TRUE);return c;};
    auto title=add(L"STATIC",L"潮汐拼音",0,28,20,440,36,0);SendMessageW(title,WM_SETFONT,(WPARAM)titleFont,TRUE);
    add(L"STATIC",L"离线全拼  /  让文字自然流动",0,28,62,440,24,0);
    checks[0]=add(L"BUTTON",L"开启本地短词学习",BS_AUTOCHECKBOX|WS_TABSTOP,28,108,420,26,110);
    add(L"STATIC",L"仅记住 1–4 字中文词条和次数。关闭后不参与排序、不更新；\n清除后无法恢复，内置词库保留。",0,28,140,440,48,0);
    add(L"STATIC",L"候选字号",0,28,207,90,28,0);
    fontBox=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,124,202,100,240,102);
    for(int size:sizes)SendMessageW(fontBox,CB_ADDSTRING,0,(LPARAM)std::to_wstring(size).c_str());SendMessageW(fontBox,CB_SETCURSEL,2,0);
    add(L"STATIC",L"磅 · 窗口随内容与屏幕缩放",0,236,207,240,28,0);
    checks[1]=add(L"BUTTON",L"中文模式使用英文标点 , . ; :",BS_AUTOCHECKBOX|WS_TABSTOP,28,255,440,26,111);
    checks[2]=add(L"BUTTON",L"拼音补全与简拼混输（shenm / sm → 什么）",BS_AUTOCHECKBOX|WS_TABSTOP,28,298,450,26,112);
    add(L"STATIC",L"模糊音  ·  按需开启，默认关闭",0,28,348,440,24,0);
    const wchar_t* names[]={L"z ↔ zh",L"c ↔ ch",L"s ↔ sh",L"n ↔ l",L"en ↔ eng",L"an ↔ ang",L"in ↔ ing"};
    for(int i=0;i<7;++i)checks[i+3]=add(L"BUTTON",names[i],BS_AUTOCHECKBOX|WS_TABSTOP,28+(i%3)*145,383+(i/3)*34,140,26,113+i);
    add(L"STATIC",L"单按 Shift 或点击系统输入法的“中 / 英”按钮切换。\n英文模式直接输入英文标点。拼写设置应用时会重建离线索引。",0,28,493,460,48,0);
    add(L"BUTTON",L"应用设置",WS_TABSTOP|BS_DEFPUSHBUTTON,28,556,128,36,103);
    add(L"BUTTON",L"清除学习数据",WS_TABSTOP,172,556,160,36,104);
    add(L"STATIC",L"候选外观预览",0,520,108,300,24,0);
    previewWindow=add(L"ChaoxiPinyin.Preview",L"",0,520,146,310,404,0);
    add(L"STATIC",L"预览文字为示例；实际候选取决于输入。\n内置雾凇中文词库 · 全程离线",0,520,559,310,48,0);
    statusText=add(L"STATIC",L"正在准备本地引擎，首次启动可能需要稍候…",0,28,619,800,26,0);
    controls(w,false);launchBroker(moduleDir());SetTimer(w,1,1000,nullptr);return 0;
  }
  if(m==WM_TIMER&&!busy){Request q;Response r;if(exchange(q,r,100)){if(!ready)load(r);ready=true;controls(w,true);SetWindowTextW(statusText,r.storageError?L"输入正常，但词频保存失败，请检查磁盘权限。":L"本地引擎已就绪 · 设置保存在本机 · 无需网络");KillTimer(w,1);}return 0;}
  if(m==WM_APP+1){if(worker.joinable())worker.join();std::unique_ptr<Result> result((Result*)lp);busy=false;controls(w,true);
    if(result->ok){load(result->response);SetWindowTextW(statusText,L"设置已应用 · 下次输入生效 · 无需网络");}
    else{SetWindowTextW(statusText,L"操作未完成，请稍候重试；索引仍可能在准备中。");MessageBoxW(w,L"本地引擎未返回成功结果，请稍候重试。",L"潮汐拼音",MB_OK|MB_ICONWARNING);}return 0;}
  if(m==WM_COMMAND){
    if(LOWORD(wp)==102&&HIWORD(wp)==CBN_SELCHANGE)InvalidateRect(previewWindow,nullptr,TRUE);
    if(LOWORD(wp)==103&&!busy){Request q;q.op=Op::Configure;q.key=fontValue();q.value=0;q.mask=0;
      if(SendMessageW(checks[0],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptLearning;
      if(SendMessageW(checks[1],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptAsciiPunct;
      if(SendMessageW(checks[2],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptCompletion;
      for(int i=0;i<7;++i)if(SendMessageW(checks[i+3],BM_GETCHECK,0,0)==BST_CHECKED)q.mask|=fuzzyFlags[i];apply(w,q);
    }
    if(LOWORD(wp)==104&&!busy&&MessageBoxW(w,L"删除全部已学习的词条及次数？内置词库和设置保留。",L"清除学习数据",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)==IDYES){Request q;q.op=Op::Erase;apply(w,q);}return 0;
  }
  if(m==WM_CTLCOLORSTATIC||m==WM_CTLCOLORBTN){auto dc=(HDC)wp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(39,77,83));SetBkColor(dc,RGB(245,249,249));return (LRESULT)background;}
  if(m==WM_CLOSE&&busy){MessageBeep(MB_OK);return 0;}
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
  SetProcessDPIAware();HDC dc=GetDC(nullptr);dpi=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(nullptr,dc);
  RECT work={};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
  int fit=std::min(MulDiv(work.right-work.left-40,96,860),MulDiv(work.bottom-work.top-70,96,665));
  dpi=std::max(72,std::min(dpi,fit));
  uiFont=CreateFontW(-px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  titleFont=CreateFontW(-px(27),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  WNDCLASSW wc={};wc.hInstance=h;wc.lpfnWndProc=preview;wc.lpszClassName=L"ChaoxiPinyin.Preview";wc.hbrBackground=background;RegisterClassW(&wc);
  wc.hIcon=LoadIconW(h,MAKEINTRESOURCEW(1));wc.lpfnWndProc=proc;wc.lpszClassName=L"ChaoxiPinyin.Settings";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
  RECT rect={0,0,px(860),px(665)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
  HWND w=CreateWindowW(wc.lpszClassName,L"潮汐拼音设置",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,h,nullptr);ShowWindow(w,show);
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
  if(worker.joinable())worker.join();DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);return 0;
}
