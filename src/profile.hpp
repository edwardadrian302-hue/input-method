#pragma once
#include "common.hpp"
#include <fstream>
#include <sstream>
#include <map>
#include <set>

inline bool chineseWord(const std::wstring& word,size_t max=32) {
  return !word.empty()&&word.size()<=max&&std::all_of(word.begin(),word.end(),[](wchar_t c){return c>=0x3400&&c<=0x9fff;});
}
inline std::string phraseCode(const std::wstring& text) {
  std::string out;
  for(auto c:text){if(c==L' '||c==L'\'')continue;if(c>=L'A'&&c<=L'Z')c+=32;if(c<L'a'||c>L'z')throw std::runtime_error("Invalid pinyin");out+=(char)c;}
  if(out.empty()||out.size()>96)throw std::runtime_error("Invalid pinyin length");return out;
}
struct Profile {
  bool learning=true;int font=12,options=OptCompletion,fuzzy=0,limit=4;
  std::map<std::wstring,uint64_t> words;
  std::map<std::pair<std::string,std::wstring>,int> phrases;
  std::string encode()const {
    std::ostringstream out;out<<"CHAOXI_BACKUP\t1\n"<<"SET\t"<<learning<<'\t'<<font<<'\t'<<options<<'\t'<<fuzzy<<'\t'<<limit<<'\n';
    for(auto& [word,count]:words)out<<"WORD\t"<<utf8(word)<<'\t'<<count<<'\n';
    for(auto& [item,unused]:phrases)out<<"PHRASE\t"<<item.first<<'\t'<<utf8(item.second)<<'\n';
    out<<"END\t"<<words.size()<<'\t'<<phrases.size()<<'\n';return out.str();
  }
  static Profile decode(const std::string& data) {
    if(data.size()>4*1024*1024||data.find('\0')!=std::string::npos)throw std::runtime_error("Invalid backup size");
    std::istringstream in(data);std::string line;Profile p;
    auto read=[&](){if(!std::getline(in,line))throw std::runtime_error("Incomplete backup");if(!line.empty()&&line.back()=='\r')line.pop_back();};
    auto fields=[](const std::string& s){std::vector<std::string> a;std::istringstream row(s);std::string f;while(std::getline(row,f,'\t'))a.push_back(f);return a;};
    auto number=[](const std::string& s,uint64_t max){if(s.empty()||!std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}))throw std::runtime_error("Invalid number");auto n=std::stoull(s);if(n>max)throw std::runtime_error("Number outside limits");return n;};
    auto unicode=[](const std::string& s){if(s.empty()||!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0))throw std::runtime_error("Invalid UTF-8");return wide(s.c_str());};
    read();if(line!="CHAOXI_BACKUP\t1")throw std::runtime_error("Unsupported backup");read();auto a=fields(line);
    if(a.size()!=6||a[0]!="SET")throw std::runtime_error("Missing settings");
    p.learning=number(a[1],1);p.font=(int)number(a[2],24);p.options=(int)number(a[3],31);p.fuzzy=(int)number(a[4],127);p.limit=(int)number(a[5],16);
    if(p.font<10||(p.options&~StoredOptions)||(p.limit!=4&&p.limit!=8&&p.limit!=16))throw std::runtime_error("Invalid settings");
    bool ended=false;while(std::getline(in,line)) {
      if(!line.empty()&&line.back()=='\r')line.pop_back();a=fields(line);if(a.size()!=3)throw std::runtime_error("Invalid record");
      if(a[0]=="END"){if(number(a[1],8192)!=p.words.size()||number(a[2],4096)!=p.phrases.size()||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Incomplete backup");ended=true;break;}
      else if(a[0]=="WORD") {auto w=unicode(a[1]);auto n=number(a[2],1000000);if(!chineseWord(w,16)||!n||!p.words.emplace(w,n).second||p.words.size()>8192)throw std::runtime_error("Invalid word");}
      else if(a[0]=="PHRASE") {auto w=unicode(a[2]);auto code=phraseCode(unicode(a[1]));if(code!=a[1]||!chineseWord(w)||!p.phrases.emplace(std::make_pair(code,w),0).second||p.phrases.size()>4096)throw std::runtime_error("Invalid phrase");}
      else throw std::runtime_error("Unknown record");
    }if(!ended)throw std::runtime_error("Incomplete backup");return p;
  }
};
inline std::string readProfileFile(const fs::path& file) {
  if(fs::file_size(file)>4*1024*1024)throw std::runtime_error("File too large");
  std::ifstream in(file,std::ios::binary);if(!in)throw std::runtime_error("Cannot read file");
  std::string result((std::istreambuf_iterator<char>(in)),{});if(in.bad())throw std::runtime_error("Cannot read file");return result;
}
inline void writeProfileFile(const fs::path& file,const std::string& data) {
  auto temp=file;temp+=L".chaoxi-tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
  // Never overwrite a leftover temporary file owned by another operation.
  HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create temporary file");
  DWORD n=0;bool ok=WriteFile(h,data.data(),(DWORD)data.size(),&n,nullptr)&&n==data.size()&&FlushFileBuffers(h);CloseHandle(h);
  if(!ok||!MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());throw std::runtime_error("Cannot save file");}
}
