#pragma once
#include "common.hpp"
#include "rime_api.h"
#include <sstream>

// Per-session phonetic disambiguation. Selection uses Rime's absolute candidate
// index, so filtering never changes the stored input or selects a different word.
struct NineKeyMenu {
  struct Item {int index;std::wstring text;std::string spelling;};
  std::string prefix;
  std::vector<std::string> spellings;
  std::vector<Item> items;
  int page=0;
  bool more=false;
  void clear(){prefix.clear();spellings.clear();items.clear();page=0;more=false;}
  static bool matches(const std::string& reading,const std::string& prefix){
    return prefix.empty()||reading==prefix||(reading.size()>prefix.size()&&reading.compare(0,prefix.size(),prefix)==0&&reading[prefix.size()]==' ');
  }
  void back(){auto space=prefix.rfind(' ');prefix=space==std::string::npos?"":prefix.substr(0,space);page=0;}
  bool choose(const std::string& spelling){
    if(spelling.empty()){clear();return true;}
    if(spelling=="-"){back();return true;}
    if(std::find(spellings.begin(),spellings.end(),spelling)==spellings.end())return false;
    if(!prefix.empty())prefix+=' ';prefix+=spelling;page=0;return true;
  }
  void update(RimeApi* api,RimeSessionId id){
    // An unconstrained key only needs a small preview. Filtered pages scan
    // incrementally to their requested page, rather than truncating the list.
    for(;;){
      spellings.clear();items.clear();more=false;
      RimeCandidateListIterator it{};
      if(api->candidate_list_begin(id,&it)){
        int scanned=0,matched=0;
        const int first=page*9,last=first+9;
        while(api->candidate_list_next(&it)){
          ++scanned;
          std::string reading=it.candidate.comment?it.candidate.comment:"";
          if(!matches(reading,prefix))continue;
          if(!prefix.empty()){
            if(matched>=first&&matched<last)items.push_back({it.index,wide(it.candidate.text),reading});
            ++matched;if(matched>last){more=true;break;}
          }
          std::istringstream syllables(prefix.empty()?reading:reading.size()>prefix.size()?reading.substr(prefix.size()+1):"");
          std::string syllable;syllables>>syllable;
          if(!syllable.empty()&&syllable.size()<=6&&syllable.find_first_not_of("abcdefghijklmnopqrstuvwxyz")==std::string::npos&&std::find(spellings.begin(),spellings.end(),syllable)==spellings.end())spellings.push_back(syllable);
          if(prefix.empty()&&scanned>=256)break;
        }
        api->candidate_list_end(&it);
      }
      if(prefix.empty()||!items.empty())break;
      if(page>0){--page;continue;}
      // Editing or partial word selection can invalidate a locked syllable.
      // Release only the last lock and retain earlier valid locks.
      back();
    }
  }
};
