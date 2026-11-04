#include "engine.hpp"
#include <fstream>
Engine::Engine(const fs::path& root,const fs::path& home):root_(root),home_(home) {
  fs::create_directories(home_);
  learning_=GetPrivateProfileIntW(L"settings",L"learning",1,(home_/L"settings.ini").c_str())!=0;
  font_=std::clamp((int)GetPrivateProfileIntW(L"settings",L"font",18,(home_/L"settings.ini").c_str()),12,32);
  std::ifstream words(home_/L"words.tsv"); std::string line;
  while(words_.size()<4096 && std::getline(words,line)) {
    auto tab=line.find('\t'); if(tab==std::string::npos)continue;
    try { auto word=wide(line.substr(0,tab).c_str()); if(word.size()<=4)words_[word]=std::stoull(line.substr(tab+1)); }catch(...){}
  }
  dll_=LoadLibraryExW((root_/L"rime.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!dll_) throw std::runtime_error("Cannot load rime.dll (or its runtime dependencies)");
  auto get=reinterpret_cast<RimeApi*(*)()>(GetProcAddress(dll_,"rime_get_api"));
  if(!get) throw std::runtime_error("rime_get_api missing"); api_=get();
  start();
}
void Engine::saveWords() {
  auto temp=home_/L"words.tmp"; std::ofstream file(temp,std::ios::binary|std::ios::trunc);
  for(const auto& [word,count]:words_)file<<utf8(word)<<'\t'<<count<<'\n';
  file.close(); if(!file || !MoveFileExW(temp.c_str(),(home_/L"words.tsv").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot save word frequencies");
}
Engine::~Engine() { stop(); if(dll_) FreeLibrary(dll_); }
void Engine::save() {
  auto file=home_/L"settings.ini";
  if(!WritePrivateProfileStringW(L"settings",L"learning",learning_?L"1":L"0",file.c_str()) ||
     !WritePrivateProfileStringW(L"settings",L"font",std::to_wstring(font_).c_str(),file.c_str())) throw std::runtime_error("Cannot save settings");
}
void Engine::start() {
  fs::create_directories(home_/L"engine"); fs::create_directories(home_/L"cache");
  shared_=utf8((root_/L"data").wstring()); user_=utf8((home_/L"engine").wstring()); staging_=utf8((home_/L"cache").wstring());
  RIME_STRUCT(RimeTraits,t); t.shared_data_dir=shared_.c_str(); t.user_data_dir=user_.c_str();
  t.prebuilt_data_dir=shared_.c_str(); t.staging_dir=staging_.c_str();
  t.distribution_name="JianPin"; t.distribution_code_name="jianpin"; t.distribution_version="0.1.0";
  t.app_name="rime.jianpin"; t.min_log_level=3; t.log_dir="";
  static const char* modules[]={"default",nullptr}; t.modules=modules;
  api_->setup(&t); api_->initialize(&t);
  // Maintenance is never run on a browser thread. Both schemas are built before IPC is ready.
  if(api_->start_maintenance(False)) api_->join_maintenance_thread();
  auto id=api_->create_session();
  bool ok=id && api_->select_schema(id,"jianpin");
  if(id) api_->destroy_session(id);
  if(!ok) { stop(); throw std::runtime_error("Schema deployment failed"); }
}
void Engine::stop() { if(api_) { api_->cleanup_all_sessions(); api_->finalize(); } sessions_.clear(); order_.clear(); }
RimeSessionId Engine::session(uint64_t client) {
  auto it=sessions_.find(client); if(it!=sessions_.end()) return it->second;
  for(auto old=sessions_.begin();old!=sessions_.end();) {
    DWORD pid=(DWORD)(old->first>>32);HANDLE process=pid?OpenProcess(SYNCHRONIZE,FALSE,pid):nullptr;
    bool dead=process && WaitForSingleObject(process,0)==WAIT_OBJECT_0;
    if(!process && pid && GetLastError()==ERROR_INVALID_PARAMETER)dead=true;
    if(process)CloseHandle(process);
    if(dead){api_->destroy_session(old->second);order_.erase(old->first);old=sessions_.erase(old);}else ++old;
  }
  if(sessions_.size()>=256) throw std::runtime_error("Too many sessions");
  auto id=api_->create_session();
  if(!id || !api_->select_schema(id,"jianpin")) throw std::runtime_error("Session failed");
  sessions_[client]=id; return id;
}
Response Engine::handle(const Request& q) {
  Response r;
  if(q.magic!=kMagic) return r;
  if(q.op==Op::Learning) {
    learning_=q.value!=0; save();
    // Rime user dictionaries are always disabled. Only our bounded word-frequency table learns.
    for(auto& s:sessions_) api_->destroy_session(s.second);
    sessions_.clear(); order_.clear();
  } else if(q.op==Op::Font) { font_=std::clamp(q.value,12,32); save(); }
  else if(q.op==Op::Erase) {
    words_.clear(); saveWords();storageError_=false;
    for(auto& s:sessions_)api_->destroy_session(s.second);
    sessions_.clear();order_.clear();
  } else if(q.op==Op::Destroy) {
    auto it=sessions_.find(q.client); if(it!=sessions_.end()) { api_->destroy_session(it->second); sessions_.erase(it); } order_.erase(q.client);
  } else if(q.op==Op::Clear || q.op==Op::Key) {
    auto id=session(q.client);
    if(q.op==Op::Clear) api_->clear_composition(id);
    else {
      const char* input=api_->get_input(id);
      if(input && strlen(input)>=120 && q.key>=32 && q.key<=126 && q.key!=' ') r.eaten=1;
      else {
        int key=q.key; auto& order=order_[q.client];
        int selected=key==' '?0:(key>='1'&&key<='9'?key-'1':-1);
        if(selected>=0 && selected<(int)order.size())key='1'+order[selected];
        r.eaten=api_->process_key(id,key,q.mask);
      }
    }
    RIME_STRUCT(RimeCommit,c);
    if(api_->get_commit(id,&c)) {
      auto word=wide(c.text);copyText(r.commit,word);api_->free_commit(&c);
      // Learn only short Chinese words, never full sentences, punctuation, Latin text, or raw keys.
      if(learning_ && !word.empty() && word.size()<=4 && std::all_of(word.begin(),word.end(),[](wchar_t ch){return ch>=0x3400&&ch<=0x9fff;})) {
        if(words_.size()<4096 || words_.count(word)){auto& count=words_[word];count=std::min<uint64_t>(count+1,1000000);try{saveWords();storageError_=false;}catch(...){storageError_=true;}}
      }
    }
    RIME_STRUCT(RimeContext,cx);
    if(api_->get_context(id,&cx)) {
      copyText(r.preedit,wide(cx.composition.preedit));
      r.page=cx.menu.page_no; r.last=cx.menu.is_last_page; r.highlight=cx.menu.highlighted_candidate_index;
      r.count=std::min(9,cx.menu.num_candidates);
      auto& order=order_[q.client];order.clear();for(int i=0;i<r.count;++i)order.push_back(i);
      if(learning_)std::stable_sort(order.begin(),order.end(),[&](int a,int b){
        auto score=[&](int n){auto it=words_.find(wide(cx.menu.candidates[n].text));return it==words_.end()?uint64_t(0):it->second;};return score(a)>score(b);
      });
      for(int i=0;i<r.count;++i)copyText(r.candidates[i],wide(cx.menu.candidates[order[i]].text));
      r.highlight=0;
      api_->free_context(&cx);
    }
  }
  r.learning=learning_; r.font=font_; r.storageError=storageError_;r.ok=1; return r;
}
