// Render our own hidden windows without opening a broker or touching user data.
#define CHAOXI_SETTINGS_PREVIEW
#define wWinMain settingsMain
#include <windows.h>
static void renderSettings(HWND);
#include "../src/settings.cpp"
#include <iostream>
static void snapshot(HWND w,const fs::path& path){
  RECT rect;GetClientRect(w,&rect);HDC dc=GetDC(w),mem=CreateCompatibleDC(dc);
  BITMAPINFO info={};info.bmiHeader={sizeof(BITMAPINFOHEADER),rect.right,-rect.bottom,1,32,BI_RGB,0,0,0,0,0};void* bits=nullptr;
  auto bmp=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(mem,bmp);
  if(w==mainWindow)paintSettings(mem);else paintPhraseWindow(mem);
  for(HWND child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
    if(!(GetWindowLongW(child,GWL_STYLE)&WS_VISIBLE))continue;
    RECT box;GetWindowRect(child,&box);MapWindowPoints(nullptr,w,(POINT*)&box,2);
    if(box.left<0||box.top<0||box.right>rect.right||box.bottom>rect.bottom)throw std::runtime_error("Control extends outside client area");
    wchar_t cls[128]={},text[2048]={};GetClassNameW(child,cls,128);GetWindowTextW(child,text,2048);
    std::wstring kind(cls),label(text);auto style=GetWindowLongW(child,GWL_STYLE);
    if(kind==L"ComboBox"){int i=(int)SendMessageW(child,CB_GETCURSEL,0,0);if(i>=0){SendMessageW(child,CB_GETLBTEXT,i,(LPARAM)text);label=text;}}
    if(kind==L"ListBox"){int n=(int)SendMessageW(child,LB_GETCOUNT,0,0);label.clear();for(int i=0;i<n;++i){SendMessageW(child,LB_GETTEXT,i,(LPARAM)text);label+=std::wstring(text)+L"\n";}}
    HFONT font=(HFONT)SendMessageW(child,WM_GETFONT,0,0);auto previousFont=SelectObject(mem,font?font:uiFont);SetBkMode(mem,TRANSPARENT);SetTextColor(mem,w==mainWindow?RGB(39,77,83):GetSysColor(COLOR_WINDOWTEXT));
    if(w!=mainWindow&&kind==L"ListBox"){
      int state=SaveDC(mem);SetViewportOrgEx(mem,box.left,box.top,nullptr);IntersectClipRect(mem,0,0,box.right-box.left,box.bottom-box.top);
      RECT client={0,0,box.right-box.left,box.bottom-box.top};FillRect(mem,&client,cardBackground);
      int top=(int)SendMessageW(child,LB_GETTOPINDEX,0,0),count=(int)SendMessageW(child,LB_GETCOUNT,0,0);
      for(int i=top;i<count;++i){
        DRAWITEMSTRUCT item={};item.CtlType=ODT_LISTBOX;item.hwndItem=child;item.hDC=mem;item.itemID=i;
        SendMessageW(child,LB_GETITEMRECT,i,(LPARAM)&item.rcItem);
        if(item.rcItem.top>=box.bottom-box.top)break;
        if(i==SendMessageW(child,LB_GETCURSEL,0,0))item.itemState=ODS_SELECTED|ODS_FOCUS;
        drawPhraseRow(item);
      }
      RestoreDC(mem,state);
    }else if(w!=mainWindow&&kind==L"Edit"){
      FillRect(mem,&box,cardBackground);SetTextColor(mem,GetSysColor(COLOR_WINDOWTEXT));DrawTextW(mem,label.c_str(),(int)label.size(),&box,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    }else if(kind==L"ChaoxiPinyin.Preview"){
      Response sample;sample.font=12;sample.count=9;copyText(sample.preedit,L"qi shang ba xia");const wchar_t* words[]={L"七上八下",L"七上",L"其",L"起",L"期",L"气",L"器",L"七",L"齐"};for(int i=0;i<9;++i)copyText(sample.candidates[i],words[i]);
      int state=SaveDC(mem);SetViewportOrgEx(mem,box.left,box.top,nullptr);CandidatePanel panel;panel.layout(mem,sample,dpi,box.right-box.left,box.bottom-box.top);panel.paint(mem,sample);RestoreDC(mem,state);
    }else{
      bool check=kind==L"Button"&&(style&BS_TYPEMASK)==BS_AUTOCHECKBOX;
      if(kind==L"Button"&&(style&BS_TYPEMASK)==BS_OWNERDRAW){
        int saved=SaveDC(mem);SetViewportOrgEx(mem,box.left,box.top,nullptr);
        DRAWITEMSTRUCT item={};item.CtlID=GetDlgCtrlID(child);item.hwndItem=child;item.hDC=mem;item.rcItem={0,0,box.right-box.left,box.bottom-box.top};
        if(item.CtlID==static_cast<UINT>(300+selectedPage))item.itemState=ODS_FOCUS;
        drawButton(item);RestoreDC(mem,saved);SelectObject(mem,previousFont);continue;
      }
      if(kind==L"Button"){RECT button=box;if(check){button.right=button.left+px(15);button.bottom=button.top+px(15);}DrawFrameControl(mem,&button,DFC_BUTTON,check?DFCS_BUTTONCHECK|((SendMessageW(child,BM_GETCHECK,0,0)==BST_CHECKED)?DFCS_CHECKED:0):DFCS_BUTTONPUSH);if(check)box.left+=px(20);}
      if(kind==L"Edit"||kind==L"ComboBox"||kind==L"ListBox"||kind==L"COMBOBOX"||kind==L"LISTBOX"){FillRect(mem,&box,(HBRUSH)GetStockObject(WHITE_BRUSH));FrameRect(mem,&box,(HBRUSH)GetStockObject(GRAY_BRUSH));box.left+=px(5);box.top+=px(3);}
      DrawTextW(mem,label.c_str(),(int)label.size(),&box,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
    }SelectObject(mem,previousFont);
  }
  BITMAPFILEHEADER file={0x4d42,(DWORD)(sizeof(file)+sizeof(info.bmiHeader)+rect.right*rect.bottom*4),0,0,sizeof(file)+sizeof(info.bmiHeader)};
  std::ofstream out(path,std::ios::binary);out.write((char*)&file,sizeof(file));out.write((char*)&info.bmiHeader,sizeof(info.bmiHeader));out.write((char*)bits,rect.right*rect.bottom*4);
  SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);ReleaseDC(w,dc);if(!out)throw std::runtime_error("snapshot write failed");
}
static void renderSettings(HWND w){
  Response r;load(r);controls(w,true);SetWindowTextW(statusText,L"引擎已就绪 · 设置保存在本机");
  auto folder=moduleDir().parent_path();
  SendMessageW(checks[0],BM_SETCHECK,BST_CHECKED,0);
  SetWindowTextW(wordCapacityBox,L"12345");
  for(int page=0;page<4;++page){
    SendMessageW(w,WM_COMMAND,300+page,0);
    for(int group=0;group<4;++group)for(HWND child:pages[group])
      if(bool(GetWindowLongW(child,GWL_STYLE)&WS_VISIBLE)!=(group==page))throw std::runtime_error("Category visibility mismatch");
    snapshot(w,folder/(L"settings-preview-"+std::to_wstring(page)+L".bmp"));
  }
  if(SendMessageW(checks[0],BM_GETCHECK,0,0)!=BST_CHECKED||GetDlgItemInt(w,122,nullptr,FALSE)!=12345)throw std::runtime_error("Category navigation lost unsaved settings");
  selectPage(w,0);snapshot(w,folder/L"settings-preview.bmp");
  RECT rect={0,0,px(730),px(690)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);
  auto dialog=CreateWindowW(L"ChaoxiPinyin.Phrases",L"个人词库",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,0,0,rect.right-rect.left,rect.bottom-rect.top,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  if(SendMessageW(phraseList,LB_GETITEMHEIGHT,0,0)!=px(40))throw std::runtime_error("Styled list row height is incorrect");
  if(GetWindowLongPtrW(wordEdit,GWL_EXSTYLE)&WS_EX_CLIENTEDGE)throw std::runtime_error("Styled input retains a native bevel");
  if((GetWindowLongPtrW(GetDlgItem(dialog,204),GWL_STYLE)&BS_TYPEMASK)!=BS_OWNERDRAW)throw std::runtime_error("Personal actions do not use themed buttons");
  personal={{L"chaoxipinyin",L"潮汐拼音"},{L"gerenciku",L"个人词库"}};
  SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)L"潮汐拼音    [chaoxipinyin]");
  SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)L"个人词库    [gerenciku]");
  SendMessageW(phraseList,LB_SETCURSEL,0,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(201,LBN_SELCHANGE),(LPARAM)phraseList);
  wchar_t selected[128];GetWindowTextW(wordEdit,selected,128);
  if(std::wstring(selected)!=L"潮汐拼音")throw std::runtime_error("Styled personal list selection did not populate word editor");
  snapshot(dialog,folder/L"phrases-preview.bmp");DestroyWindow(dialog);
  learnedMode=true;rect={0,0,px(730),px(610)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);
  dialog=CreateWindowW(L"ChaoxiPinyin.Phrases",L"本地短词",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,0,0,rect.right-rect.left,rect.bottom-rect.top,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  personal={{L"12",L"你好"},{L"8",L"世界"}};
  SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)L"你好    次数：12");
  SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)L"世界    次数：8");
  SendMessageW(phraseList,LB_SETCURSEL,1,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(201,LBN_SELCHANGE),(LPARAM)phraseList);
  GetWindowTextW(codeEdit,selected,128);if(std::wstring(selected)!=L"8")throw std::runtime_error("Styled learned list selection did not populate count editor");
  Response capacity;capacity.importNew=8192;capacity.importInvalid=12;updateLearnedSummary(capacity);
  wchar_t summary[256];GetWindowTextW(learnedSummary,summary,256);
  if(std::wstring(summary).find(L"8180")==std::wstring::npos)throw std::runtime_error("Remaining learned capacity is incorrect");
  snapshot(dialog,folder/L"learned-preview.bmp");DestroyWindow(dialog);
  std::cout<<"PASS: hidden settings and personal-dictionary control layouts rendered; no broker launched.\n";
}
int wmain(){SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);return settingsMain(GetModuleHandleW(nullptr),nullptr,nullptr,SW_HIDE);}
