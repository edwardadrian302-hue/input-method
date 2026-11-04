#pragma once
#include "common.hpp"
#include "rime_api.h"
#include <map>
class Engine {
  HMODULE dll_=nullptr;
  RimeApi* api_=nullptr;
  fs::path root_,home_;
  std::string shared_,user_,staging_;
  std::map<uint64_t,RimeSessionId> sessions_;
  std::map<uint64_t,std::vector<int>> order_;
  std::map<std::wstring,uint64_t> words_;
  bool learning_=true;
  bool storageError_=false;
  int font_=18;
  void start();
  void stop();
  void save();
  void saveWords();
  RimeSessionId session(uint64_t);
public:
  Engine(const fs::path& root,const fs::path& home);
  ~Engine();
  Response handle(const Request& q);
};
