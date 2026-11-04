#include "engine.hpp"
#include <fstream>
#include <sstream>
Engine::Engine(const fs::path& root,const fs::path& home):root_(root),home_(home) {
  fs::create_directories(home_);
  auto state=home_/L"profile.tsv";
  if(fs::exists(state))profile_=Profile::decode(readProfileFile(state));
  else {
    auto ini=home_/L"settings.ini";
    profile_.learning=GetPrivateProfileIntW(L"settings",L"learning",1,ini.c_str())!=0;
    profile_.font=std::clamp((int)GetPrivateProfileIntW(L"settings",L"font",12,ini.c_str()),10,24);
    profile_.options=GetPrivateProfileIntW(L"settings",L"options",OptCompletion,ini.c_str())&3;
    profile_.fuzzy=GetPrivateProfileIntW(L"settings",L"fuzzy",0,ini.c_str())&127;
    std::ifstream old(home_/L"words.tsv");std::string line;
    while(profile_.words.size()<8192&&std::getline(old,line)){auto tab=line.find('\t');if(tab==std::string::npos)continue;
      try{auto word=wide(line.substr(0,tab).c_str());auto count=std::stoull(line.substr(tab+1));if(chineseWord(word,4)&&count)profile_.words[word]=std::min<uint64_t>(count,1000000);}catch(...){} }
  }
  lexicon_.load(root_);
  english_.load(root_);
  dll_=LoadLibraryExW((root_/L"rime.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!dll_) throw std::runtime_error("Cannot load rime.dll (or its runtime dependencies)");
  auto get=reinterpret_cast<RimeApi*(*)()>(GetProcAddress(dll_,"rime_get_api"));
  if(!get) throw std::runtime_error("rime_get_api missing"); api_=get();
  start();
}
Engine::~Engine() { stop(); if(dll_) FreeLibrary(dll_); }
void Engine::save() {writeProfileFile(home_/L"profile.tsv",profile_.encode());}
void Engine::resetSessions() {
  for(auto& item:sessions_)api_->destroy_session(item.second);
  sessions_.clear();choices_.clear();predictions_.clear();paging_.clear();
}
void Engine::predict(uint64_t client,const std::wstring& word,Response& r) {
  auto& list=predictions_[client];list.clear();
  if(profile_.options&OptPrediction)list=lexicon_.predict(word);
  if(profile_.options&OptPrediction)for(auto& [item,unused]:profile_.phrases){auto& text=item.second;
    if(text.size()>word.size()&&text.compare(0,word.size(),word)==0){auto suffix=text.substr(word.size());if(std::find(list.begin(),list.end(),suffix)==list.end())list.insert(list.begin(),suffix);}}
  if(list.size()>9)list.resize(9);r.prediction=!list.empty();r.count=(int)list.size();r.page=0;r.last=1;
  for(int i=0;i<r.count;++i)copyText(r.candidates[i],list[i]);
}
void Engine::configureSchema() {
  fs::create_directories(home_/L"engine-v5");
  auto file=home_/L"engine-v5"/L"chaoxipinyin.custom.yaml";
  std::ostringstream text;
  text<<"patch:\n  translator/enable_completion: "<<((profile_.options&OptCompletion)?"true":"false")
      <<"\n  translator/enable_word_completion: false\n  speller/algebra:\n";
  // Derive fuzzy spellings before abbreviating, so shenm / sm remain valid.
  if(profile_.fuzzy&FuzzyZ)text<<"    - derive/^zh/z/\n    - derive/^z([^h])/zh$1/\n";
  if(profile_.fuzzy&FuzzyC)text<<"    - derive/^ch/c/\n    - derive/^c([^h])/ch$1/\n";
  if(profile_.fuzzy&FuzzyS)text<<"    - derive/^sh/s/\n    - derive/^s([^h])/sh$1/\n";
  if(profile_.fuzzy&FuzzyNL)text<<"    - derive/^n/l/\n    - derive/^l/n/\n";
  if(profile_.fuzzy&FuzzyEn)text<<"    - derive/eng$/en/\n    - derive/en$/eng/\n";
  if(profile_.fuzzy&FuzzyAn)text<<"    - derive/ang$/an/\n    - derive/an$/ang/\n";
  if(profile_.fuzzy&FuzzyIn)text<<"    - derive/ing$/in/\n    - derive/in$/ing/\n";
  text<<"    - derive/^([nl])ue$/$1ve/\n    - derive/^([jqxy])u/$1v/\n";
  if(profile_.options&OptCompletion)text<<"    - abbrev/^([a-z]).+$/$1/\n    - abbrev/^([zcs]h).+$/$1/\n";
  auto content=text.str();std::ifstream previous(file,std::ios::binary);
  std::string old((std::istreambuf_iterator<char>(previous)),{});
  previous.close();
  if(old==content)return;
  // Rime's modification check has second-level timestamps. Invalidate just the
  // derived schema/prism so rapid changes cannot reuse the previous spelling rules.
  fs::remove(home_/L"cache-v5"/L"chaoxipinyin.schema.yaml");
  fs::remove(home_/L"cache-v5"/L"chaoxipinyin.prism.bin");
  auto temp=file;temp+=L".tmp";std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<content;out.close();
  if(!out||!MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot save spelling configuration");
}
void Engine::start() {
  configureSchema();
  fs::create_directories(home_/L"engine-v5"); fs::create_directories(home_/L"cache-v5");
  shared_=utf8((root_/L"data").wstring()); user_=utf8((home_/L"engine-v5").wstring()); staging_=utf8((home_/L"cache-v5").wstring());
  RIME_STRUCT(RimeTraits,t); t.shared_data_dir=shared_.c_str(); t.user_data_dir=user_.c_str();
  t.prebuilt_data_dir=shared_.c_str(); t.staging_dir=staging_.c_str();
  t.distribution_name="ChaoxiPinyin"; t.distribution_code_name="chaoxipinyin"; t.distribution_version="0.6.0";
  t.app_name="rime.chaoxipinyin"; t.min_log_level=3; t.log_dir="";
  static const char* modules[]={"default",nullptr}; t.modules=modules;
  api_->setup(&t); api_->initialize(&t);
  // Maintenance is never run on a browser thread. The schema is built before IPC is ready.
  if(api_->start_maintenance(True)) api_->join_maintenance_thread();
  auto id=api_->create_session();
  bool ok=id && api_->select_schema(id,"chaoxipinyin");
  if(id) api_->destroy_session(id);
  if(!ok) { stop(); throw std::runtime_error("Schema deployment failed"); }
}
void Engine::stop() { if(api_) { api_->cleanup_all_sessions(); api_->finalize(); } sessions_.clear(); choices_.clear(); predictions_.clear();paging_.clear(); }
RimeSessionId Engine::session(uint64_t client) {
  auto it=sessions_.find(client); if(it!=sessions_.end()) return it->second;
  for(auto old=sessions_.begin();old!=sessions_.end();) {
    DWORD pid=(DWORD)(old->first>>32);HANDLE process=pid?OpenProcess(SYNCHRONIZE,FALSE,pid):nullptr;
    bool dead=process && WaitForSingleObject(process,0)==WAIT_OBJECT_0;
    if(!process && pid && GetLastError()==ERROR_INVALID_PARAMETER)dead=true;
    if(process)CloseHandle(process);
    if(dead){api_->destroy_session(old->second);choices_.erase(old->first);predictions_.erase(old->first);paging_.erase(old->first);old=sessions_.erase(old);}else ++old;
  }
  if(sessions_.size()>=256) throw std::runtime_error("Too many sessions");
  auto id=api_->create_session();
  if(!id || !api_->select_schema(id,"chaoxipinyin")) throw std::runtime_error("Session failed");
  sessions_[client]=id; return id;
}
Response Engine::handle(const Request& q) {
  Response r;
  if(q.magic!=kMagic)return r;
  // IPC strings must be terminated before any conversion or filesystem operation.
  if(!wmemchr(q.text,0,1024)||!wmemchr(q.code,0,128)||!wmemchr(q.previousWord,0,64)||!wmemchr(q.previousCode,0,128))return r;
  auto replace=[&](Profile next){
    auto old=profile_;bool rebuild=((old.options^next.options)&OptCompletion)||old.fuzzy!=next.fuzzy;
    profile_=std::move(next);
    try {if(rebuild){stop();start();}save();}
    catch(...){profile_=std::move(old);if(rebuild){stop();start();}throw;}
    resetSessions();storageError_=false;
  };
  if(q.op==Op::Configure) {
    if(q.limit!=4&&q.limit!=8&&q.limit!=16)return r;
    auto next=profile_;next.options=q.value&StoredOptions;next.learning=(q.value&OptLearning)!=0;next.font=std::clamp(q.key,10,24);next.fuzzy=q.mask&127;next.limit=q.limit;replace(next);
  }else if(q.op==Op::Learning){auto next=profile_;next.learning=q.value!=0;replace(next);}
  else if(q.op==Op::Font){auto next=profile_;next.font=std::clamp(q.value,10,24);replace(next);}
  else if(q.op==Op::Erase){auto next=profile_;next.words.clear();fs::remove(home_/L"words.tsv");replace(next);}
  else if(q.op==Op::Backup) {
    fs::path path(q.text);if(!path.is_absolute()||fs::weakly_canonical(path)==fs::weakly_canonical(home_/L"profile.tsv"))return r;
    writeProfileFile(path,profile_.encode());
  }else if(q.op==Op::Restore){fs::path path(q.text);if(!path.is_absolute())return r;replace(Profile::decode(readProfileFile(path)));}
  else if(q.op==Op::PhrasePut||q.op==Op::PhraseDelete) {
    auto next=profile_;std::wstring word(q.text);if(!chineseWord(word))return r;auto code=phraseCode(q.code);auto item=std::make_pair(code,word);
    if(q.op==Op::PhrasePut){if(*q.previousWord&&!next.phrases.erase(std::make_pair(phraseCode(q.previousCode),std::wstring(q.previousWord))))return r;
      if(next.phrases.size()>=4096&&!next.phrases.count(item))return r;next.phrases[item]=0;}
    else if(!next.phrases.erase(item))return r;replace(next);
  }else if(q.op==Op::ImportPreview){
    importOwner_=0;pendingImport_={};fs::path path(q.text);if(!q.client||!path.is_absolute())return r;
    auto parsed=DictionaryImport::parse(readDictionaryFile(path));
    for(auto it=parsed.phrases.begin();it!=parsed.phrases.end();){if(profile_.phrases.count(it->first)||lexicon_.contains(it->first)){++parsed.duplicates;it=parsed.phrases.erase(it);}else ++it;}
    r.importNew=(int)parsed.phrases.size();r.importDuplicate=parsed.duplicates;r.importInvalid=parsed.invalid;
    pendingImport_=std::move(parsed);importOwner_=q.client;importTime_=GetTickCount64();
  }else if(q.op==Op::ImportCancel){if(q.client==importOwner_){pendingImport_={};importOwner_=0;}}
  else if(q.op==Op::ImportCommit){
    if(!q.client||q.client!=importOwner_||GetTickCount64()-importTime_>300000)return r;
    auto next=profile_;int added=0;for(auto& item:pendingImport_.phrases)if(next.phrases.emplace(item).second)++added;
    if(next.phrases.size()>4096)return r;replace(std::move(next));r.importNew=added;pendingImport_={};importOwner_=0;
  }else if(q.op==Op::PhraseList) {
    if(q.key<0||q.key>=(int)profile_.phrases.size()) {if(!profile_.phrases.empty())return r;}
    else {auto it=profile_.phrases.begin();std::advance(it,q.key);for(;it!=profile_.phrases.end()&&r.count<9;++it){copyText(r.codes[r.count],wide(it->first.first.c_str()));copyText(r.candidates[r.count++],it->first.second);}}
  }else if(q.op==Op::Destroy) {
    auto it=sessions_.find(q.client);if(it!=sessions_.end()){api_->destroy_session(it->second);sessions_.erase(it);}choices_.erase(q.client);predictions_.erase(q.client);paging_.erase(q.client);
  }else if(q.op==Op::Clear||q.op==Op::Key) {
    auto id=session(q.client);auto& choices=choices_[q.client];auto& suggestions=predictions_[q.client];auto& paging=paging_[q.client];
    std::wstring committed,punctuation;bool selectedDirect=false;
    if(q.op==Op::Clear){api_->clear_composition(id);choices.clear();suggestions.clear();paging=Paging{};}
    else {
      int key=q.key;int selected=key==' '?0:(key>='1'&&key<='9'?key-'1':-1);
      bool next=key=='='||key==0xff56,previous=key=='-'||key==0xff55;
      if(paging.rimePage==0&&((next&&paging.page+1<paging.pages)||(previous&&paging.page>0))){paging.page+=next?1:-1;selectedDirect=true;r.eaten=1;}
      else if(!next&&!previous)paging.page=0;
      if(!suggestions.empty()){
        if(selected>=0&&selected<(int)suggestions.size()){committed=suggestions[selected];selectedDirect=true;r.eaten=1;}
        else if(key==0xff1b){selectedDirect=true;r.eaten=1;}
        suggestions.clear();
      }
      if(!selectedDirect){
        const char* input=api_->get_input(id);
        const std::string symbols=",.;:!?()[]/\\<>_";
        const std::wstring chinese=L"，。；：！？（）【】、\\《》—";
        auto pos=key>=32&&key<=126?symbols.find((char)key):std::string::npos;
        auto choose=[&](int n){auto choice=choices[n];if(choice.index<0){committed=choice.text;api_->clear_composition(id);}else api_->select_candidate_on_current_page(id,choice.index);};
        if(input&&*input&&pos!=std::string::npos){if(!choices.empty())choose(0);else api_->commit_composition(id);}
        if(pos!=std::string::npos){punctuation=(profile_.options&OptAsciiPunct)?std::wstring(1,(wchar_t)key):std::wstring(1,chinese[pos]);r.eaten=1;}
        else if(selected>=0&&selected<(int)choices.size()&&input&&*input){choose(selected);r.eaten=1;}
        else if(input&&strlen(input)>=120&&key>=32&&key<=126&&key!=' ')r.eaten=1;
        else r.eaten=api_->process_key(id,key,q.mask);
      }
    }
    RIME_STRUCT(RimeCommit,c);
    if(api_->get_commit(id,&c)){committed+=wide(c.text);api_->free_commit(&c);}
    if(profile_.learning&&chineseWord(committed,profile_.limit)){
      if(profile_.words.size()<8192||profile_.words.count(committed)){
        auto& count=profile_.words[committed];count=std::min<uint64_t>(count+1,1000000);
        try{save();storageError_=false;}catch(...){storageError_=true;}
      }
    }
    copyText(r.commit,committed+punctuation);
    RIME_STRUCT(RimeContext,cx);
    if(api_->get_context(id,&cx)) {
      copyText(r.preedit,wide(cx.composition.preedit));r.page=cx.menu.page_no;r.last=cx.menu.is_last_page;r.highlight=0;
      if(r.page==0&&paging.rimePage==1&&(q.key=='-'||q.key==0xff55))paging.page=std::max(0,paging.pages-1);
      paging.rimePage=r.page;
      choices.clear();for(int i=0;i<std::min(9,cx.menu.num_candidates);++i)choices.push_back({i,wide(cx.menu.candidates[i].text)});
      if(profile_.learning){
        // Keep Rime's positions for complete sentences versus partial segments.
        // Learned frequency only rearranges equal-length alternatives in their
        // existing slots; a frequent single character cannot jump over a phrase.
        std::map<size_t,std::vector<size_t>> slots;
        for(size_t i=0;i<choices.size();++i)slots[choices[i].text.size()].push_back(i);
        for(auto& [length,indices]:slots){std::vector<Choice> group;for(auto i:indices)group.push_back(choices[i]);
          auto score=[&](const Choice& c){auto it=profile_.words.find(c.text);return it==profile_.words.end()?uint64_t(0):it->second;};
          std::stable_sort(group.begin(),group.end(),[&](auto& a,auto& b){return score(a)>score(b);});
          for(size_t i=0;i<indices.size();++i)choices[indices[i]]=group[i];}
      }
      api_->free_context(&cx);
      auto raw=api_->get_input(id);std::string code;
      if(raw&&*raw){try{code=phraseCode(wide(raw));}catch(...){} }
      if(!code.empty()&&r.page==0){
        std::vector<Choice> extra;
        auto add=[&](const std::wstring& text){if(std::none_of(extra.begin(),extra.end(),[&](auto& c){return c.text==text;}))extra.push_back({-1,text});};
        for(auto& [item,unused]:profile_.phrases)if(item.first==code)add(item.second);
        // Preserve the engine's first candidate. Corrections are suggestions below
        // it; only explicit personal entries can take the first position.
        auto personal=extra.size();
        if(profile_.options&OptCorrection){auto corrected=lexicon_.corrections(code);for(auto& text:corrected)add(text);}
        std::vector<Choice> merged;
        auto append=[&](const Choice& c){if(std::none_of(merged.begin(),merged.end(),[&](auto& x){return x.text==c.text;}))merged.push_back(c);};
        for(size_t i=0;i<personal;++i)append(extra[i]);
        if(!choices.empty())append(choices[0]);
        // Leave normal alternatives available as well as correction candidates.
        for(size_t i=personal;i<extra.size()&&i<personal+3;++i)append(extra[i]);
        for(auto& choice:choices)append(choice);choices=std::move(merged);
      }
      if(raw&&r.page==0&&EnglishLexicon::letters(raw)){
        std::vector<Choice> english;english.push_back({-1,wide(raw)});
        if(profile_.options&OptCompletion)for(auto& word:english_.complete(raw))english.push_back({-1,word});
        // Keep the current Chinese/personal first choice and all native choices.
        // English choices share the existing direct-selection and extra-page path.
        size_t at=choices.empty()?0:1;
        for(auto& choice:english)if(std::none_of(choices.begin(),choices.end(),[&](const auto& c){return c.text==choice.text;})){
          choices.insert(choices.begin()+at,choice);++at;
        }
      }
      if(r.page==0){paging.pages=std::max(1,((int)choices.size()+8)/9);paging.page=std::clamp(paging.page,0,paging.pages-1);
        int first=paging.page*9,last=std::min(first+9,(int)choices.size());choices=std::vector<Choice>(choices.begin()+first,choices.begin()+last);
        r.page=paging.page;r.last=r.last&&paging.page+1==paging.pages;
      }else r.page+=paging.pages-1;
      r.count=(int)choices.size();for(int i=0;i<r.count;++i)copyText(r.candidates[i],choices[i].text);
    }
    if(q.op==Op::Key&&!committed.empty()&&punctuation.empty()&&!*r.preedit)predict(q.client,committed,r);
  }
  r.learning=profile_.learning;r.font=profile_.font;r.options=profile_.options;r.fuzzy=profile_.fuzzy;r.limit=profile_.limit;
  r.phraseCount=(int)profile_.phrases.size();r.storageError=storageError_;r.ok=1;return r;
}
