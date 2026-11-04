#pragma once
#include "profile.hpp"
#include <numeric>
// Bounded, read-only indexes built from the bundled, pinned Chinese dictionaries.
class Lexicon {
  struct Entry {std::string code;std::wstring word;uint64_t weight;};
  std::vector<Entry> entries_;std::vector<size_t> byWord_;
public:
  bool contains(const std::pair<std::string,std::wstring>& item)const {
    auto it=std::lower_bound(entries_.begin(),entries_.end(),item.first,[](auto& e,auto& code){return e.code<code;});
    for(;it!=entries_.end()&&it->code==item.first;++it)if(it->word==item.second)return true;return false;
  }
  void load(const fs::path& root) {
    for(auto name:{L"8105",L"base",L"ext"}) {
      std::ifstream in(root/L"data"/L"cn_dicts"/(std::wstring(name)+L".dict.yaml"));if(!in)throw std::runtime_error("Dictionary index missing");
      std::string line;bool body=false;
      while(std::getline(in,line)) {
        if(!body){if(line=="..."||line=="...\r")body=true;continue;}
        if(line.empty()||line[0]=='#')continue;auto tab=line.find('\t');if(tab==std::string::npos)continue;
        auto end=line.find('\t',tab+1);if(end==std::string::npos)continue;
        try {auto word=wide(line.substr(0,tab).c_str());if(!chineseWord(word,32))continue;auto code=phraseCode(wide(line.substr(tab+1,end-tab-1).c_str()));
          uint64_t weight=std::min<uint64_t>(std::stoull(line.substr(end+1)),1000000000);entries_.push_back({code,word,weight});}catch(...){}
      }
    }
    std::sort(entries_.begin(),entries_.end(),[](auto& a,auto& b){return a.code<b.code;});
    byWord_.resize(entries_.size());std::iota(byWord_.begin(),byWord_.end(),0);
    std::sort(byWord_.begin(),byWord_.end(),[&](auto a,auto b){return entries_[a].word<entries_[b].word;});
  }
  std::vector<std::wstring> corrections(const std::string& code)const {
    if(code.size()<4||code.size()>48)return {};
    std::set<std::string> variants;
    for(size_t i=0;i<code.size();++i){auto s=code;s.erase(i,1);variants.insert(s);if(i+1<code.size()){s=code;std::swap(s[i],s[i+1]);variants.insert(s);}
      for(char c='a';c<='z';++c){s=code;s[i]=c;variants.insert(s);}}
    for(size_t i=0;i<=code.size();++i)for(char c='a';c<='z';++c){auto s=code;s.insert(i,1,c);variants.insert(s);}
    variants.erase(code);std::map<std::wstring,uint64_t> scores;
    for(auto& v:variants){auto it=std::lower_bound(entries_.begin(),entries_.end(),v,[](auto& e,auto& c){return e.code<c;});
      for(int n=0;it!=entries_.end()&&it->code==v&&n<512;++it,++n)if(it->word.size()>1)scores[it->word]=std::max(scores[it->word],it->weight);}
    return ranked(scores);
  }
  std::vector<std::wstring> predict(const std::wstring& prefix)const {
    if(!chineseWord(prefix,16))return {};std::map<std::wstring,uint64_t> scores;
    auto it=std::lower_bound(byWord_.begin(),byWord_.end(),prefix,[&](auto i,auto& w){return entries_[i].word<w;});
    for(int n=0;it!=byWord_.end()&&n<8192;++it,++n){auto& e=entries_[*it];if(e.word.compare(0,prefix.size(),prefix))break;
      if(e.word.size()>prefix.size()){auto suffix=e.word.substr(prefix.size());scores[suffix]=std::max(scores[suffix],e.weight);}}
    return ranked(scores);
  }
  static std::vector<std::wstring> ranked(const std::map<std::wstring,uint64_t>& scores) {
    std::vector<std::pair<std::wstring,uint64_t>> items(scores.begin(),scores.end());std::stable_sort(items.begin(),items.end(),[](auto& a,auto& b){return a.second>b.second;});
    std::vector<std::wstring> result;for(auto& e:items){result.push_back(e.first);if(result.size()==9)break;}return result;
  }
};
