#include "common.hpp"
#include "candidate.hpp"
#include "profile.hpp"
#include "dictionary_download.hpp"
#include <commdlg.h>
#include <commctrl.h>
#include <thread>
#include <memory>
static HWND checks[12]={},fontBox,limitBox,wordCapacityBox,phraseCapacityBox,statusText,previewWindow,mainWindow,phraseWindow=nullptr,phraseList,wordEdit,codeEdit,learnedSummary;
static HFONT uiFont,titleFont;static int dpi=96;static bool ready=false,busy=false;static std::thread worker;
static bool learnedMode=false;
static uint64_t importClient=((uint64_t)GetCurrentProcessId()<<32)^GetTickCount64();
static void chooseImport(HWND,bool);
static std::vector<std::pair<std::wstring,std::wstring>> personal;
static HBRUSH background=CreateSolidBrush(RGB(245,247,245));
static HBRUSH cardBackground=CreateSolidBrush(RGB(255,255,255));
static int px(int n){return MulDiv(n,dpi,96);}
static int selectedPage=0;
static std::vector<HWND> pages[4];
static const wchar_t* pageNames[]={L"常用设置",L"输入辅助",L"候选外观",L"词库与数据"};
static void selectPage(HWND w,int page){
  selectedPage=page;
  for(int i=0;i<4;++i)for(HWND c:pages[i])ShowWindow(c,i==page?SW_SHOW:SW_HIDE);
  for(int i=0;i<4;++i)InvalidateRect(GetDlgItem(w,300+i),nullptr,TRUE);
  InvalidateRect(w,nullptr,TRUE);
}
static void paintSettings(HDC dc){
  RECT all={0,0,px(960),px(700)};FillRect(dc,&all,background);
  RECT sidebar={0,0,px(210),px(700)};FillRect(dc,&sidebar,cardBackground);
  auto pen=CreatePen(PS_SOLID,1,RGB(226,232,226));auto oldPen=SelectObject(dc,pen);
  auto oldBrush=SelectObject(dc,cardBackground);
  const int bottoms[]={470,490,500,560};
  RoundRect(dc,px(238),px(88),px(932),px(bottoms[selectedPage]),px(12),px(12));
  SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(39,77,83));
  auto oldFont=SelectObject(dc,titleFont);RECT title={px(246),px(26),px(920),px(67)};
  DrawTextW(dc,pageNames[selectedPage],-1,&title,DT_LEFT|DT_SINGLELINE|DT_VCENTER);SelectObject(dc,oldFont);
}
static void drawButton(const DRAWITEMSTRUCT& item){
  HDC dc=item.hDC;RECT rect=item.rcItem;int id=(int)item.CtlID;
  bool nav=id>=300&&id<304,active=nav&&id-300==selectedPage;
  COLORREF fill=active||id==103||id==204||(learnedMode&&id==205)?RGB(206,235,211):nav?RGB(255,255,255):RGB(243,247,243);
  if(item.itemState&ODS_SELECTED)fill=RGB(180,220,188);
  auto brush=CreateSolidBrush(fill);auto pen=CreatePen(PS_SOLID,1,nav?fill:RGB(217,228,218));
  auto oldBrush=SelectObject(dc,brush);auto oldPen=SelectObject(dc,pen);
  RoundRect(dc,rect.left,rect.top,rect.right,rect.bottom,px(8),px(8));
  SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);
  wchar_t text[256];GetWindowTextW(item.hwndItem,text,256);SetBkMode(dc,TRANSPARENT);
  SetTextColor(dc,item.itemState&ODS_DISABLED?GetSysColor(COLOR_GRAYTEXT):(GetParent(item.hwndItem)==mainWindow?RGB(39,77,83):GetSysColor(COLOR_BTNTEXT)));
  auto oldFont=SelectObject(dc,uiFont);if(nav)rect.left+=px(18);
  DrawTextW(dc,text,-1,&rect,DT_SINGLELINE|DT_VCENTER|(nav?DT_LEFT:DT_CENTER));SelectObject(dc,oldFont);
  if((item.itemState&ODS_FOCUS)&&!(item.itemState&ODS_NOFOCUSRECT)){
    RECT focus=item.rcItem;InflateRect(&focus,-px(3),-px(3));
    auto focusPen=CreatePen(PS_SOLID,px(1),RGB(116,158,124));auto previousPen=SelectObject(dc,focusPen);
    auto previousBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
    RoundRect(dc,focus.left,focus.top,focus.right,focus.bottom,px(6),px(6));
    SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(focusPen);
  }
}

