#include "engine.hpp"
#include <jni.h>
#include <memory>
#include <mutex>
#include <codecvt>
static std::unique_ptr<Engine> engine;
static std::mutex gate;
static std::string text(JNIEnv* env,jstring value) {
  auto chars=env->GetStringChars(value,nullptr);if(!chars)throw std::runtime_error("Cannot read path");
  std::u16string data(reinterpret_cast<const char16_t*>(chars),env->GetStringLength(value));
  env->ReleaseStringChars(value,chars);
  return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t>{}.to_bytes(data);
}
static jstring string(JNIEnv* env,const std::wstring& value) {
  auto data=std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t>{}.from_bytes(utf8(value));
  return env->NewString(reinterpret_cast<const jchar*>(data.data()),static_cast<jsize>(data.size()));
}
static jobjectArray response(JNIEnv* env,const Response& r){
  auto spellings=engine->nineKeySpellings(1);
  std::vector<std::wstring> values={r.commit,r.preedit,std::to_wstring(r.page),std::to_wstring(r.last),std::to_wstring(r.prediction),std::to_wstring(r.eaten),std::to_wstring(r.count),wide(engine->nineKeyPrefix(1).c_str())};
  for(int i=0;i<r.count;++i)values.emplace_back(r.candidates[i]);
  for(auto& spelling:spellings)values.push_back(wide(spelling.c_str()));
  auto strings=env->NewObjectArray(static_cast<jsize>(values.size()),env->FindClass("java/lang/String"),nullptr);
  for(size_t i=0;i<values.size();++i){auto item=string(env,values[i]);env->SetObjectArrayElement(strings,static_cast<jsize>(i),item);env->DeleteLocalRef(item);}
  return strings;
}
extern "C" JNIEXPORT void JNICALL Java_com_chaoxi_pinyin_EngineBridge_initialize(JNIEnv* env,jclass,jstring root,jstring home) {
  std::lock_guard<std::mutex> lock(gate);
  try {if(!engine)engine=std::make_unique<Engine>(fs::u8path(text(env,root)),fs::u8path(text(env,home)));}
  catch(const std::exception&){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Input engine initialization failed");}
}
extern "C" JNIEXPORT jobjectArray JNICALL Java_com_chaoxi_pinyin_EngineBridge_request(JNIEnv* env,jclass,jint op,jint key,jint value) {
  std::lock_guard<std::mutex> lock(gate);
  try {
    if(!engine)throw std::runtime_error("Engine not ready");
    Request q;q.op=static_cast<Op>(op);q.key=key;q.value=value;q.client=1;
    auto r=engine->handle(q);if(!r.ok)throw std::runtime_error("Request failed");
    return response(env,r);
  }catch(const std::exception&){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Input engine request failed");return nullptr;}
}

extern "C" JNIEXPORT void JNICALL Java_com_chaoxi_pinyin_EngineBridge_privacy(JNIEnv*,jclass,jboolean suppress) {
  std::lock_guard<std::mutex> lock(gate);if(engine)engine->suppressLearning(suppress);
}

extern "C" JNIEXPORT void JNICALL Java_com_chaoxi_pinyin_EngineBridge_setNineKey(JNIEnv* env,jclass,jboolean enabled) {
  std::lock_guard<std::mutex> lock(gate);
  try {if(!engine)throw std::runtime_error("Not ready");engine->setNineKey(enabled);}
  catch(...){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Cannot change keyboard layout");}
}
extern "C" JNIEXPORT jintArray JNICALL Java_com_chaoxi_pinyin_EngineBridge_settings(JNIEnv* env,jclass,jint words,jint phrases,jboolean learning,jboolean completion,jint length,jboolean save) {
  std::lock_guard<std::mutex> lock(gate);
  try {
    if(!engine)throw std::runtime_error("Not ready");
    Request q;q.op=Op::Status;auto state=engine->handle(q);
    if(save){q.op=Op::Configure;q.key=state.font;q.mask=state.fuzzy;q.limit=length;
      q.value=(state.options&~OptCompletion)|(learning?OptLearning:0)|(completion?OptCompletion:0);
      copyText(q.text,std::to_wstring(words)+L" "+std::to_wstring(phrases));state=engine->handle(q);
      if(!state.ok)throw std::runtime_error("Invalid settings");}
    q={};q.op=Op::Capacity;auto caps=engine->handle(q);
    jint values[]={state.learning,(state.options&OptCompletion)?1:0,caps.importNew,caps.importDuplicate,caps.importInvalid,caps.phraseCount,state.limit};
    auto result=env->NewIntArray(7);env->SetIntArrayRegion(result,0,7,values);return result;
  }catch(...){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Cannot save settings or reduce capacity below existing records");return nullptr;}
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_chaoxi_pinyin_EngineBridge_chooseSpelling(JNIEnv* env,jclass,jstring spelling){
  std::lock_guard<std::mutex> lock(gate);
  try {if(!engine)throw std::runtime_error("Not ready");return response(env,engine->chooseNineKeySpelling(1,text(env,spelling)));}
  catch(...){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Cannot choose pinyin");return nullptr;}
}
