# 潮汐拼音 Android 第三方组件

- Rime 1.17.0：BSD-3-Clause，固定源码提交见 ../sources.lock.json。
- 雾凇中文及英文词表：GPL-3.0，版本与文件摘要见 ../dictionary.lock.json，保留词表原始头部。
- yaml-cpp：MIT；LevelDB：BSD-3-Clause；OpenCC：Apache-2.0。固定源码提交及归档摘要见 ../android-dependencies.lock.json。
- OpenCC 归档内 Marisa：BSD / LGPL 双重许可，采用 BSD 条款；Darts-clone：BSD；RapidJSON：MIT（部分头部另有注明）。
- Boost 1.89.0：Boost Software License 1.0。

许可原文随 APK 保存在 assets/licenses。源码来源：
https://github.com/rime/librime
https://github.com/iDvel/rime-ice
https://github.com/jbeder/yaml-cpp
https://github.com/google/leveldb
https://github.com/BYVoid/OpenCC
https://www.boost.org/
https://github.com/Tencent/rapidjson/tree/v1.1.0

Android SDK、NDK、CMake、Gradle、AGP 和 JDK 仅用于构建；NDK 的 C++ 共享运行库随 APK 打包，相关 LLVM libc++ 许可见 assets/licenses/libcxx.txt。
