#pragma once
#include "common.hpp"
#include <fstream>

// Read only the pinned word/code table, never YAML directives or other files.
class EnglishLexicon {
  struct Entry {std::string code;std::wstring word;};
  std::vector<Entry> entries_;
  static std::string lower(std::string text) {
    for(auto& c:text)if(c>='A'&&c<='Z')c+=('a'-'A');return text;
  }
public:
  static bool letters(const std::string& text) {
    return !text.empty()&&text.size()<=120&&std::all_of(text.begin(),text.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z');});
  }
  void load(const fs::path& root) {
    entries_.clear();std::ifstream file(root/L"data"/L"en_dicts"/L"en.dict.yaml");std::string line;bool body=false;
    while(std::getline(file,line)) {
      if(!line.empty()&&line.back()=='\r')line.pop_back();
      if(!body){if(line=="...")body=true;continue;}
      if(line.empty()||line[0]=='#')continue;
      auto tab=line.find('\t');if(tab==std::string::npos)continue;
      auto word=line.substr(0,tab),code=line.substr(tab+1);
      if(!letters(word)||!letters(code)||word.size()>64||lower(word)!=lower(code))continue;
      entries_.push_back({lower(code),wide(word.c_str())});
    }
    std::sort(entries_.begin(),entries_.end(),[](const auto& a,const auto& b){return a.code!=b.code?a.code<b.code:a.word<b.word;});
    entries_.erase(std::unique(entries_.begin(),entries_.end(),[](const auto& a,const auto& b){return a.code==b.code&&a.word==b.word;}),entries_.end());
  }
  std::vector<std::wstring> complete(const std::string& raw)const {
    if(!letters(raw)||raw.size()<3||raw.size()>64)return {};
    auto code=lower(raw);auto original=wide(raw.c_str());std::vector<const Entry*> matches;
    auto it=std::lower_bound(entries_.begin(),entries_.end(),code,[](const auto& entry,const auto& value){return entry.code<value;});
    for(;it!=entries_.end()&&it->code.compare(0,code.size(),code)==0;++it)
      if(it->word!=original)matches.push_back(&*it);
    std::sort(matches.begin(),matches.end(),[](auto a,auto b){return a->word.size()!=b->word.size()?a->word.size()<b->word.size():a->word<b->word;});
    std::vector<std::wstring> result;
    for(auto entry:matches){result.push_back(entry->word);if(result.size()==3)break;}return result;
  }
};
