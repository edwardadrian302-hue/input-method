# 第三方组件

- **librime**：RIME Developers，BSD-3-Clause，核心 API 来源见 `sources.lock.json`；运行库为官方 1.17.0 Windows MSVC x64 发布包。授权副本 `licenses/librime-BSD.txt`。发布包包含的其他依赖也必须保留相应授权，发布前核验。
- **rime-double-pinyin**：原作者佛振，GPL-3.0。`data/jianpin.schema.yaml` 从微软双拼方案修改，去除 ve→v 兼容映射得到搜狗默认双拼键位，并移除反查、繁简转换、引擎自带学习。授权副本 `licenses/double-pinyin-GPL3.txt`。
- **rime-pinyin-simp**：来源于 Android 开源拼音项目的简体词库，Apache-2.0；保留词库头部来源说明及 `licenses/pinyin-simp-Apache2.txt`。
- **LLVM-MinGW**：仅为开发构建工具，位于 `.tools`，不随输入法安装包分发。官方发行地址与校验记录见依赖锁文件。

当前工程供用户本地构建、试用。若对外发布，须附带对应版本源码、修改说明与所有依赖授权；不包含搜狗商标资产或搜狗私有词库。
