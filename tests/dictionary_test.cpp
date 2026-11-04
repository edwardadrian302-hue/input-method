#include "dictionary_import.hpp"
#include "dictionary_download.hpp"
#include <iostream>
static int checks=0;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);++checks;}
int wmain(int argc,wchar_t**){try{
  auto p=DictionaryImport::parse(utf8(L"\ufeff# example\r\n潮汐新词\tchao xi xin ci\t100\r\n潮汐新词\tCHAO XI XIN CI\n坏词123\thuai ci\n没有拼音\n"));
  require(p.phrases.size()==1&&p.duplicates==1&&p.invalid==2,"UTF-8 BOM CRLF normalization duplicates invalid rows");
  p=DictionaryImport::parse(utf8(L"# Rime\n---\nname: test\nimport_tables:\n  - not-loaded\n...\n潮汐词库\tchao xi ci ku\t50\n"));
  require(p.phrases.size()==1&&!p.invalid,"Rime metadata is data only");
  for(auto data:{std::string("\xff"),std::string("a\0b",3),std::string("---\nname: test\n"),std::string("---\ncolumns: [code,text]\n...\na\tb\n")}){
    bool rejected=false;try{DictionaryImport::parse(data);}catch(...){rejected=true;}require(rejected,"reject encoding truncated YAML and remapped columns");}
  p=DictionaryImport::parse(utf8(L"女儿\tnv er\n旅游\tlü you\n"));require(p.phrases.size()==1&&p.invalid==1,"unsupported pronunciation is counted rather than guessed");
  if(argc>1){auto data=downloadDictionary();auto current=DictionaryImport::parse(data);require(current.phrases.size()>1000,"live upstream dictionary downloads and parses");
    auto out=moduleDir().parent_path()/L"network-dictionary-check.txt";writeProfileFile(out,"Source: "+utf8(DictionaryUrl)+"\nBytes: "+std::to_string(data.size())+"\nValid: "+std::to_string(current.phrases.size())+"\nInvalid: "+std::to_string(current.invalid)+"\n");
    std::cout<<"Live download: "<<data.size()<<" bytes, "<<current.phrases.size()<<" unique valid phrases\n";}
  std::cout<<"PASS: "<<checks<<" dictionary checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
