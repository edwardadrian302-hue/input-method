#pragma once
#include "common.hpp"
#include "rime_api.h"
#include "lexicon.hpp"
#include "dictionary_import.hpp"
#include "english.hpp"
#include "nine_key.hpp"
#include <map>
class Engine {
#ifdef _WIN32
  HMODULE dll_=nullptr;
#endif
  RimeApi* api_=nullptr;
  fs::path root_,home_;
  std::string shared_,user_,staging_;
  std::map<uint64_t,RimeSessionId> sessions_;
  struct Choice {int index;std::wstring text;bool absolute=false;};
  std::map<uint64_t,NineKeyMenu> nineMenus_;
  std::map<uint64_t,std::vector<Choice>> choices_;
  std::map<uint64_t,std::vector<std::wstring>> predictions_;
  struct Paging {int page=0,pages=1,rimePage=0;bool last=true;};
  std::map<uint64_t,Paging> paging_;
  Profile profile_;Lexicon lexicon_;EnglishLexicon english_;
  DictionaryImport pendingImport_;uint64_t importOwner_=0,importTime_=0;
  bool storageError_=false,learningSuppressed_=false,nineKey_=false;
  void configureSchema();
  void start();
  void stop();
  void save();
  void resetSessions();
  void predict(uint64_t,const std::wstring&,Response&);
  RimeSessionId session(uint64_t);
public:
  Engine(const fs::path& root,const fs::path& home);
  ~Engine();
  Response handle(const Request& q);
  void setNineKey(bool value) {if(nineKey_!=value){resetSessions();nineKey_=value;}}
  std::vector<std::string> nineKeySpellings(uint64_t client)const {auto it=nineMenus_.find(client);return it==nineMenus_.end()?std::vector<std::string>{}:it->second.spellings;}
  std::string nineKeyPrefix(uint64_t client)const {auto it=nineMenus_.find(client);return it==nineMenus_.end()?"":it->second.prefix;}
  Response chooseNineKeySpelling(uint64_t client,const std::string& spelling){
    if(!nineKey_||!nineMenus_[client].choose(spelling))throw std::runtime_error("Invalid nine-key spelling");
    if(nineMenus_[client].prefix.empty()){paging_[client]=Paging{};api_->highlight_candidate(session(client),0);}
    Request q;q.client=client;q.op=Op::Key;q.key=0;return handle(q);
  }
  void suppressLearning(bool value) {learningSuppressed_=value;}
};
