#include "common.hpp"
static HWND checkBox,fontBox,statusText;
static bool ready=false;
static HFONT uiFont,titleFont;
static int dpi=96;
static int px(int n){return MulDiv(n,dpi,96);}
static HBRUSH background=CreateSolidBrush(RGB(248,250,245));
static bool send(HWND window,Op op,int value=0) {
  Request q;q.op=op;q.value=value;Response r;
  if(!exchange(q,r,3000)){MessageBoxW(window,L"本地引擎尚未就绪，或操作失败。首次准备词库可能需要稍候。",L"简拼",MB_OK|MB_ICONWARNING);return false;}
  SendMessageW(checkBox,BM_SETCHECK,r.learning?BST_CHECKED:BST_UNCHECKED,0);
  SetWindowTextW(fontBox,std::to_wstring(r.font).c_str());return true;
}
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
  if(m==WM_CREATE){
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int h,int id){HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(width),px(h),w,(HMENU)(INT_PTR)id,nullptr,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)uiFont,TRUE);return c;};
    HWND title=add(L"STATIC",L"简拼 JianPin",0,28,22,450,36,0);
    SendMessageW(title,WM_SETFONT,(WPARAM)titleFont,TRUE);
    add(L"STATIC",L"离线全拼 · 简单输入，安心表达",0,28,62,460,24,0);
    checkBox=add(L"BUTTON",L"开启本地学习",BS_AUTOCHECKBOX|WS_TABSTOP,28,112,440,28,101);
    add(L"STATIC",L"只记住 1–4 字中文词条和次数，不保存按时间排列的输入历史。\n关闭后不使用、不更新学习数据；重新开启可继续使用。",0,28,152,478,60,0);
    add(L"STATIC",L"候选字号（12–32）：",0,28,230,190,28,0);
    fontBox=add(L"EDIT",L"18",WS_BORDER|ES_NUMBER|WS_TABSTOP,226,226,70,30,102);
    add(L"BUTTON",L"应用字号",WS_TABSTOP,314,225,130,34,103);
    add(L"BUTTON",L"清除学习数据",WS_TABSTOP,28,284,180,36,104);
    add(L"STATIC",L"全拼：xue → 学，nihao → 你好\n数字 1–9 选词 · 空格首选 · - / = 翻页\n单按 Shift 切换中英文；Win + 空格切换系统输入法。",0,28,348,478,72,0);
    statusText=add(L"STATIC",L"正在连接本地引擎…",0,28,428,478,28,0);
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
  if(m==WM_CTLCOLORSTATIC || m==WM_CTLCOLORBTN){HDC dc=(HDC)wp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(62,73,53));SetBkColor(dc,RGB(248,250,245));return (LRESULT)background;}
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
  SetProcessDPIAware();HDC dc=GetDC(nullptr);dpi=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(nullptr,dc);
  uiFont=CreateFontW(-px(15),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  titleFont=CreateFontW(-px(26),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  WNDCLASSW wc={};wc.hInstance=h;wc.lpfnWndProc=proc;wc.lpszClassName=L"JianPin.Settings";wc.hbrBackground=background;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
  HWND w=CreateWindowW(wc.lpszClassName,L"简拼设置",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,px(560),px(525),nullptr,nullptr,h,nullptr);ShowWindow(w,show);
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);return 0;
}
