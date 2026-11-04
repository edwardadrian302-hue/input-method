#pragma once
#include "common.hpp"
// Shared by the real TSF popup and the offline rendering preview.
class CandidatePanel {
  HFONT text_=nullptr,small_=nullptr;
public:
  int dpi=96,width=160,height=280,row=26,header=27,footer=23;
  int px(int n) const{return MulDiv(n,dpi,96);}
  ~CandidatePanel(){if(text_)DeleteObject(text_);if(small_)DeleteObject(small_);}
  void layout(HDC dc,const Response& v,UINT scale,int maxWidth,int maxHeight=10000){
    dpi=scale;if(text_)DeleteObject(text_);if(small_)DeleteObject(small_);
    auto make=[&](int size){return CreateFontW(-size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Microsoft YaHei UI");};
    int font=MulDiv(v.font,dpi,72);text_=make(font);small_=make(px(13));
    row=font+px(8);header=px(27);footer=px(23);width=px(150);
    auto old=SelectObject(dc,text_);SIZE s={};
    for(int i=0;i<v.count;++i){GetTextExtentPoint32W(dc,v.candidates[i],(int)wcslen(v.candidates[i]),&s);width=std::max(width,(int)s.cx+px(64));}
    const wchar_t* heading=v.prediction?L"联想 · 空格或数字选择":v.preedit;
    SelectObject(dc,small_);GetTextExtentPoint32W(dc,heading,(int)wcslen(heading),&s);width=std::max(width,(int)s.cx+px(32));SelectObject(dc,old);
    width=std::min(width,std::min(px(440),maxWidth));height=header+row*v.count+footer;
    if(height>maxHeight){row=std::max(1,(maxHeight-header-footer)/std::max(1,v.count));height=std::min(maxHeight,header+row*v.count+footer);}
  }
  void paint(HDC target,const Response& v){
    HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,width,height);auto previous=SelectObject(dc,bitmap);
    RECT all={0,0,width,height};HBRUSH white=CreateSolidBrush(RGB(255,255,255));FillRect(dc,&all,white);DeleteObject(white);
    SetBkMode(dc,TRANSPARENT);auto font=SelectObject(dc,small_);
    auto label=[&](std::wstring s,RECT r,COLORREF color,UINT flags=DT_LEFT){SetTextColor(dc,color);DrawTextW(dc,s.c_str(),(int)s.size(),&r,flags|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);};
    label(v.prediction?L"联想 · 空格或数字选择":v.preedit,{px(14),px(3),width-px(14),header},RGB(132,135,130));
    for(int i=0;i<v.count;++i){
      int top=header+i*row;
      if(i==v.highlight){auto pen=CreatePen(PS_NULL,0,0);auto oldPen=SelectObject(dc,pen);auto brush=CreateSolidBrush(RGB(218,232,191));auto oldBrush=SelectObject(dc,brush);RoundRect(dc,px(6),top,width-px(6),top+row,px(8),px(8));SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);}
      SelectObject(dc,small_);label(std::to_wstring(i+1),{px(16),top,px(34),top+row},i==v.highlight?RGB(139,157,115):RGB(183,185,181));
      SelectObject(dc,text_);label(v.candidates[i],{px(36),top,width-px(15),top+row},RGB(39,43,36));
    }
    SelectObject(dc,small_);
    std::wstring page=v.prediction?L"Esc 关闭联想":L"第 "+std::to_wstring(v.page+1)+L" 页";
    if(v.page>0)page=L"‹  "+page;if(!v.last)page+=L"  ›";
    label(page,{px(14),height-footer,width-px(10),height-px(2)},RGB(166,171,162),DT_RIGHT);
    SelectObject(dc,font);BitBlt(target,0,0,width,height,dc,0,0,SRCCOPY);SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);
  }
};
