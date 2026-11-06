#include "engine.hpp"
#include <iostream>
#include <fstream>
#include <memory>
static int checks=0;
static void require(bool good,const char* message){if(!good)throw std::runtime_error(message);++checks;}
int wmain(){
  try{
    auto root=moduleDir().parent_path().parent_path()/L"dist";auto home=root.parent_path()/L"test-output"/(L"run-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    auto migrationRoot=home/L"migration";auto old=migrationRoot/L"old";auto target=migrationRoot/L"new";
    fs::create_directories(old);Profile migrationProfile;migrationProfile.words[L"你好"]=12;
    migrationProfile.phrases[{"chaoxipinyin",L"潮汐拼音"}]=0;
    writeProfileFile(old/L"profile.tsv",migrationProfile.encode());
    migrateLegacyProfile(target,old);
    require(readProfileFile(target/L"profile.tsv")==migrationProfile.encode(),"migration preserves settings, learned words and personal phrases byte for byte");
    require(readProfileFile(old/L"profile.tsv")==migrationProfile.encode(),"migration preserves migrationProfile profile");
    Profile changed;changed.words[L"世界"]=7;writeProfileFile(target/L"profile.tsv",changed.encode());
    migrateLegacyProfile(target,old);
    require(readProfileFile(target/L"profile.tsv")==changed.encode(),"migration never overwrites existing destination");
    writeProfileFile(old/L"profile.tsv","broken");bool badMigration=false;
    try{migrateLegacyProfile(migrationRoot/L"invalid",old);}catch(...){badMigration=true;}
    require(badMigration&&!fs::exists(migrationRoot/L"invalid"/L"profile.tsv"),"invalid source cannot create a new profile");
    migrateLegacyProfile(migrationRoot/L"empty",migrationRoot/L"missing");
    require(!fs::exists(migrationRoot/L"empty"/L"profile.tsv"),"fresh installation does not invent a profile");
    auto engine=std::make_unique<Engine>(root,home);
    auto op=[&](Op op,int value=0,uint64_t client=1){Request q;q.op=op;q.value=value;q.client=client;return engine->handle(q);};
    auto key=[&](int k,uint64_t client=1){Request q;q.op=Op::Key;q.key=k;q.client=client;return engine->handle(q);};
    auto type=[&](const char* input,uint64_t client=1){op(Op::Clear,0,client);Response r;for(auto p=input;*p;++p)r=key(*p,client);return r;};
    require(op(Op::Status).learning==1,"learning defaults on");
    auto configure=[&](int options,int fuzzy=0,int font=12,int limit=4){Request q;q.op=Op::Configure;q.value=options;q.mask=fuzzy;q.key=font;q.limit=limit;return engine->handle(q);};
    auto contains=[&](const Response& r,const wchar_t* word){for(int i=0;i<r.count;++i)if(std::wstring(r.candidates[i])==word)return true;return false;};
    require(op(Op::Status).font==12,"compact default font is 12 points");
    auto fixture=type("qishangbaxia");require(std::wstring(fixture.candidates[0])==L"七上八下","common idiom ranks first");
    require(contains(type("shenm"),L"什么"),"unfinished full pinyin shenm offers shen me");
    require(contains(type("zenm"),L"怎么"),"unfinished full pinyin zenm offers zen me");
    require(contains(type("sm"),L"什么"),"initial abbreviation sm offers shen me");
    op(Op::Clear);
    require(std::wstring(key(';').commit)==L"；","idle semicolon emits Chinese semicolon");
    require(std::wstring(key('.').commit)==L"。","idle period emits Chinese period");
    type("nihao");require(std::wstring(key(',').commit)==L"你好，","comma commits selected word followed by punctuation");
    type("nihao");require(std::wstring(key(';').commit)==L"你好；","semicolon commits selected word followed by punctuation");
    configure(OptCompletion|OptAsciiPunct|OptLearning);
    require(std::wstring(key(',').commit)==L",","English punctuation option emits ASCII comma");
    require(std::wstring(key(';').commit)==L";","English punctuation option emits ASCII semicolon");
    type("nihao");require(std::wstring(key(';').commit)==L"你好;","English punctuation option preserves composition");
    configure(OptCompletion|OptLearning);

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
    auto learned=home/L"profile.tsv";
    auto learnedWords=[&](){return Profile::decode(readProfileFile(learned)).words;};require(fs::exists(learned),"local learning file created");
    op(Op::Learning,0);r=type("shi");require(std::wstring(r.candidates[0])!=ninth,"disabled learning ignores stored rankings");
    auto before=fs::last_write_time(learned);key('1');require(fs::last_write_time(learned)==before,"disabled learning does not write data");
    op(Op::Learning,1);r=type("shi");require(std::wstring(r.candidates[0])==ninth,"re-enable restores learned ranking");
    op(Op::Erase);require(learnedWords().empty(),"erase removes all learned words");
    r=type("shi");require(std::wstring(r.candidates[0])!=ninth,"erase resets ranking");
    auto page=key('=');require(page.page==1 && page.count>0,"candidate next page");
    require(key('-').page==0,"candidate previous page");
    r=type("nihao");require(*key(0xff08).preedit,"backspace edits preedit");
    require(!*key(0xff1b).preedit,"escape clears composition");
    require(std::wstring(key(',').commit)==L"，","Chinese punctuation");
    type("nihao",1);r=type("zhongguo",2);require(std::wstring(key(' ',1).commit)==L"你好","independent window sessions");
    require(std::wstring(key(' ',2).commit)==L"中国","second window session isolated");
    op(Op::Erase);
    for(const char* raw:{"hello","english","nihao","woxihuannimen","xi'an"}){
      type(raw);auto literal=key(0xff0d);
      require(literal.eaten&&std::wstring(literal.commit)==wide(raw)&&!*literal.preedit&&literal.count==0,"Return commits original Latin input without syllable formatting");
    }
    require(learnedWords().empty(),"raw Latin commits never learned");
    engine->suppressLearning(true);type("beijing");key(' ');
    require(learnedWords().empty(),"temporary private input suppresses learning");
    require(op(Op::Status).learning==1,"private input does not change saved learning preference");
    engine->suppressLearning(false);type("beijing");key(' ');
    require(learnedWords().count(L"北京")==1,"learning resumes after private input");op(Op::Erase);

    bool userdb=false;for(auto& entry:fs::recursive_directory_iterator(home))if(entry.path().wstring().find(L"userdb")!=std::wstring::npos)userdb=true;
    require(!userdb,"Rime user databases always disabled");
    op(Op::Erase);r=type("nihaozhongguoren");require(wcslen(r.candidates[0])>4,"long sentence fixture");key(' ');require(learnedWords().empty(),"default limit excludes long Chinese sentences");
    type("nihao");key(' ');op(Op::Learning,0);engine.reset();engine=std::make_unique<Engine>(root,home);
    require(op(Op::Status).learning==0,"learning switch persists across restarts");
    auto persisted=fs::file_size(learned);require(persisted>0,"short word survives restart");
    op(Op::Learning,1);type("zhongguo");
    HANDLE locked=CreateFileW(learned.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);require(locked!=INVALID_HANDLE_VALUE,"lock learning file for failure test");
    r=key(' ');CloseHandle(locked);require(std::wstring(r.commit)==L"中国" && r.storageError,"storage failure never loses committed text");
    op(Op::Clear);
    configure(OptLearning);
    require(!contains(type("shenm"),L"什么"),"turning off completion disables mixed unfinished spelling");
    configure(OptCompletion|OptLearning,FuzzyS);
    require(contains(type("senme"),L"什么"),"s/sh fuzzy switch changes recognition");
    configure(OptCompletion|OptLearning,0);
    require(!contains(type("senme"),L"什么"),"disabling fuzzy restores exact recognition");
    configure(OptCompletion|OptLearning,FuzzyZ|FuzzyC|FuzzyS|FuzzyNL|FuzzyEn|FuzzyAn|FuzzyIn,14);
    require(contains(type("zidao"),L"知道"),"z/zh fuzzy recognition");
    require(contains(type("cifan"),L"吃饭"),"c/ch fuzzy recognition");
    require(contains(type("penyou"),L"朋友"),"en/eng fuzzy recognition");
    require(contains(type("banggong"),L"办公"),"an/ang fuzzy recognition");
    require(contains(type("mintian"),L"明天"),"in/ing fuzzy recognition");
    require(contains(type("lihao"),L"你好"),"n/l fuzzy recognition");
    engine.reset();engine=std::make_unique<Engine>(root,home);
    require(op(Op::Status).fuzzy==127 && op(Op::Status).font==14,"visual settings persist across restart");
    configure(OptCompletion|OptLearning,0);
    // New features are isolated from the installed user's data.
    require(op(Op::Status).limit==4,"learning length remains four by default");
    auto phrase=[&](Op action,const wchar_t* code,const wchar_t* word,const wchar_t* previousCode=L"",const wchar_t* previousWord=L""){
      Request q;q.op=action;copyText(q.code,code);copyText(q.text,word);copyText(q.previousCode,previousCode);copyText(q.previousWord,previousWord);return engine->handle(q);
    };
    require(!phrase(Op::PhrasePut,L"abc",L"包含123").ok,"personal phrases reject non-Chinese text");
    require(phrase(Op::PhrasePut,L"chao xi pin yin",L"潮汐拼音").ok,"add a personal phrase with spaced pinyin");
    r=type("chaoxipinyin");require(std::wstring(r.candidates[0])==L"潮汐拼音","personal full-pinyin phrase is first");
    require(std::wstring(key('1').commit)==L"潮汐拼音","personal candidate number commits exact text");
    op(Op::Learning,0);r=type("chaoxipinyin");require(std::wstring(key(' ').commit)==L"潮汐拼音","manual phrases work with automatic learning off");
    op(Op::Erase);require(op(Op::Status).phraseCount==1,"clearing learned words preserves manual phrases");
    phrase(Op::PhrasePut,L"chao xi shu ru fa",L"潮汐输入法",L"chaoxipinyin",L"潮汐拼音");
    require(op(Op::Status).phraseCount==1,"editing a phrase replaces its previous identity atomically");
    r=type("chaoxishurufa");require(std::wstring(key(',').commit)==L"潮汐输入法，","personal phrase punctuation commit");
    Request list;list.op=Op::PhraseList;r=engine->handle(list);require(r.count==1&&std::wstring(r.codes[0])==L"chaoxishurufa","list returns normalized pinyin and text");
    require(phrase(Op::PhraseDelete,L"chaoxishurufa",L"潮汐输入法").ok&&op(Op::Status).phraseCount==0,"delete personal phrase");
    phrase(Op::PhrasePut,L"chao xi pin yin ce shi",L"潮汐拼音测试");
    configure(OptCompletion|OptLearning,0,12,8);type("chaoxipinyinceshi");key(' ');
    require(learnedWords().count(L"潮汐拼音测试"),"eight-character learning option learns a six-character phrase");
    configure(OptCompletion|OptLearning,0,12,4);op(Op::Erase);type("chaoxipinyinceshi");key(' ');
    require(learnedWords().empty(),"four-character option still excludes six-character phrase");
    configure(OptCompletion|OptLearning|OptCorrection);
    r=type("nihso");require(contains(r,L"你好"),"one-key replacement typo suggests nihao");
    int correction=-1;for(int i=0;i<r.count;++i)if(std::wstring(r.candidates[i])==L"你好")correction=i;
    require(correction>=0&&std::wstring(key('1'+correction).commit)==L"你好","correction numeric selection matches displayed text");
    require(contains(type("nihoa"),L"你好"),"adjacent transposition typo suggests nihao");
    require(contains(type("nihaao"),L"你好"),"extra-key typo suggests nihao");
    require(contains(type("niho"),L"你好"),"missing-key typo suggests nihao");
    configure(OptCompletion|OptLearning);require(!contains(type("nihso"),L"你好"),"correction switch disables typo suggestions");
    configure(OptCompletion|OptLearning|OptCorrection);require(std::wstring(type("nihao").candidates[0])==L"你好","correction preserves exact first candidate");
    configure(OptCompletion|OptLearning|OptPrediction);
    type("zhongguo");r=key(' ');require(r.prediction&&r.count>0&&!*r.preedit&&std::wstring(r.commit)==L"中国","prediction offers suffixes without a composition");
    auto suffix=std::wstring(r.candidates[0]);require(std::wstring(key(' ').commit)==suffix,"prediction appends suffix only");
    type("zhongguo");key(' ');r=key(0xff1b);require(!r.prediction&&!*r.preedit&&!*r.commit&&r.eaten,"Escape dismisses prediction without deleting committed text");
    type("zhongguo");key(' ');r=key('n');require(!r.prediction&&*r.preedit&&!*r.commit,"typing letters starts next composition instead of accepting prediction");
    op(Op::Clear);r=key(' ');require(!r.prediction&&!*r.commit,"clearing session removes stale predictions");
    type("zhongguo",1);key(' ',1);require(!key(' ',2).prediction,"prediction sessions are isolated");
    type("zhongguo");r=key(',');require(!r.prediction,"punctuation suppresses next-word prediction");
    configure(OptCompletion|OptLearning);type("zhongguo");require(!key(' ').prediction,"prediction switch disables suggestions");
    configure(OptCompletion|OptLearning|OptPrediction|OptCorrection,FuzzyS,16,16);
    type("nihao");key(' ');auto saved=Profile::decode(readProfileFile(learned));
    Request backup;backup.op=Op::Backup;auto backupPath=home/L"roundtrip.cxbackup";copyText(backup.text,backupPath.wstring());
    require(engine->handle(backup).ok,"export backup");require(readProfileFile(backupPath)==saved.encode(),"backup contains complete profile snapshot");
    phrase(Op::PhraseDelete,L"chaoxipinyinceshi",L"潮汐拼音测试");configure(OptCompletion);op(Op::Erase);
    Request restore;restore.op=Op::Restore;copyText(restore.text,backupPath.wstring());require(engine->handle(restore).ok,"restore backup");
    require(readProfileFile(learned)==saved.encode(),"settings frequencies and manual phrases round trip exactly");
    require(op(Op::Status).limit==16&&op(Op::Status).font==16&&op(Op::Status).phraseCount==1,"restored settings active");
    auto bad=home/L"bad.cxbackup";std::ofstream(bad)<<"CHAOXI_BACKUP\t99\n";copyText(restore.text,bad.wstring());bool rejected=false;
    try{engine->handle(restore);}catch(...){rejected=true;}require(rejected&&readProfileFile(learned)==saved.encode(),"invalid backup leaves current profile intact");
    locked=CreateFileW(learned.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);require(locked!=INVALID_HANDLE_VALUE,"lock profile for restore failure");
    copyText(restore.text,backupPath.wstring());rejected=false;try{engine->handle(restore);}catch(...){rejected=true;}CloseHandle(locked);
    require(rejected&&readProfileFile(learned)==saved.encode()&&op(Op::Status).phraseCount==1,"failed restore preserves disk and active profile");
    engine.reset();engine=std::make_unique<Engine>(root,home);require(op(Op::Status).limit==16&&op(Op::Status).phraseCount==1,"expanded settings and personal phrases persist across restart");
    // English selections are local candidates and must leave Chinese mode/learning intact.
    configure(OptCompletion|OptLearning|OptPrediction);op(Op::Erase);
    auto selectText=[&](const char* input,const wchar_t* text){auto candidates=type(input);int found=-1;
      for(int i=0;i<candidates.count;++i)if(std::wstring(candidates.candidates[i])==text)found=i;
      require(found>=0,"English candidate appears on the first page");return key('1'+found);};
    for(const char* input:{"build","hello","nihao","qzqz"}){
      auto result=selectText(input,wide(input).c_str());
      require(std::wstring(result.commit)==wide(input)&&!*result.preedit&&!result.count&&!result.prediction,"numeric English selection commits exactly once without prediction");
    }
    r=type("nihao");require(std::wstring(r.candidates[0])==L"你好"&&std::wstring(r.candidates[1])==L"nihao","Chinese first choice is followed by raw English input");
    require(std::wstring(key(' ').commit)==L"你好","English candidate preserves Chinese space selection");op(Op::Erase);
    r=type("buil");require(contains(r,L"buil")&&contains(r,L"build"),"prefix offers both raw input and complete English word");
    int englishCount=0;for(int i=0;i<r.count;++i)if(EnglishLexicon::letters(utf8(r.candidates[i])))++englishCount;
    require(englishCount<=4,"English suggestions are limited to raw input and three completions");
    auto completed=selectText("buil",L"build");require(std::wstring(completed.commit)==L"build"&&!*completed.preedit&&!completed.prediction,"numeric completion commits full English word");
    require(learnedWords().empty(),"English raw/completed selections are never learned as Chinese words");
    require(!contains(type("bu"),L"build"),"short prefixes do not trigger English completion");
    configure(OptLearning);r=type("buil");require(contains(r,L"buil")&&!contains(r,L"build"),"completion switch disables English expansion but preserves raw candidate");
    configure(OptCompletion|OptLearning);type("build");key(0xff08);r=key(0xff08);
    require(contains(r,L"bui")&&!contains(r,L"buil"),"backspace refreshes raw input and removes stale raw candidate");
    type("build");r=key(0xff1b);require(!r.count&&!*r.commit&&!*r.preedit,"Escape removes English candidates without committing");
    require(!contains(type("xi'an"),L"xi'an"),"pinyin separators are not treated as English words");
    r=type("nihao");int duplicates=0;for(int i=0;i<r.count;++i)if(std::wstring(r.candidates[i])==L"nihao")++duplicates;
    require(duplicates==1,"raw candidate appears once");
    // Extra candidates have their own pages; none of the original first nine are lost.
    configure(OptCompletion|OptLearning);auto original=type("nihao");std::vector<std::wstring> baseline;
    for(int i=0;i<original.count;++i)baseline.push_back(original.candidates[i]);
    const wchar_t* custom[]={L"你好甲",L"你好乙",L"你好丙",L"你好丁",L"你好戊",L"你好己",L"你好庚",L"你好辛",L"你好壬",L"你好癸"};
    for(auto text:custom)phrase(Op::PhrasePut,L"nihao",text);
    r=type("nihao");require(r.count==9&&!r.last,"personal candidates spill into extra nine-item pages");
    std::vector<std::wstring> seen;for(int pageIndex=0;pageIndex<3;++pageIndex){for(int i=0;i<r.count;++i)seen.push_back(r.candidates[i]);if(r.last)break;r=key('=');}
    bool preserved=true;for(auto& text:baseline)if(std::find(seen.begin(),seen.end(),text)==seen.end())preserved=false;
    for(auto text:custom)if(std::find(seen.begin(),seen.end(),text)==seen.end())preserved=false;
    require(preserved,"pagination preserves manual phrases and all original page candidates");
    r=key('-');require(r.count>0,"previous page returns from native page to extra page");
    auto chosen=std::wstring(r.candidates[0]);auto selectedResult=key('1');
    require(std::wstring(selectedResult.commit)==chosen||(*selectedResult.preedit&&!*selectedResult.commit),"paged candidate selects matching text or partial segment");
    Request badBackup;badBackup.op=Op::Restore;copyText(badBackup.text,bad.wstring());
    std::ofstream(bad,std::ios::binary)<<"CHAOXI_BACKUP\t1\nSET\t1\t12\t1\t0\t4\n";
    auto intact=readProfileFile(learned);rejected=false;try{engine->handle(badBackup);}catch(...){rejected=true;}
    require(rejected&&readProfileFile(learned)==intact,"truncated backup without end marker is rejected");
    std::ofstream(bad,std::ios::binary)<<"CHAOXI_BACKUP\t1\nSET\t1\t12\t1\t0\t4\nWORD\t"<<utf8(L"你好")<<"\t1\nWORD\t"<<utf8(L"你好")<<"\t2\nEND\t1\t0\n";
    rejected=false;try{engine->handle(badBackup);}catch(...){rejected=true;}require(rejected&&readProfileFile(learned)==intact,"duplicate backup record is rejected without changing profile");
    // Frequently selected short words must not displace full-input candidates.
    configure(OptCompletion|OptLearning);type("ren");key(' ');
    auto rankProfile=Profile::decode(readProfileFile(learned));rankProfile.words[L"人"]=1000;
    auto rankPath=home/L"ranking.cxbackup";writeProfileFile(rankPath,rankProfile.encode());Request rankRestore;rankRestore.op=Op::Restore;copyText(rankRestore.text,rankPath.wstring());engine->handle(rankRestore);
    auto ranking=type("renshengchanghen");
    std::cout<<"Ranking sample: ";for(int i=0;i<ranking.count;++i)std::cout<<utf8(ranking.candidates[i])<<" | ";std::cout<<"\n";
    require(std::wstring(ranking.candidates[0])==L"人生长恨","learned single character cannot outrank four-syllable sentence");
    require(std::wstring(key(' ').commit)==L"人生长恨","space commits complete four-syllable sentence");
    // Import previews own an immutable snapshot; only confirmation can save it.
    Request builtIn;builtIn.op=Op::ImportPreview;builtIn.client=899;copyText(builtIn.text,(root/L"data"/L"cn_dicts"/L"ext.dict.yaml").wstring());
    auto builtInResult=engine->handle(builtIn);require(builtInResult.ok&&builtInResult.importNew==0&&builtInResult.importDuplicate>300000,"full bundled dictionary preview filters all existing entries without consuming personal capacity");
    auto dictionary=home/L"import.dict.yaml";
    auto source=std::string("---\nname: test\n...\n")+utf8(L"潮汐批量词库\tchao xi pi liang ci ku\t100\n潮汐批量词库\tchao xi pi liang ci ku\t100\n你好\tni hao\t100\ninvalid\tnot pinyin\n");
    writeProfileFile(dictionary,source);Request preview;preview.op=Op::ImportPreview;preview.client=900;copyText(preview.text,dictionary.wstring());
    auto beforeImport=readProfileFile(learned);r=engine->handle(preview);
    require(r.ok&&r.importNew==1&&r.importDuplicate==2&&r.importInvalid==1,"preview filters builtin and duplicate entries and counts invalid rows");
    require(readProfileFile(learned)==beforeImport,"preview never writes profile");
    Request confirm;confirm.op=Op::ImportCommit;confirm.client=901;require(!engine->handle(confirm).ok,"different client cannot confirm preview");
    writeProfileFile(dictionary,utf8(L"已替换\tyi ti huan\n"));confirm.client=900;
    r=engine->handle(confirm);require(r.ok&&r.importNew==1,"confirmed import uses snapshot rather than changed file");
    require(std::wstring(type("chaoxipiliangciku").candidates[0])==L"潮汐批量词库","imported candidate immediately available");
    require(!engine->handle(confirm).ok,"confirmed snapshot cannot be reused");
    engine->handle(preview);Request cancel;cancel.op=Op::ImportCancel;cancel.client=900;engine->handle(cancel);
    require(!engine->handle(confirm).ok,"cancel drops pending import");
    engine->handle(preview);auto intactImport=readProfileFile(learned);
    locked=CreateFileW(learned.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);rejected=false;
    try{engine->handle(confirm);}catch(...){rejected=true;}CloseHandle(locked);
    require(rejected&&readProfileFile(learned)==intactImport,"failed import rolls back disk and active profile");
    require(engine->handle(confirm).ok,"failed save can retry same confirmed snapshot");
    std::string large;for(int i=0;i<4100;++i)large+=utf8(std::wstring(L"容量")+(wchar_t)(0x4e00+i))+"\trong liang\n";
    writeProfileFile(dictionary,large);r=engine->handle(preview);auto fullBefore=readProfileFile(learned);
    require(r.importNew>4096&&!engine->handle(confirm).ok&&readProfileFile(learned)==fullBefore,"overcapacity import is rejected atomically without partial additions");
    copyText(backup.text,backupPath.wstring());engine->handle(backup);
    require(Profile::decode(readProfileFile(backupPath)).phrases.count({"chaoxipiliangciku",L"潮汐批量词库"}),"imported words are included in normal backup");
    // Legacy 0.3 settings and words are read once, without deleting originals.
    engine.reset();auto legacy=home/L"legacy";fs::create_directories(legacy);std::ofstream(legacy/L"settings.ini")<<"[settings]\nlearning=0\nfont=14\n";
    std::ofstream(legacy/L"words.tsv")<<utf8(L"你好")<<"\t7\n";engine=std::make_unique<Engine>(root,legacy);
    require(op(Op::Status).learning==0&&op(Op::Status).font==14,"legacy preferences migrate");op(Op::Learning,1);
    require(Profile::decode(readProfileFile(legacy/L"profile.tsv")).words.at(L"你好")==7&&fs::exists(legacy/L"words.tsv"),"legacy frequency migrates without deleting original data");
    op(Op::Erase);require(!fs::exists(legacy/L"words.tsv")&&Profile::decode(readProfileFile(legacy/L"profile.tsv")).words.empty(),"explicit clear also deletes legacy learned frequencies");

    // Versioned capacity persistence and backward-compatible legacy decoding.
    auto oldFormat=Profile::decode("CHAOXI_BACKUP\t1\nSET\t1\t12\t1\t0\t4\nWORD\t"+utf8(L"你好")+"\t2\nEND\t1\t0\n");
    require(oldFormat.wordCapacity==8192&&oldFormat.phraseCapacity==4096,"v1 profiles keep historical default capacities");
    Profile expanded;expanded.wordCapacity=9000;expanded.phraseCapacity=6000;
    for(int i=0;i<8200;++i)expanded.words[std::wstring(L"学")+(wchar_t)(0x4e00+i)]=1;
    for(int i=0;i<4200;++i)expanded.phrases[{"ci",std::wstring(L"词")+(wchar_t)(0x4e00+i)}]=0;
    auto expandedAgain=Profile::decode(expanded.encode());
    require(expandedAgain.words.size()==8200&&expandedAgain.phrases.size()==4200&&expandedAgain.wordCapacity==9000&&expandedAgain.phraseCapacity==6000,"v2 round trip exceeds both former fixed limits");
    for(auto limits:{"0\t4096","100001\t4096","8192\t0","8192\t100001"}){
      bool invalid=false;try{Profile::decode(std::string("CHAOXI_BACKUP\t2\nSET\t1\t12\t1\t0\t4\nCAP\t")+limits+"\nEND\t0\t0\n");}catch(...){invalid=true;}
      require(invalid,"invalid persisted capacities rejected");
    }
    engine.reset();auto advanced=home/L"capacity-ninekey";engine=std::make_unique<Engine>(root,advanced);
    Request capacity;capacity.op=Op::Capacity;auto capacities=engine->handle(capacity);
    require(capacities.importNew==8192&&capacities.importDuplicate==4096,"query exposes default capacities without writing");
    capacity.value=1;capacity.limit=2;require(engine->handle(capacity).ok,"small custom capacities accepted");
    type("nihao");key(' ');type("beijing");key(' ');
    require(Profile::decode(readProfileFile(advanced/L"profile.tsv")).words.size()==1,"learning stops adding records at configured capacity");
    type("nihao");key(' ');
    require(Profile::decode(readProfileFile(advanced/L"profile.tsv")).words.at(L"你好")==2,"existing words continue counting when capacity is full");
    capacity.value=9000;capacity.limit=6000;require(engine->handle(capacity).ok,"capacity can be raised without deleting words");
    type("beijing");key(' ');require(Profile::decode(readProfileFile(advanced/L"profile.tsv")).words.count(L"北京")==1,"raising capacity permits new learned words");
    auto safeProfile=readProfileFile(advanced/L"profile.tsv");capacity.value=1;
    require(!engine->handle(capacity).ok&&readProfileFile(advanced/L"profile.tsv")==safeProfile,"capacity below existing records is rejected atomically");
    Request allSettings;allSettings.op=Op::Configure;allSettings.value=OptLearning;allSettings.key=18;allSettings.limit=8;copyText(allSettings.text,L"1 6000");
    require(!engine->handle(allSettings).ok&&engine->handle(Request{}).font==12,"invalid capacity cannot partially apply other settings");
    copyText(allSettings.text,L"9000 6000");require(engine->handle(allSettings).ok,"capacity and other settings apply together");
    configure(OptCompletion|OptLearning);
    engine.reset();engine=std::make_unique<Engine>(root,advanced);capacity.value=0;capacities=engine->handle(capacity);
    require(capacities.importNew==9000&&capacities.importDuplicate==6000,"capacities persist after engine restart");
    preview.client=900;copyText(preview.text,dictionary.wstring());auto overOldLimit=engine->handle(preview);
    require(overOldLimit.importNew>4096&&engine->handle(confirm).ok,"batch import follows raised personal capacity");
    capacity.value=9000;capacity.limit=4096;safeProfile=readProfileFile(advanced/L"profile.tsv");
    require(!engine->handle(capacity).ok&&safeProfile==readProfileFile(advanced/L"profile.tsv"),"cannot shrink personal capacity below imported records");
    auto typeNine=[&](const char* digits){op(Op::Clear);Response state;for(auto p=digits;*p;++p){Request digit;digit.op=Op::NineKey;digit.key=*p;digit.client=1;state=engine->handle(digit);}return state;};
    engine->setNineKey(true);
    r=typeNine("7664");
    require(r.count>1&&contains(r,L"送")&&contains(r,L"松"),"7664 exposes multiple selectable words rather than only rong");
    auto readings=engine->nineKeySpellings(1);
    require(std::find(readings.begin(),readings.end(),"song")!=readings.end()&&std::find(readings.begin(),readings.end(),"rong")!=readings.end(),"nine-key offers song and rong phonetic alternatives");
    r=engine->chooseNineKeySpelling(1,"song");
    require(std::wstring(r.preedit)==L"song"&&contains(r,L"送")&&!contains(r,L"容"),"choosing song filters out rong and updates preedit");
    int songIndex=-1;for(int i=0;i<r.count;i++)if(std::wstring(r.candidates[i])==L"松")songIndex=i;
    require(songIndex>=0&&std::wstring(key('1'+songIndex).commit)==L"松","filtered touch selection uses the matching absolute Rime index");
    require(engine->nineKeyPrefix(1).empty(),"committing a filtered word clears its spelling lock");
    typeNine("7664");r=engine->chooseNineKeySpelling(1,"song");auto firstSong=std::wstring(r.candidates[0]);
    r=key('=');require(r.page==1&&r.count>0,"filtered candidates have a second page");
    r=key('-');require(r.page==0&&std::wstring(r.candidates[0])==firstSong,"filtered previous page returns to the same words");
    r=engine->chooseNineKeySpelling(1,"-");require(engine->nineKeyPrefix(1).empty()&&contains(r,L"容"),"undo spelling restores all alternatives from page zero");
    typeNine("766454");r=engine->chooseNineKeySpelling(1,"song");
    require(contains(r,L"送礼"),"song filtering retains multi-syllable phrases");
    readings=engine->nineKeySpellings(1);require(std::find(readings.begin(),readings.end(),"li")!=readings.end(),"phonetic selection advances to the next syllable");
    r=engine->chooseNineKeySpelling(1,"li");require(std::wstring(r.preedit)==L"song li"&&std::wstring(r.candidates[0])==L"送礼","successive song li locks select the desired phrase");
    require(std::wstring(key(' ').commit)==L"送礼","space commits a complete filtered phrase");
    typeNine("7664");engine->chooseNineKeySpelling(1,"song");r=key(0xff08);
    require(*r.preedit&&r.count>0,"editing a filtered composition remains usable");
    op(Op::Clear);require(engine->nineKeyPrefix(1).empty()&&engine->nineKeySpellings(1).empty(),"clear removes all phonetic state");
    r=typeNine("64426");

    std::cout<<"Nine-key sample: ";for(int i=0;i<r.count;++i)std::cout<<utf8(r.candidates[i])<<" | ";std::cout<<"\n";
    bool foundHello=false;int helloIndex=-1;
    for(int pageIndex=0;pageIndex<5;++pageIndex){for(int i=0;i<r.count;++i)if(std::wstring(r.candidates[i])==L"你好"){foundHello=true;helloIndex=i;break;}if(foundHello||r.last)break;r=key('=');}
    require(foundHello,"real nine-key 64426 offers nihao without multi-tap letters");
    require(std::wstring(key('1'+helloIndex).commit)==L"你好","touch candidate selection commits nine-key word");
    r=typeNine("64426");bool fakeEnglish=false;for(int i=0;i<r.count;++i)if(EnglishLexicon::letters(utf8(r.candidates[i])))fakeEnglish=true;
    require(!fakeEnglish,"nine-key internal alphabet never leaks as English candidates");
    std::cout<<"Nine-key spelling: "<<utf8(r.preedit)<<"\n";
    require(std::wstring(r.preedit)==L"ni hao","nine-key preedit displays decoded ni hao letters");
    for(const char* digits:{"5","56","5664","6442","64426744543"}){
      auto spelling=typeNine(digits);std::cout<<"Nine-key "<<digits<<": "<<utf8(spelling.preedit)<<"\n";
      require(*spelling.preedit&&std::wstring(spelling.preedit).find_first_of(L"0123456789")==std::wstring::npos,"partial and full nine-key composition contains letters rather than digits");
    }
    r=typeNine("64426");
    r=key(0xff08);require(*r.preedit&&r.count>0,"nine-key backspace recomputes candidates");
    r=key(0xff1b);require(!*r.preedit&&!r.count,"nine-key Escape clears composition");
    typeNine("64426");require(std::wstring(key(0xff0d).commit)==L"64426","nine-key raw submit never exposes internal letters");
    r=typeNine("64426744543");
    std::cout<<"Nine-key sentence: "<<utf8(r.candidates[0])<<"\n";
    require(std::wstring(r.candidates[0])==L"你好世界","nine-key composes a multiword phrase");
    require(std::wstring(key(' ').commit)==L"你好世界","nine-key space commits complete multiword phrase");
    r=typeNine("744");auto firstNinePage=std::wstring(r.candidates[0]);auto nextNinePage=key('=');
    require(nextNinePage.page>0&&nextNinePage.count>0,"nine-key candidate next page works");
    require(key('-').page==0,"nine-key candidate previous page returns correctly");
    typeNine("64426");engine->setNineKey(false);r=type("nihao");
    require(std::wstring(r.candidates[0])==L"你好","switching back to 26 keys preserves normal pinyin");
    require(contains(type("buil"),L"build"),"English completion works after switching away from nine keys");
    std::cout<<"PASS: "<<checks<<" engine checks. Test data: "<<utf8(home.wstring())<<"\n";return 0;
  }catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}
}
