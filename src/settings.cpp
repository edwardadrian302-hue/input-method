#include "common.hpp"
#include "candidate.hpp"
#include "profile.hpp"
#include "dictionary_download.hpp"
#include <commdlg.h>
#include <thread>
#include <memory>
static HWND checks[12]={},fontBox,limitBox,statusText,previewWindow,mainWindow,phraseWindow=nullptr,phraseList,wordEdit,codeEdit;
static HFONT uiFont,titleFont;static int dpi=96;static bool ready=false,busy=false;static std::thread worker;
static uint64_t importClient=((uint64_t)GetCurrentProcessId()<<32)^GetTickCount64();
static void chooseImport(HWND,bool);
static std::vector<std::pair<std::wstring,std::wstring>> personal;
static HBRUSH background=CreateSolidBrush(RGB(245,249,249));
static int px(int n){return MulDiv(n,dpi,96);}
static const int sizes[]={10,11,12,14,16,18,20,24};
static const int fuzzyFlags[]={FuzzyZ,FuzzyC,FuzzyS,FuzzyNL,FuzzyEn,FuzzyAn,FuzzyIn};
static int fontValue(){int i=(int)SendMessageW(fontBox,CB_GETCURSEL,0,0);return i>=0&&i<8?sizes[i]:12;}
static void controls(HWND w,bool enable){
  for(HWND c:checks)EnableWindow(c,enable);EnableWindow(fontBox,enable);EnableWindow(limitBox,enable);
  for(int id:{103,104,105,106,107})EnableWindow(GetDlgItem(w,id),enable);
  if(phraseWindow)for(int id:{201,202,203,204,205,206,207,208})EnableWindow(GetDlgItem(phraseWindow,id),enable);
}
static void load(const Response& r){
  SendMessageW(checks[0],BM_SETCHECK,r.learning?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[1],BM_SETCHECK,(r.options&OptAsciiPunct)?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[2],BM_SETCHECK,(r.options&OptCompletion)?BST_CHECKED:BST_UNCHECKED,0);
  for(int i=0;i<7;++i)SendMessageW(checks[i+3],BM_SETCHECK,(r.fuzzy&fuzzyFlags[i])?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[10],BM_SETCHECK,(r.options&OptCorrection)?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(checks[11],BM_SETCHECK,(r.options&OptPrediction)?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(limitBox,CB_SETCURSEL,r.limit==16?2:r.limit==8?1:0,0);
  int n=2;for(int i=0;i<8;++i)if(sizes[i]==r.font)n=i;SendMessageW(fontBox,CB_SETCURSEL,n,0);InvalidateRect(previewWindow,nullptr,TRUE);
}
struct Result {bool ok;Response response;Op op;std::wstring error;std::vector<std::pair<std::wstring,std::wstring>> items;};
static void apply(HWND w,Request q,bool online=false){
  busy=true;controls(w,false);SetWindowTextW(statusText,online?L"正在下载雾凇扩展词库，仅下载词库文件…":L"正在处理本地数据，请稍候…");
  worker=std::thread([w,q,online]()mutable{auto result=new Result{};result->op=q.op;
    fs::path download;
    try{
      if(online){auto data=downloadDictionary();fs::create_directories(userDir());
        download=userDir()/(L"dictionary-download-"+std::to_wstring(importClient)+L".tmp");writeProfileFile(download,data);copyText(q.text,download.wstring());}
      result->ok=exchange(q,result->response,120000);
    }catch(...){result->error=online?L"词库下载失败。请检查网络后重试；现有词库仍可正常使用。":L"词库读取失败。";}
    if(!download.empty()){std::error_code error;fs::remove(download,error);}
    if(result->ok&&(q.op==Op::PhraseList||q.op==Op::PhrasePut||q.op==Op::PhraseDelete||q.op==Op::ImportCommit)){
      for(int offset=0;;){Request list;list.op=Op::PhraseList;list.key=offset;Response r;
        if(!exchange(list,r,5000)){result->ok=false;break;}
        for(int i=0;i<r.count;++i)result->items.emplace_back(r.codes[i],r.candidates[i]);
        offset+=r.count;if(offset>=r.phraseCount||!r.count)break;
      }
    }
    if(!PostMessageW(w,WM_APP+1,0,(LPARAM)result))delete result;
  });
}
static int selectedPhrase(){return (int)SendMessageW(phraseList,LB_GETCURSEL,0,0);}
static LRESULT CALLBACK phraseProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
  if(m==WM_CREATE){
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){auto c=CreateWindowExW(cls==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(width),px(height),w,(HMENU)(INT_PTR)id,nullptr,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)uiFont,TRUE);return c;};
    add(L"STATIC",L"手动词条独立于自动学习；最多 4096 条；导入会跳过内置及已有词条。",0,20,15,590,25,0);
    phraseList=add(L"LISTBOX",L"",LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP|WS_BORDER,20,50,590,210,201);
    add(L"STATIC",L"词语",0,20,280,50,26,0);wordEdit=add(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,80,276,530,30,202);SendMessageW(wordEdit,EM_SETLIMITTEXT,32,0);
    add(L"STATIC",L"拼音",0,20,324,50,26,0);codeEdit=add(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,80,320,530,30,203);SendMessageW(codeEdit,EM_SETLIMITTEXT,96,0);
    add(L"STATIC",L"例如：潮汐拼音 / chao xi pin yin；ü 请填 v。输入完整拼音选词。",0,20,365,590,25,0);
    add(L"BUTTON",L"新增词条",WS_TABSTOP,20,410,130,35,204);
    add(L"BUTTON",L"修改选中词条",WS_TABSTOP,170,410,160,35,205);
    add(L"BUTTON",L"删除选中词条",WS_TABSTOP,350,410,160,35,206);
    add(L"BUTTON",L"导入本地词库…",WS_TABSTOP,20,460,260,35,207);
    add(L"BUTTON",L"检查新词更新…",WS_TABSTOP,310,460,300,35,208);
    add(L"STATIC",L"支持 UTF-8 的 Rime / TXT / TSV；新词来源：雾凇拼音（GPL-3.0）。",0,20,510,590,25,0);return 0;
  }
  if(m==WM_COMMAND&&!busy){int id=LOWORD(wp);int index=selectedPhrase();
    if(id==207||id==208){chooseImport(w,id==208);return 0;}
    if(id==201&&HIWORD(wp)==LBN_SELCHANGE&&index>=0&&index<(int)personal.size()){SetWindowTextW(codeEdit,personal[index].first.c_str());SetWindowTextW(wordEdit,personal[index].second.c_str());}
    if(id==204||id==205||id==206){
      if(id!=204&&(index<0||index>=(int)personal.size()))return 0;
      Request q;q.op=id==206?Op::PhraseDelete:Op::PhrasePut;
      GetWindowTextW(wordEdit,q.text,1024);GetWindowTextW(codeEdit,q.code,128);
      if(id==206){copyText(q.code,personal[index].first);copyText(q.text,personal[index].second);}
      if(id==205){copyText(q.previousCode,personal[index].first);copyText(q.previousWord,personal[index].second);}
      try{if(!chineseWord(q.text))throw std::runtime_error("word");phraseCode(q.code);}
      catch(...){MessageBoxW(w,L"词语需为 1–32 个汉字；拼音只接受英文字母、空格和分隔撇号。",L"请检查词条",MB_OK|MB_ICONINFORMATION);return 0;}
      if(id==206&&MessageBoxW(w,L"删除选中的手动词条？",L"个人词库",MB_YESNO|MB_DEFBUTTON2)!=IDYES)return 0;
      apply(mainWindow,q);
    }return 0;
  }
  if(m==WM_CLOSE){if(busy){MessageBeep(MB_OK);return 0;}EnableWindow(mainWindow,TRUE);DestroyWindow(w);phraseWindow=nullptr;SetActiveWindow(mainWindow);return 0;}
  return DefWindowProcW(w,m,wp,lp);
}
static void chooseImport(HWND w,bool online){
  Request q;q.op=Op::ImportPreview;q.client=importClient;
  if(online){
    if(MessageBoxW(w,L"从雾凇拼音（iDvel/rime-ice）下载最新扩展词库，筛选本机尚未收录的新词，再确认导入。\n\n来源：GitHub / raw.githubusercontent.com\n许可：GPL-3.0，随附许可证见安装目录 licenses。\n这不是实时热搜榜。仅下载文件，不上传输入内容。继续？",L"检查新词更新",MB_YESNO|MB_DEFBUTTON2|MB_ICONINFORMATION)!=IDYES)return;
  }else{
    wchar_t path[1024]={};OPENFILENAMEW dialog={sizeof(dialog)};dialog.hwndOwner=w;dialog.lpstrFile=path;dialog.nMaxFile=1024;
    dialog.lpstrFilter=L"UTF-8 词库 (*.yaml;*.txt;*.tsv)\0*.yaml;*.txt;*.tsv\0所有文件\0*.*\0\0";dialog.Flags=OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(!GetOpenFileNameW(&dialog))return;copyText(q.text,path);
  }apply(mainWindow,q,online);
}
static void chooseBackup(HWND w,bool restore){
  wchar_t path[1024]=L"ChaoxiPinyin-backup.cxbackup";OPENFILENAMEW dialog={sizeof(dialog)};dialog.hwndOwner=w;dialog.lpstrFile=path;dialog.nMaxFile=1024;
  dialog.lpstrFilter=L"潮汐拼音备份 (*.cxbackup)\0*.cxbackup\0\0";dialog.lpstrDefExt=L"cxbackup";
  dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(restore?OFN_FILEMUSTEXIST:OFN_OVERWRITEPROMPT);
  if(!(restore?GetOpenFileNameW(&dialog):GetSaveFileNameW(&dialog)))return;
  if(restore&&MessageBoxW(w,L"恢复将替换当前设置、学习词频和手动词条。建议先导出当前备份。继续恢复？",L"恢复本地备份",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)!=IDYES)return;
  Request q;q.op=restore?Op::Restore:Op::Backup;copyText(q.text,path);apply(w,q);
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
    add(L"STATIC",L"全拼输入  /  让文字自然流动",0,28,62,440,24,0);
    checks[0]=add(L"BUTTON",L"开启本地短词学习",BS_AUTOCHECKBOX|WS_TABSTOP,28,108,420,26,110);
    add(L"STATIC",L"只保存汉字词条和次数。关闭后保留、不使用、不更新。\n清除只影响自动学习；手动词条保留。",0,28,166,440,40,0);
    add(L"STATIC",L"自动学习长度",0,28,140,130,24,0);
    limitBox=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,160,134,170,150,108);
    for(auto text:{L"1–4 字（默认）",L"1–8 字",L"1–16 字"})SendMessageW(limitBox,CB_ADDSTRING,0,(LPARAM)text);
    add(L"STATIC",L"候选字号",0,28,207,90,28,0);
    fontBox=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,124,202,100,240,102);
    for(int size:sizes)SendMessageW(fontBox,CB_ADDSTRING,0,(LPARAM)std::to_wstring(size).c_str());SendMessageW(fontBox,CB_SETCURSEL,2,0);
    add(L"STATIC",L"磅 · 窗口随内容与屏幕缩放",0,236,207,240,28,0);
    checks[1]=add(L"BUTTON",L"中文模式使用英文标点 , . ; :",BS_AUTOCHECKBOX|WS_TABSTOP,28,255,440,26,111);
    checks[2]=add(L"BUTTON",L"拼音补全、简拼与英文补全（buil → build）",BS_AUTOCHECKBOX|WS_TABSTOP,28,298,450,26,112);
    add(L"STATIC",L"模糊音  ·  按需开启，默认关闭",0,28,348,440,24,0);
    const wchar_t* names[]={L"z ↔ zh",L"c ↔ ch",L"s ↔ sh",L"n ↔ l",L"en ↔ eng",L"an ↔ ang",L"in ↔ ing"};
    for(int i=0;i<7;++i)checks[i+3]=add(L"BUTTON",names[i],BS_AUTOCHECKBOX|WS_TABSTOP,28+(i%3)*145,383+(i/3)*34,140,26,113+i);
    checks[10]=add(L"BUTTON",L"拼音误按纠错（候选建议，不自动改字）",BS_AUTOCHECKBOX|WS_TABSTOP,28,491,460,26,120);
    checks[11]=add(L"BUTTON",L"上屏联想（本地词库续词）",BS_AUTOCHECKBOX|WS_TABSTOP,28,521,460,26,121);
    add(L"STATIC",L"单按 Shift 或点击系统输入法的“中 / 英”按钮切换。\n英文模式直接输入英文标点。拼写设置应用时会重建词库索引。",0,28,548,460,42,0);
    add(L"BUTTON",L"应用设置",WS_TABSTOP|BS_DEFPUSHBUTTON,28,603,128,36,103);
    add(L"BUTTON",L"清除学习数据",WS_TABSTOP,172,603,160,36,104);
    add(L"STATIC",L"候选外观预览",0,520,108,300,24,0);
    previewWindow=add(L"ChaoxiPinyin.Preview",L"",0,520,146,310,264,0);
    add(L"BUTTON",L"管理个人词库…",WS_TABSTOP,520,435,310,36,105);
    add(L"BUTTON",L"导出设置与词库备份…",WS_TABSTOP,520,483,310,36,106);
    add(L"BUTTON",L"恢复备份…",WS_TABSTOP,520,531,310,36,107);
    add(L"STATIC",L"备份为明文，包含手动词条与学习词频。\n请妥善保存；词库更新由你主动联网下载。",0,520,591,310,48,0);
    statusText=add(L"STATIC",L"正在准备本地引擎，首次启动可能需要稍候…",0,28,659,800,26,0);
    controls(w,false);
