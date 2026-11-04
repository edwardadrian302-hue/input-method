#include "engine.hpp"
#include <iostream>
#include <fstream>
#include <memory>
static int checks=0;
static void require(bool good,const char* message){if(!good)throw std::runtime_error(message);++checks;}
int wmain(){
  try{
    auto root=moduleDir();auto home=root.parent_path()/L"test-output"/(L"run-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    auto engine=std::make_unique<Engine>(root,home);
    auto op=[&](Op op,int value=0,uint64_t client=1){Request q;q.op=op;q.value=value;q.client=client;return engine->handle(q);};
    auto key=[&](int k,uint64_t client=1){Request q;q.op=Op::Key;q.key=k;q.client=client;return engine->handle(q);};
    auto type=[&](const char* input,uint64_t client=1){op(Op::Clear,0,client);Response r;for(auto p=input;*p;++p)r=key(*p,client);return r;};
    require(op(Op::Status).learning==1,"learning defaults on");
    auto r=type("xu");auto beforeE=std::wstring(r.preedit);r=key('e');
    require(r.eaten && std::wstring(r.preedit)!=beforeE && std::wstring(r.preedit).find(L"xue")!=std::wstring::npos,"third letter e updates xu to xue");
    bool hasXue=false;for(int page=0;page<10;++page){for(int i=0;i<r.count;++i)if(std::wstring(r.candidates[i])==L"学")hasXue=true;if(hasXue||r.last)break;r=key('=');}
    require(hasXue,"xue offers the character xue");
    r=type("daxuefengshan");require(r.count>0 && *r.preedit,"multi-syllable full pinyin keeps responding");
    r=type("nihao");require(r.count>0 && std::wstring(r.candidates[0])==L"你好","full pinyin nihao -> ni hao");
    require(std::wstring(key(' ').commit)==L"你好","space commits first candidate");
    r=type("zhongguo");require(std::wstring(r.candidates[0])==L"中国","full pinyin zhongguo -> zhong guo");
    r=type("beijing");require(std::wstring(r.candidates[0])==L"北京","full pinyin beijing");
    r=type("shi");require(r.count==9,"nine candidates per page");
    auto ninth=std::wstring(r.candidates[8]);require(std::wstring(key('9').commit)==ninth,"number nine selects ninth displayed item");
    r=type("shi");require(std::wstring(r.candidates[0])==ninth,"local word frequency promotes chosen candidate");
    auto learned=home/L"words.tsv";require(fs::exists(learned),"local learning file created");
    op(Op::Learning,0);r=type("shi");require(std::wstring(r.candidates[0])!=ninth,"disabled learning ignores stored rankings");
    auto before=fs::last_write_time(learned);key('1');require(fs::last_write_time(learned)==before,"disabled learning does not write data");
    op(Op::Learning,1);r=type("shi");require(std::wstring(r.candidates[0])==ninth,"re-enable restores learned ranking");
    op(Op::Erase);require(fs::file_size(learned)==0,"erase removes all learned words");
    r=type("shi");require(std::wstring(r.candidates[0])!=ninth,"erase resets ranking");
    auto page=key('=');require(page.page==1 && page.count>0,"candidate next page");
    require(key('-').page==0,"candidate previous page");
    r=type("nihao");require(*key(0xff08).preedit,"backspace edits preedit");
    require(!*key(0xff1b).preedit,"escape clears composition");
    require(std::wstring(key(',').commit)==L"，","Chinese punctuation");
    type("nihao",1);r=type("zhongguo",2);require(std::wstring(key(' ',1).commit)==L"你好","independent window sessions");
    require(std::wstring(key(' ',2).commit)==L"中国","second window session isolated");
    op(Op::Erase);type("woxihuannimen");key(0xff0d);require(fs::file_size(learned)==0,"raw Latin commits never learned");
    bool userdb=false;for(auto& entry:fs::recursive_directory_iterator(home))if(entry.path().wstring().find(L"userdb")!=std::wstring::npos)userdb=true;
    require(!userdb,"Rime user databases always disabled");
    op(Op::Erase);r=type("nihaozhongguoren");require(wcslen(r.candidates[0])>4,"long sentence fixture");key(' ');require(fs::file_size(learned)==0,"long Chinese sentences never learned");
    type("nihao");key(' ');op(Op::Learning,0);engine.reset();engine=std::make_unique<Engine>(root,home);
    require(op(Op::Status).learning==0,"learning switch persists across restarts");
    auto persisted=fs::file_size(learned);require(persisted>0,"short word survives restart");
    op(Op::Learning,1);type("zhongguo");
    HANDLE locked=CreateFileW(learned.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);require(locked!=INVALID_HANDLE_VALUE,"lock learning file for failure test");
    r=key(' ');CloseHandle(locked);require(std::wstring(r.commit)==L"中国" && r.storageError,"storage failure never loses committed text");
    std::cout<<"PASS: "<<checks<<" engine checks. Test data: "<<utf8(home.wstring())<<"\n";return 0;
  }catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}
}