static void paintPhraseWindow(HDC dc){
  RECT all={0,0,px(730),px(690)};FillRect(dc,&all,background);
  auto pen=CreatePen(PS_SOLID,1,RGB(226,232,226));auto previousPen=SelectObject(dc,pen);
  auto previousBrush=SelectObject(dc,cardBackground);
  RoundRect(dc,px(24),px(learnedMode?98:76),px(706),px(330),px(12),px(12));
  RoundRect(dc,px(24),px(346),px(706),px(526),px(12),px(12));
  for(HWND field:{wordEdit,codeEdit}){
    if(!field)continue;
    RECT box;GetWindowRect(field,&box);MapWindowPoints(nullptr,GetParent(field),(POINT*)&box,2);
    InflateRect(&box,px(10),px(8));
    auto fieldPen=CreatePen(PS_SOLID,1,GetFocus()==field?RGB(116,158,124):RGB(217,228,218));
    auto old=SelectObject(dc,fieldPen);
    RoundRect(dc,box.left,box.top,box.right,box.bottom,px(8),px(8));
    SelectObject(dc,old);DeleteObject(fieldPen);
  }
  SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(pen);
}
static void drawPhraseRow(const DRAWITEMSTRUCT& item){
  auto dc=item.hDC;RECT row=item.rcItem;FillRect(dc,&row,cardBackground);
  if(item.itemID==static_cast<UINT>(-1))return;
  bool selected=(item.itemState&ODS_SELECTED)!=0;
  RECT inset=row;InflateRect(&inset,-px(2),-px(3));
  auto brush=CreateSolidBrush(selected?RGB(206,235,211):RGB(255,255,255));
  auto pen=CreatePen(PS_SOLID,1,selected?RGB(206,235,211):RGB(255,255,255));
  auto previousBrush=SelectObject(dc,brush);auto previousPen=SelectObject(dc,pen);
  RoundRect(dc,inset.left,inset.top,inset.right,inset.bottom,px(8),px(8));
  SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(brush);DeleteObject(pen);
  int length=(int)SendMessageW(item.hwndItem,LB_GETTEXTLEN,item.itemID,0);
  if(length==LB_ERR)return;
  std::vector<wchar_t> text(length+1);SendMessageW(item.hwndItem,LB_GETTEXT,item.itemID,(LPARAM)text.data());
  auto font=SelectObject(dc,uiFont);SetBkMode(dc,TRANSPARENT);
  SetTextColor(dc,item.itemState&ODS_DISABLED?GetSysColor(COLOR_GRAYTEXT):GetSysColor(COLOR_WINDOWTEXT));
  row.left+=px(14);row.right-=px(14);DrawTextW(dc,text.data(),length,&row,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
  SelectObject(dc,font);
  if((item.itemState&ODS_FOCUS)&&!(item.itemState&ODS_NOFOCUSRECT)){
    auto focus=CreatePen(PS_SOLID,1,RGB(116,158,124));auto oldPen=SelectObject(dc,focus);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
    RoundRect(dc,inset.left,inset.top,inset.right,inset.bottom,px(8),px(8));
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(focus);
  }
}

static const int sizes[]={10,11,12,14,16,18,20,24};
static const int fuzzyFlags[]={FuzzyZ,FuzzyC,FuzzyS,FuzzyNL,FuzzyEn,FuzzyAn,FuzzyIn};
static int fontValue(){int i=(int)SendMessageW(fontBox,CB_GETCURSEL,0,0);return i>=0&&i<8?sizes[i]:12;}
static void controls(HWND w,bool enable){
  for(HWND c:checks)EnableWindow(c,enable);EnableWindow(fontBox,enable);EnableWindow(limitBox,enable);EnableWindow(wordCapacityBox,enable);EnableWindow(phraseCapacityBox,enable);
  for(int id:{103,104,105,106,107,124})EnableWindow(GetDlgItem(w,id),enable);
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
struct Result {bool ok,committed=false;Response response,capacity;Op op;std::wstring error;std::vector<std::pair<std::wstring,std::wstring>> items;};
static void apply(HWND w,Request q,bool online=false){
  busy=true;controls(w,false);SetWindowTextW(statusText,online?L"正在下载雾凇扩展词库，仅下载词库文件…":L"正在处理本地数据，请稍候…");
  worker=std::thread([w,q,online]()mutable{auto result=new Result{};result->op=q.op;
    fs::path download;
    try{
      if(online){auto data=downloadDictionary();fs::create_directories(userDir());
        download=userDir()/(L"dictionary-download-"+std::to_wstring(importClient)+L".tmp");writeProfileFile(download,data);copyText(q.text,download.wstring());}
      result->ok=exchange(q,result->response,120000);
      if(q.op==Op::LearnedList||q.op==Op::LearnedPut||q.op==Op::LearnedDelete){
        if(result->ok&&result->response.importNew!=1){result->ok=false;result->error=L"当前引擎尚不支持短词管理，请升级后注销重登。";}
        if(result->ok){
          result->committed=q.op==Op::LearnedPut||q.op==Op::LearnedDelete;
          Request list;list.op=Op::LearnedList;
          for(;;){Response r;if(!exchange(list,r,5000)||r.importNew!=1){result->ok=false;break;}
            for(int i=0;i<r.count;++i)result->items.emplace_back(r.codes[i],r.candidates[i]);
            if(r.last)break;
            if(!r.count){result->ok=false;break;}copyText(list.previousWord,r.candidates[r.count-1]);
          }
        }else if(result->error.empty())result->error=L"修改未保存。记录可能已发生变化，或新词语与已有记录重复。请关闭后重新打开列表再试。";
      }
    }catch(...){result->error=online?L"词库下载失败。请检查网络后重试；现有词库仍可正常使用。":L"词库读取失败。";}
    if(!download.empty()){std::error_code error;fs::remove(download,error);}
    if(result->ok&&(q.op==Op::PhraseList||q.op==Op::PhrasePut||q.op==Op::PhraseDelete||q.op==Op::ImportCommit)){
      for(int offset=0;;){Request list;list.op=Op::PhraseList;list.key=offset;Response r;
        if(!exchange(list,r,5000)){result->ok=false;break;}
        for(int i=0;i<r.count;++i)result->items.emplace_back(r.codes[i],r.candidates[i]);
        offset+=r.count;if(offset>=r.phraseCount||!r.count)break;
      }
    }
    if(result->ok){Request capacity;capacity.op=Op::Capacity;
      result->ok=exchange(capacity,result->capacity,5000)&&result->capacity.importNew>=1&&result->capacity.importDuplicate>=1;}
    if(!PostMessageW(w,WM_APP+1,0,(LPARAM)result))delete result;
  });
}
static void updateLearnedSummary(const Response& capacity){
  if(!learnedSummary)return;
  int used=capacity.importInvalid,total=capacity.importNew;
  auto summary=L"已保存 "+std::to_wstring(used)+L" 条    总容量 "+std::to_wstring(total)+L" 条    剩余 "+std::to_wstring(std::max(0,total-used))+L" 条";
  SetWindowTextW(learnedSummary,summary.c_str());
}
static int selectedPhrase(){return (int)SendMessageW(phraseList,LB_GETCURSEL,0,0);}
static LRESULT CALLBACK phraseProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
  if(m==WM_CREATE){
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){
      std::wstring kind(cls);
      if(kind==L"BUTTON")style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
      if(kind==L"EDIT"){x+=10;y+=8;width-=20;height-=16;}
      auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(width),px(height),w,(HMENU)(INT_PTR)id,nullptr,nullptr);
      SendMessageW(c,WM_SETFONT,(WPARAM)uiFont,TRUE);return c;
    };
    add(L"STATIC",learnedMode?L"查看、修改自动学习的词语与次数；修改后点击保存。":L"手动词条独立于自动学习；容量可在设置中调整；导入跳过已有词条。",0,32,24,666,42,0);
    learnedSummary=nullptr;
    if(learnedMode)learnedSummary=add(L"STATIC",L"正在读取词库用量…",0,32,66,666,26,209);
    phraseList=add(L"LISTBOX",L"",LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|WS_VSCROLL|WS_TABSTOP,36,learnedMode?110:88,658,learnedMode?208:230,201);
    add(L"STATIC",L"词语",0,44,370,60,26,0);wordEdit=add(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,116,362,566,40,202);SendMessageW(wordEdit,EM_SETLIMITTEXT,32,0);
    add(L"STATIC",learnedMode?L"次数":L"拼音",0,44,422,60,26,0);codeEdit=add(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,116,414,566,40,203);SendMessageW(codeEdit,EM_SETLIMITTEXT,96,0);
    add(L"STATIC",learnedMode?L"词语为 1–16 个汉字，次数为 1–1000000。修改文字只影响学习排序。":L"例如：潮汐拼音 / chao xi pin yin；ü 请填 v。输入完整拼音选词。",0,44,474,638,42,0);
    add(L"BUTTON",L"新增词条",WS_TABSTOP,24,546,218,40,204);
    add(L"BUTTON",learnedMode?L"保存选中短词":L"修改选中词条",WS_TABSTOP,learnedMode?24:256,546,learnedMode?330:218,40,205);
    add(L"BUTTON",L"删除选中词条",WS_TABSTOP,learnedMode?376:488,546,learnedMode?330:218,40,206);
    add(L"BUTTON",L"导入本地词库…",WS_TABSTOP,24,598,334,40,207);
    add(L"BUTTON",L"检查新词更新…",WS_TABSTOP,372,598,334,40,208);
    add(L"STATIC",L"支持 UTF-8 的 Rime / TXT / TSV；新词来源：雾凇拼音（GPL-3.0）。",0,32,650,666,28,210);
    if(learnedMode){for(int id:{204,207,208})ShowWindow(GetDlgItem(w,id),SW_HIDE);
      ShowWindow(GetDlgItem(w,210),SW_HIDE);
      SendMessageW(wordEdit,EM_SETLIMITTEXT,16,0);SendMessageW(codeEdit,EM_SETLIMITTEXT,7,0);
    }return 0;
  }
  if(m==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);paintPhraseWindow(dc);EndPaint(w,&ps);return 0;}
  if(m==WM_MEASUREITEM){auto item=(MEASUREITEMSTRUCT*)lp;if(item->CtlType==ODT_LISTBOX){item->itemHeight=px(40);return TRUE;}}
  if(m==WM_DRAWITEM){auto& item=*(DRAWITEMSTRUCT*)lp;if(item.CtlType==ODT_LISTBOX)drawPhraseRow(item);else drawButton(item);return TRUE;}
  if(m==WM_CTLCOLOREDIT||m==WM_CTLCOLORLISTBOX||m==WM_CTLCOLORSTATIC){
    HDC dc=(HDC)wp;RECT box;GetWindowRect((HWND)lp,&box);MapWindowPoints(nullptr,w,(POINT*)&box,2);
    bool card=m!=WM_CTLCOLORSTATIC||(box.top>=px(346)&&box.top<px(526));
    SetBkMode(dc,TRANSPARENT);SetBkColor(dc,card?RGB(255,255,255):RGB(245,247,245));
    SetTextColor(dc,IsWindowEnabled((HWND)lp)?GetSysColor(COLOR_WINDOWTEXT):GetSysColor(COLOR_GRAYTEXT));
    return (LRESULT)(card?cardBackground:background);
  }
  if(m==WM_COMMAND&&(HIWORD(wp)==EN_SETFOCUS||HIWORD(wp)==EN_KILLFOCUS))InvalidateRect(w,nullptr,FALSE);
  if(m==WM_COMMAND&&!busy){int id=LOWORD(wp);int index=selectedPhrase();
    if(id==207||id==208){chooseImport(w,id==208);return 0;}
    if(id==201&&HIWORD(wp)==LBN_SELCHANGE&&index>=0&&index<(int)personal.size()){SetWindowTextW(codeEdit,personal[index].first.c_str());SetWindowTextW(wordEdit,personal[index].second.c_str());}
    if(learnedMode&&(id==205||id==206)){
      if(index<0||index>=(int)personal.size())return 0;
      Request q;q.op=id==206?Op::LearnedDelete:Op::LearnedPut;
      copyText(q.previousWord,personal[index].second);copyText(q.code,personal[index].first);
      GetWindowTextW(wordEdit,q.text,1024);
      wchar_t count[128];GetWindowTextW(codeEdit,count,128);std::wstring number(count);
      if(id==205){
        if(number.empty()||number.size()>7||!std::all_of(number.begin(),number.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';})||!chineseWord(q.text,16)){
          MessageBoxW(w,L"词语需为 1–16 个汉字，次数需为 1–1000000 的整数。",L"请检查短词",MB_OK|MB_ICONINFORMATION);return 0;}
        q.value=std::stoi(number);if(q.value<1||q.value>1000000){MessageBoxW(w,L"次数需为 1–1000000。",L"请检查短词",MB_OK);return 0;}
      }
      if(id==206&&MessageBoxW(w,L"删除选中的自动学习短词及次数？",L"本地短词",MB_YESNO|MB_DEFBUTTON2)!=IDYES)return 0;
      apply(mainWindow,q);return 0;
    }
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
    int page=-1;
    auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){
      if(std::wstring(cls)==L"BUTTON"&&(style&BS_TYPEMASK)!=BS_AUTOCHECKBOX)style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
      auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(width),px(height),w,(HMENU)(INT_PTR)id,nullptr,nullptr);
      SendMessageW(c,WM_SETFONT,(WPARAM)uiFont,TRUE);
      if(page>=0)pages[page].push_back(c);
      return c;
    };
    auto title=add(L"STATIC",L"潮汐拼音",0,24,28,175,36,0);SendMessageW(title,WM_SETFONT,(WPARAM)titleFont,TRUE);
    add(L"STATIC",L"全拼输入  /  让文字自然流动",0,24,76,165,46,0);
    for(int i=0;i<4;++i)add(L"BUTTON",pageNames[i],WS_TABSTOP,16,146+i*56,178,44,300+i);
    page=0;
    checks[0]=add(L"BUTTON",L"开启本地短词学习",BS_AUTOCHECKBOX|WS_TABSTOP,266,112,620,28,110);
    add(L"STATIC",L"自动学习长度",0,266,164,150,28,0);
    limitBox=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,460,158,240,150,108);
    for(auto text:{L"1–4 字（默认）",L"1–8 字",L"1–16 字"})SendMessageW(limitBox,CB_ADDSTRING,0,(LPARAM)text);
    add(L"STATIC",L"只保存汉字词条和次数。关闭后保留、不使用、不更新。\n清除只影响自动学习；手动词条保留。",0,266,212,630,48,0);
    checks[1]=add(L"BUTTON",L"中文模式使用英文标点 , . ; :",BS_AUTOCHECKBOX|WS_TABSTOP,266,286,620,28,111);
    checks[2]=add(L"BUTTON",L"拼音补全、简拼与英文补全（buil → build）",BS_AUTOCHECKBOX|WS_TABSTOP,266,344,640,28,112);
    page=1;
    add(L"STATIC",L"模糊音  ·  按需开启，默认关闭",0,266,112,620,28,0);
    const wchar_t* names[]={L"z ↔ zh",L"c ↔ ch",L"s ↔ sh",L"n ↔ l",L"en ↔ eng",L"an ↔ ang",L"in ↔ ing"};
    for(int i=0;i<7;++i)checks[i+3]=add(L"BUTTON",names[i],BS_AUTOCHECKBOX|WS_TABSTOP,266+(i%3)*210,156+(i/3)*38,190,26,113+i);
    checks[10]=add(L"BUTTON",L"拼音误按纠错（候选建议，不自动改字）",BS_AUTOCHECKBOX|WS_TABSTOP,266,298,640,28,120);
    checks[11]=add(L"BUTTON",L"上屏联想（本地词库续词）",BS_AUTOCHECKBOX|WS_TABSTOP,266,348,640,28,121);
    add(L"STATIC",L"单按 Shift 或点击系统输入法的“中 / 英”按钮切换。\n英文模式直接输入英文标点。拼写设置应用时会重建词库索引。",0,266,407,630,48,0);
    page=2;
    add(L"STATIC",L"候选字号",0,266,112,100,28,0);
    fontBox=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,380,106,100,240,102);
    for(int size:sizes)SendMessageW(fontBox,CB_ADDSTRING,0,(LPARAM)std::to_wstring(size).c_str());SendMessageW(fontBox,CB_SETCURSEL,2,0);
    add(L"STATIC",L"磅 · 窗口随内容与屏幕缩放",0,500,112,390,28,0);
    add(L"STATIC",L"候选外观预览",0,266,167,300,28,0);
    previewWindow=add(L"ChaoxiPinyin.Preview",L"",0,266,208,610,264,0);
    page=3;
    add(L"STATIC",L"词库容量（每类 1–100000 条）",0,266,112,620,28,0);
    add(L"STATIC",L"自动学习上限",0,266,162,190,28,0);
    wordCapacityBox=add(L"EDIT",L"8192",WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,514,156,360,32,122);
    add(L"STATIC",L"个人词条上限",0,266,210,190,28,0);
    phraseCapacityBox=add(L"EDIT",L"4096",WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,514,204,360,32,123);
    SendMessageW(wordCapacityBox,EM_SETLIMITTEXT,6,0);SendMessageW(phraseCapacityBox,EM_SETLIMITTEXT,6,0);
    add(L"BUTTON",L"管理个人词库…",WS_TABSTOP,266,264,296,40,105);
    add(L"BUTTON",L"查看与修改本地短词…",WS_TABSTOP,578,264,296,40,124);
    add(L"BUTTON",L"清除学习数据",WS_TABSTOP,266,318,608,40,104);
    add(L"BUTTON",L"导出设置与词库备份…",WS_TABSTOP,266,372,296,40,106);
    add(L"BUTTON",L"恢复备份…",WS_TABSTOP,578,372,296,40,107);
    add(L"STATIC",L"备份为明文，包含手动词条与学习词频。\n请妥善保存；词库更新由你主动联网下载。",0,266,444,608,56,0);
    page=-1;
    add(L"BUTTON",L"应用设置",WS_TABSTOP|BS_DEFPUSHBUTTON,774,594,158,42,103);
    statusText=add(L"STATIC",L"正在准备本地引擎，首次启动可能需要稍候…",0,238,652,694,30,0);
    selectPage(w,0);
    controls(w,false);
