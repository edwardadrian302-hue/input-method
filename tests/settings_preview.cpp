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
  FillRect(mem,&rect,background);
  for(HWND child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
    RECT box;GetWindowRect(child,&box);MapWindowPoints(nullptr,w,(POINT*)&box,2);
    if(box.left<0||box.top<0||box.right>rect.right||box.bottom>rect.bottom)throw std::runtime_error("Control extends outside client area");
    wchar_t cls[128]={},text[2048]={};GetClassNameW(child,cls,128);GetWindowTextW(child,text,2048);
    std::wstring kind(cls),label(text);auto style=GetWindowLongW(child,GWL_STYLE);
    if(kind==L"ComboBox"){int i=(int)SendMessageW(child,CB_GETCURSEL,0,0);if(i>=0){SendMessageW(child,CB_GETLBTEXT,i,(LPARAM)text);label=text;}}
    if(kind==L"ListBox"){int n=(int)SendMessageW(child,LB_GETCOUNT,0,0);label.clear();for(int i=0;i<n;++i){SendMessageW(child,LB_GETTEXT,i,(LPARAM)text);label+=std::wstring(text)+L"\n";}}
    HFONT font=(HFONT)SendMessageW(child,WM_GETFONT,0,0);auto previousFont=SelectObject(mem,font?font:uiFont);SetBkMode(mem,TRANSPARENT);SetTextColor(mem,RGB(39,77,83));
    if(kind==L"ChaoxiPinyin.Preview"){
      Response sample;sample.font=12;sample.count=9;copyText(sample.preedit,L"qi shang ba xia");const wchar_t* words[]={L"七上八下",L"七上",L"其",L"起",L"期",L"气",L"器",L"七",L"齐"};for(int i=0;i<9;++i)copyText(sample.candidates[i],words[i]);
      int state=SaveDC(mem);SetViewportOrgEx(mem,box.left,box.top,nullptr);CandidatePanel panel;panel.layout(mem,sample,dpi,box.right-box.left,box.bottom-box.top);panel.paint(mem,sample);RestoreDC(mem,state);
    }else{
      bool check=kind==L"Button"&&(style&BS_TYPEMASK)==BS_AUTOCHECKBOX;
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
  auto folder=moduleDir().parent_path();snapshot(w,folder/L"settings-preview.bmp");
  RECT rect={0,0,px(630),px(555)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);
  auto dialog=CreateWindowW(L"ChaoxiPinyin.Phrases",L"个人词库",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,0,0,rect.right-rect.left,rect.bottom-rect.top,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)L"潮汐拼音    [chaoxipinyin]");SetWindowTextW(wordEdit,L"潮汐拼音");SetWindowTextW(codeEdit,L"chao xi pin yin");snapshot(dialog,folder/L"phrases-preview.bmp");DestroyWindow(dialog);
  std::cout<<"PASS: hidden settings and personal-dictionary control layouts rendered; no broker launched.\n";
}
int wmain(){SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);return settingsMain(GetModuleHandleW(nullptr),nullptr,nullptr,SW_HIDE);}
