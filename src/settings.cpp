#include "common.hpp"
static HWND checkBox,fontBox,statusText;
static bool ready=false;
static bool send(HWND window,Op op,int value=0) {
  Request q;q.op=op;q.value=value;Response r;
  if(!exchange(q,r,3000)){MessageBoxW(window,L"本地引擎尚未就绪，或操作失败。首次准备词库可能需要稍候。",L"简拼",MB_OK|MB_ICONWARNING);return false;}
  SendMessageW(checkBox,BM_SETCHECK,r.learning?BST_CHECKED:BST_UNCHECKED,0);
  SetWindowTextW(fontBox,std::to_wstring(r.font).c_str());return true;
}
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
  if(m==WM_CREATE){
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int h,int id){HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,h,w,(HMENU)(INT_PTR)id,nullptr,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return c;};
    add(L"STATIC",L"简拼 · 离线搜狗双拼",0,24,18,400,24,0);
    checkBox=add(L"BUTTON",L"开启本地学习",BS_AUTOCHECKBOX|WS_TABSTOP,24,56,280,28,101);
    add(L"STATIC",L"只记住 1–4 字中文词条和次数，不保存按时间排列的输入历史。\n关闭后不使用、不更新学习数据；重新开启可继续使用。",0,24,90,440,48,0);
    add(L"STATIC",L"候选字号（12–32）：",0,24,154,180,24,0);
    fontBox=add(L"EDIT",L"18",WS_BORDER|ES_NUMBER|WS_TABSTOP,205,150,70,28,102);
    add(L"BUTTON",L"应用字号",WS_TABSTOP,290,148,110,32,103);
    add(L"BUTTON",L"清除学习数据",WS_TABSTOP,24,204,160,36,104);
    add(L"STATIC",L"9 个候选 · 数字选词 · 空格首选 · - / = 翻页\n单按 Shift 切换中英文；Win + 空格切换系统输入法。",0,24,258,440,46,0);
    statusText=add(L"STATIC",L"正在连接本地引擎…",0,24,326,440,24,0);
    EnableWindow(checkBox,FALSE);EnableWindow(GetDlgItem(w,103),FALSE);EnableWindow(GetDlgItem(w,104),FALSE);
    launchBroker(moduleDir());SetTimer(w,1,750,nullptr);return 0;
  }
  if(m==WM_TIMER){Request q;Response r;if(exchange(q,r)){if(!ready){SendMessageW(checkBox,BM_SETCHECK,r.learning?BST_CHECKED:BST_UNCHECKED,0);SetWindowTextW(fontBox,std::to_wstring(r.font).c_str());}ready=true;EnableWindow(checkBox,TRUE);EnableWindow(GetDlgItem(w,103),TRUE);EnableWindow(GetDlgItem(w,104),TRUE);SetWindowTextW(statusText,r.storageError?L"输入正常，但词频保存失败，请检查磁盘权限。":L"本地引擎已就绪 · 无需网络");KillTimer(w,1);}return 0;}
  if(m==WM_COMMAND){
    switch(LOWORD(wp)){
      case 101:send(w,Op::Learning,SendMessageW(checkBox,BM_GETCHECK,0,0)==BST_CHECKED);break;
      case 103:{wchar_t t[16];GetWindowTextW(fontBox,t,16);int n=_wtoi(t);if(n<12||n>32)MessageBoxW(w,L"字号请输入 12 到 32。",L"简拼",MB_OK);else send(w,Op::Font,n);break;}
      case 104:if(MessageBoxW(w,L"清除所有已学习的词条和词频？\n此操作无法撤销，内置词库和设置会保留。",L"清除学习数据",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)==IDYES){if(send(w,Op::Erase))MessageBoxW(w,L"学习数据已清除。",L"简拼",MB_OK);}break;
    }return 0;
  }
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
  SetProcessDPIAware();WNDCLASSW wc={};wc.hInstance=h;wc.lpfnWndProc=proc;wc.lpszClassName=L"JianPin.Settings";wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
  HWND w=CreateWindowW(wc.lpszClassName,L"简拼设置",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,510,415,nullptr,nullptr,h,nullptr);ShowWindow(w,show);
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}return 0;
}