#ifndef CHAOXI_SETTINGS_PREVIEW
    launchBroker(moduleDir());SetTimer(w,1,1000,nullptr);
#endif
    return 0;
  }
  if(m==WM_TIMER&&!busy){Request q;Response r;if(exchange(q,r,100)){if(!ready)load(r);ready=true;controls(w,true);SetWindowTextW(statusText,r.storageError?L"输入正常，但本地词频保存失败，请检查磁盘权限。":L"引擎已就绪 · 新词可手动更新");KillTimer(w,1);}return 0;}
  if(m==WM_APP+1){if(worker.joinable())worker.join();std::unique_ptr<Result> result((Result*)lp);busy=false;controls(w,true);
    if(result->ok&&result->op==Op::ImportPreview){
      auto& r=result->response;bool fits=r.phraseCount+r.importNew<=4096;
      std::wstring summary=L"可新增："+std::to_wstring(r.importNew)+L" 条\n重复或内置："+std::to_wstring(r.importDuplicate)+L" 条\n无效 / 不支持："+std::to_wstring(r.importInvalid)+L" 行\n当前个人词条："+std::to_wstring(r.phraseCount)+L" / 4096\n\n";
      summary+=!r.importNew?L"没有可新增词条；当前文件中的有效词条已收录。":fits?L"使用文件预览时的快照，已有词条保留。是否导入？":L"超出 4096 条容量，本次不会导入。请先拆分词库或删除不需要的个人词条。";
      int answer=MessageBoxW(phraseWindow,summary.c_str(),L"确认批量导入",fits&&r.importNew?MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION:MB_OK|MB_ICONINFORMATION);
      Request next;next.client=importClient;next.op=fits&&r.importNew&&answer==IDYES?Op::ImportCommit:Op::ImportCancel;apply(w,next);return 0;
    }
    if(result->ok){if(result->op==Op::Configure||result->op==Op::Restore)load(result->response);
      if(result->op==Op::PhraseList||result->op==Op::PhrasePut||result->op==Op::PhraseDelete||result->op==Op::ImportCommit){personal=std::move(result->items);if(phraseWindow){SendMessageW(phraseList,LB_RESETCONTENT,0,0);for(auto& item:personal){auto text=item.second+L"    ["+item.first+L"]";SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)text.c_str());}}}
      SetWindowTextW(statusText,result->op==Op::ImportCancel?L"未导入新词 · 原有数据保留":result->op==Op::ImportCommit?L"词库已批量导入 · 新词已生效":result->op==Op::Backup?L"备份已导出 · 包含设置、学习词频和手动词条":result->op==Op::Restore?L"备份已恢复 · 下次输入生效":L"操作完成 · 本地数据已保存");}
    else{SetWindowTextW(statusText,L"操作未完成，原有数据保留。");MessageBoxW(phraseWindow?phraseWindow:w,result->error.empty()?L"操作未完成。词库需为 UTF-8，使用默认 Rime 列顺序或“词语<TAB>拼音”格式；不支持搜狗 SCEL。也请检查文件权限、4096 条容量或重试。":result->error.c_str(),L"潮汐拼音",MB_OK|MB_ICONWARNING);}return 0;}
  if(m==WM_COMMAND){
    if(LOWORD(wp)==102&&HIWORD(wp)==CBN_SELCHANGE)InvalidateRect(previewWindow,nullptr,TRUE);
    if(LOWORD(wp)==103&&!busy){Request q;q.op=Op::Configure;q.key=fontValue();q.value=0;q.mask=0;
      int selection=(int)SendMessageW(limitBox,CB_GETCURSEL,0,0);q.limit=selection==2?16:selection==1?8:4;
      if(SendMessageW(checks[0],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptLearning;
      if(SendMessageW(checks[1],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptAsciiPunct;
      if(SendMessageW(checks[2],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptCompletion;
      if(SendMessageW(checks[10],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptCorrection;
      if(SendMessageW(checks[11],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptPrediction;
      for(int i=0;i<7;++i)if(SendMessageW(checks[i+3],BM_GETCHECK,0,0)==BST_CHECKED)q.mask|=fuzzyFlags[i];apply(w,q);
    }
    if(LOWORD(wp)==105&&!busy){
      if(!phraseWindow){RECT rect={0,0,px(630),px(555)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);phraseWindow=CreateWindowW(L"ChaoxiPinyin.Phrases",L"个人词库",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,w,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(phraseWindow,SW_SHOW);EnableWindow(w,FALSE);}
      Request q;q.op=Op::PhraseList;apply(w,q);
    }
    if(LOWORD(wp)==106&&!busy)chooseBackup(w,false);
    if(LOWORD(wp)==107&&!busy)chooseBackup(w,true);
    if(LOWORD(wp)==104&&!busy&&MessageBoxW(w,L"删除全部自动学习的词条及次数？手动词条、内置词库和设置保留。",L"清除学习数据",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)==IDYES){Request q;q.op=Op::Erase;apply(w,q);}return 0;
  }
  if(m==WM_CTLCOLORSTATIC||m==WM_CTLCOLORBTN){auto dc=(HDC)wp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(39,77,83));SetBkColor(dc,RGB(245,249,249));return (LRESULT)background;}
  if(m==WM_CLOSE&&busy){MessageBeep(MB_OK);return 0;}
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
  SetProcessDPIAware();HDC dc=GetDC(nullptr);dpi=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(nullptr,dc);
  RECT work={};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
  int fit=std::min(MulDiv(work.right-work.left-40,96,860),MulDiv(work.bottom-work.top-70,96,700));
  dpi=std::max(72,std::min(dpi,fit));
  uiFont=CreateFontW(-px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  titleFont=CreateFontW(-px(27),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  WNDCLASSW wc={};wc.hInstance=h;wc.lpfnWndProc=preview;wc.lpszClassName=L"ChaoxiPinyin.Preview";wc.hbrBackground=background;RegisterClassW(&wc);
  wc.hIcon=LoadIconW(h,MAKEINTRESOURCEW(1));wc.lpfnWndProc=proc;wc.lpszClassName=L"ChaoxiPinyin.Settings";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
  wc.lpfnWndProc=phraseProc;wc.lpszClassName=L"ChaoxiPinyin.Phrases";RegisterClassW(&wc);
  RECT rect={0,0,px(860),px(700)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
  HWND w=CreateWindowW(L"ChaoxiPinyin.Settings",L"潮汐拼音设置",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,h,nullptr);mainWindow=w;
#ifdef CHAOXI_SETTINGS_PREVIEW
  renderSettings(w);DestroyWindow(w);DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);return 0;
#else
  ShowWindow(w,show);
#endif
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!(phraseWindow&&IsDialogMessageW(phraseWindow,&msg))&&!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
  if(worker.joinable())worker.join();DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);return 0;
}
