#pragma once
#include "profile.hpp"
// Data-only reader. YAML metadata, Lua and import_tables are never executed.
struct DictionaryImport {
  std::map<std::pair<std::string,std::wstring>,int> phrases;
  int invalid=0,duplicates=0;
  static DictionaryImport parse(std::string data) {
    if(data.empty()||data.size()>16*1024*1024||data.find('\0')!=std::string::npos||
       !validUtf8(data))throw std::runtime_error("Expected UTF-8 dictionary");
    if(data.compare(0,3,"\xef\xbb\xbf")==0)data.erase(0,3);
    bool yaml=false;{std::istringstream test(data);std::string s;while(std::getline(test,s)){if(!s.empty()&&s.back()=='\r')s.pop_back();if(s=="---"){yaml=true;break;}if(s.find('\t')!=std::string::npos)break;}}
    DictionaryImport result;std::istringstream in(data);std::string line;bool body=!yaml;
    while(std::getline(in,line)){
      if(!line.empty()&&line.back()=='\r')line.pop_back();
      if(!body){if(line=="...")body=true;
        // Custom column mappings require a converter; do not silently swap fields.
        if(line.rfind("columns:",0)==0)throw std::runtime_error("Custom columns unsupported");continue;}
      if(line.empty()||line[0]=='#'||line.find_first_not_of(" \t")==std::string::npos)continue;
      try{auto first=line.find('\t');if(first==std::string::npos)throw std::runtime_error("Missing pinyin");auto end=line.find('\t',first+1);
        auto word=wide(line.substr(0,first).c_str());auto code=phraseCode(wide(line.substr(first+1,end==std::string::npos?end:end-first-1).c_str()));
        if(!chineseWord(word))throw std::runtime_error("Invalid word");
        if(!result.phrases.emplace(std::make_pair(code,word),0).second)++result.duplicates;
      }catch(...){++result.invalid;}
    }
    if(!body)throw std::runtime_error("Missing dictionary body");return result;
  }
};
inline std::string readDictionaryFile(const fs::path& path){
  if(fs::file_size(path)>16*1024*1024)throw std::runtime_error("Dictionary too large");
  std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot open dictionary");
  std::string data;char buffer[65536];while(in.read(buffer,sizeof(buffer))||in.gcount()){data.append(buffer,(size_t)in.gcount());if(data.size()>16*1024*1024)throw std::runtime_error("Dictionary too large");}
  if(!in.eof())throw std::runtime_error("Dictionary read failed");return data;
}