#ifndef CHAOXI_SETTINGS_PREVIEW
    launchBroker(moduleDir());SetTimer(w,1,1000,nullptr);
#endif
    return 0;
  }
  if(m==WM_TIMER&&!busy){Request q;Response r;if(exchange(q,r,100)){
    if(!ready){Request caps;caps.op=Op::Capacity;Response capacity;
      if(!exchange(caps,capacity,100)||capacity.importNew<1||capacity.importDuplicate<1){SetWindowTextW(statusText,L"请保存工作并注销重登，以加载支持容量设置的新版引擎。");return 0;}
      load(r);SetWindowTextW(wordCapacityBox,std::to_wstring(capacity.importNew).c_str());SetWindowTextW(phraseCapacityBox,std::to_wstring(capacity.importDuplicate).c_str());}
    ready=true;controls(w,true);SetWindowTextW(statusText,r.storageError?L"输入正常，但本地词频保存失败，请检查磁盘权限。":L"引擎已就绪 · 新词可手动更新");KillTimer(w,1);}return 0;}
  if(m==WM_APP+1){if(worker.joinable())worker.join();std::unique_ptr<Result> result((Result*)lp);busy=false;controls(w,true);
    if(result->ok&&result->op==Op::ImportPreview){
      auto& r=result->response;bool fits=r.phraseCount+r.importNew<=result->capacity.importDuplicate;
      std::wstring summary=L"可新增："+std::to_wstring(r.importNew)+L" 条\n重复或内置："+std::to_wstring(r.importDuplicate)+L" 条\n无效 / 不支持："+std::to_wstring(r.importInvalid)+L" 行\n当前个人词条："+std::to_wstring(r.phraseCount)+L" / "+std::to_wstring(result->capacity.importDuplicate)+L"\n\n";
      summary+=!r.importNew?L"没有可新增词条；当前文件中的有效词条已收录。":fits?L"使用文件预览时的快照，已有词条保留。是否导入？":L"超出配置的容量，本次不会导入。请先拆分词库或删除不需要的个人词条。";
      int answer=MessageBoxW(phraseWindow,summary.c_str(),L"确认批量导入",fits&&r.importNew?MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION:MB_OK|MB_ICONINFORMATION);
      Request next;next.client=importClient;next.op=fits&&r.importNew&&answer==IDYES?Op::ImportCommit:Op::ImportCancel;apply(w,next);return 0;
    }
    if(result->ok&&phraseWindow&&learnedMode)updateLearnedSummary(result->capacity);
    if(result->ok){if(result->op==Op::Configure||result->op==Op::Restore){load(result->response);
        SetWindowTextW(wordCapacityBox,std::to_wstring(result->capacity.importNew).c_str());SetWindowTextW(phraseCapacityBox,std::to_wstring(result->capacity.importDuplicate).c_str());}
      if(result->op==Op::LearnedList||result->op==Op::LearnedPut||result->op==Op::LearnedDelete||result->op==Op::PhraseList||result->op==Op::PhrasePut||result->op==Op::PhraseDelete||result->op==Op::ImportCommit){personal=std::move(result->items);if(phraseWindow){SendMessageW(phraseList,LB_RESETCONTENT,0,0);for(auto& item:personal){auto text=item.second+(learnedMode?L"    次数：":L"    [")+item.first+(learnedMode?L"":L"]");SendMessageW(phraseList,LB_ADDSTRING,0,(LPARAM)text.c_str());}}}
      SetWindowTextW(statusText,result->op==Op::ImportCancel?L"未导入新词 · 原有数据保留":result->op==Op::ImportCommit?L"词库已批量导入 · 新词已生效":result->op==Op::Backup?L"备份已导出 · 包含设置、学习词频和手动词条":result->op==Op::Restore?L"备份已恢复 · 下次输入生效":L"操作完成 · 本地数据已保存");}
    else{SetWindowTextW(statusText,result->committed?L"短词修改已保存，但列表刷新失败，请关闭后重新打开。":L"操作未完成，原有数据保留。");if(result->committed)result->error=L"短词修改已保存，但列表刷新失败，请关闭后重新打开。";MessageBoxW(phraseWindow?phraseWindow:w,result->error.empty()?L"操作未完成。容量不能低于已有记录数，范围为 1–100000。词库需为 UTF-8，使用默认 Rime 列顺序或“词语<TAB>拼音”格式；不支持搜狗 SCEL。也请检查文件权限、词库容量或重试。":result->error.c_str(),L"潮汐拼音",MB_OK|MB_ICONWARNING);}return 0;}
  if(m==WM_COMMAND){
    if(LOWORD(wp)>=300&&LOWORD(wp)<304){selectPage(w,LOWORD(wp)-300);return 0;}
    if(LOWORD(wp)==102&&HIWORD(wp)==CBN_SELCHANGE)InvalidateRect(previewWindow,nullptr,TRUE);
    if(LOWORD(wp)==103&&!busy){BOOL validWords=FALSE,validPhrases=FALSE;
      auto words=GetDlgItemInt(w,122,&validWords,FALSE),phrases=GetDlgItemInt(w,123,&validPhrases,FALSE);
      if(!validWords||!validPhrases||words<1||words>MaxWordCapacity||phrases<1||phrases>MaxWordCapacity){MessageBoxW(w,L"两项容量均需填写 1–100000 的整数；不能低于已有词条数。",L"词库容量",MB_OK|MB_ICONINFORMATION);return 0;}
      Request q;copyText(q.text,std::to_wstring(words)+L" "+std::to_wstring(phrases));q.op=Op::Configure;q.key=fontValue();q.value=0;q.mask=0;
      int selection=(int)SendMessageW(limitBox,CB_GETCURSEL,0,0);q.limit=selection==2?16:selection==1?8:4;
      if(SendMessageW(checks[0],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptLearning;
      if(SendMessageW(checks[1],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptAsciiPunct;
      if(SendMessageW(checks[2],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptCompletion;
      if(SendMessageW(checks[10],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptCorrection;
      if(SendMessageW(checks[11],BM_GETCHECK,0,0)==BST_CHECKED)q.value|=OptPrediction;
      for(int i=0;i<7;++i)if(SendMessageW(checks[i+3],BM_GETCHECK,0,0)==BST_CHECKED)q.mask|=fuzzyFlags[i];apply(w,q);
    }
    if((LOWORD(wp)==105||LOWORD(wp)==124)&&!busy){
      learnedMode=LOWORD(wp)==124;personal.clear();
      if(!phraseWindow){RECT rect={0,0,px(730),px(learnedMode?610:690)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE);phraseWindow=CreateWindowW(L"ChaoxiPinyin.Phrases",learnedMode?L"本地短词":L"个人词库",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,w,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(phraseWindow,SW_SHOW);EnableWindow(w,FALSE);}
      Request q;q.op=learnedMode?Op::LearnedList:Op::PhraseList;apply(w,q);
    }
    if(LOWORD(wp)==106&&!busy)chooseBackup(w,false);
    if(LOWORD(wp)==107&&!busy)chooseBackup(w,true);
    if(LOWORD(wp)==104&&!busy&&MessageBoxW(w,L"删除全部自动学习的词条及次数？手动词条、内置词库和设置保留。",L"清除学习数据",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)==IDYES){Request q;q.op=Op::Erase;apply(w,q);}return 0;
  }
  if(m==DM_GETDEFID)return MAKELRESULT(103,DC_HASDEFID);
  if(m==WM_NOTIFY){
    auto header=(NMHDR*)lp;
    if(header->code==NM_CUSTOMDRAW){
      auto draw=(NMCUSTOMDRAW*)lp;auto style=GetWindowLongW(header->hwndFrom,GWL_STYLE);
      if((style&BS_TYPEMASK)==BS_AUTOCHECKBOX&&draw->dwDrawStage==CDDS_PREPAINT){
        auto dc=draw->hdc;RECT rect=draw->rc;FillRect(dc,&rect,cardBackground);
        RECT box={px(1),px(5),px(19),px(23)};bool checked=SendMessageW(header->hwndFrom,BM_GETCHECK,0,0)==BST_CHECKED;
        auto brush=CreateSolidBrush(checked?RGB(206,235,211):RGB(255,255,255));FillRect(dc,&box,brush);DeleteObject(brush);
        FrameRect(dc,&box,(HBRUSH)GetStockObject(GRAY_BRUSH));
        if(checked){auto pen=CreatePen(PS_SOLID,px(2),RGB(39,77,83));auto previous=SelectObject(dc,pen);
          MoveToEx(dc,px(5),px(14),nullptr);LineTo(dc,px(9),px(18));LineTo(dc,px(16),px(10));SelectObject(dc,previous);DeleteObject(pen);}
        rect.left+=px(28);wchar_t text[256];GetWindowTextW(header->hwndFrom,text,256);
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,IsWindowEnabled(header->hwndFrom)?RGB(39,77,83):GetSysColor(COLOR_GRAYTEXT));
        auto previous=SelectObject(dc,uiFont);DrawTextW(dc,text,-1,&rect,DT_LEFT|DT_SINGLELINE|DT_VCENTER);SelectObject(dc,previous);
        if(draw->uItemState&CDIS_FOCUS)DrawFocusRect(dc,&rect);return CDRF_SKIPDEFAULT;
      }
    }
  }
  if(m==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(w,&ps);paintSettings(dc);EndPaint(w,&ps);return 0;}
  if(m==WM_DRAWITEM){drawButton(*(DRAWITEMSTRUCT*)lp);return TRUE;}
  if(m==WM_CTLCOLORSTATIC||m==WM_CTLCOLORBTN){auto dc=(HDC)wp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(39,77,83));RECT box;GetWindowRect((HWND)lp,&box);MapWindowPoints(nullptr,w,(POINT*)&box,2);bool card=box.left<px(210)||(box.left>=px(238)&&box.top<px(580));SetBkColor(dc,card?RGB(255,255,255):RGB(245,247,245));return (LRESULT)(card?cardBackground:background);}
  if(m==WM_CLOSE&&busy){MessageBeep(MB_OK);return 0;}
  if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
  SetProcessDPIAware();HDC dc=GetDC(nullptr);dpi=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(nullptr,dc);
  RECT work={};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
  int fit=std::min(MulDiv(work.right-work.left-40,96,960),MulDiv(work.bottom-work.top-70,96,700));
  dpi=std::max(72,std::min(dpi,fit));
  uiFont=CreateFontW(-px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  titleFont=CreateFontW(-px(27),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");
  WNDCLASSW wc={};wc.hInstance=h;wc.lpfnWndProc=preview;wc.lpszClassName=L"ChaoxiPinyin.Preview";wc.hbrBackground=background;RegisterClassW(&wc);
  wc.hIcon=LoadIconW(h,MAKEINTRESOURCEW(1));wc.lpfnWndProc=proc;wc.lpszClassName=L"ChaoxiPinyin.Settings";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
  wc.lpfnWndProc=phraseProc;wc.lpszClassName=L"ChaoxiPinyin.Phrases";RegisterClassW(&wc);
  RECT rect={0,0,px(960),px(700)};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
  HWND w=CreateWindowW(L"ChaoxiPinyin.Settings",L"潮汐拼音设置",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,h,nullptr);mainWindow=w;
#ifdef CHAOXI_SETTINGS_PREVIEW
  renderSettings(w);DestroyWindow(w);DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);DeleteObject(cardBackground);return 0;
#else
  ShowWindow(w,show);
#endif
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!(phraseWindow&&IsDialogMessageW(phraseWindow,&msg))&&!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
  if(worker.joinable())worker.join();DeleteObject(uiFont);DeleteObject(titleFont);DeleteObject(background);DeleteObject(cardBackground);return 0;
}
