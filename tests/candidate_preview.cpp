#include "candidate.hpp"
#include <fstream>
int wmain(){
  Response r;copyText(r.preedit,L"da'xue'feng'shan");r.count=9;r.last=0;
  const wchar_t* words[]={L"大雪封山",L"大学",L"大雪",L"大",L"打",L"达",L"答",L"搭",L"塔"};
  for(int i=0;i<9;++i)copyText(r.candidates[i],words[i]);
  for(int dpi:{96,144,192}){
    CandidatePanel panel;HDC screen=GetDC(nullptr);panel.layout(screen,r,dpi,1920);HDC dc=CreateCompatibleDC(screen);
    BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=panel.width;bi.bmiHeader.biHeight=-panel.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(dc,bitmap);panel.paint(dc,r);GdiFlush();
    BITMAPFILEHEADER header={};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+panel.width*panel.height*4;
    std::ofstream out(moduleDir()/(L"candidate-"+std::to_wstring(dpi)+L".bmp"),std::ios::binary);out.write((char*)&header,sizeof(header));out.write((char*)&bi.bmiHeader,sizeof(BITMAPINFOHEADER));out.write((char*)pixels,panel.width*panel.height*4);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);if(!out)return 1;
  }return 0;
}
