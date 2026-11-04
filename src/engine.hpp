#pragma once
#include "common.hpp"
#include "rime_api.h"
#include "lexicon.hpp"
#include "dictionary_import.hpp"
#include "english.hpp"
#include <map>
class Engine {
  HMODULE dll_=nullptr;
  RimeApi* api_=nullptr;
  fs::path root_,home_;
  std::string shared_,user_,staging_;
  std::map<uint64_t,RimeSessionId> sessions_;
  struct Choice {int index;std::wstring text;};
  std::map<uint64_t,std::vector<Choice>> choices_;
  std::map<uint64_t,std::vector<std::wstring>> predictions_;
  struct Paging {int page=0,pages=1,rimePage=0;};
  std::map<uint64_t,Paging> paging_;
  Profile profile_;Lexicon lexicon_;EnglishLexicon english_;
  DictionaryImport pendingImport_;uint64_t importOwner_=0,importTime_=0;
  bool storageError_=false;
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
};
