# 第三方组件

- **librime**：RIME Developers，BSD-3-Clause，核心 API 来源见 `sources.lock.json`；运行库为官方 1.17.0 Windows MSVC x64 发布包。授权副本 `licenses/librime-BSD.txt`。发布包包含的其他依赖也必须保留相应授权，发布前核验。
- **rime-double-pinyin**：0.1.0 双拼方案的历史来源，原作者佛振，GPL-3.0。0.2.0 改为全拼，不再使用其双拼代数规则；保留历史来源锁与许可证副本。
- **rime-pinyin-simp**：来源于 Android 开源拼音项目的简体词库，Apache-2.0；保留词库头部来源说明及 `licenses/pinyin-simp-Apache2.txt`。
- **LLVM-MinGW**：仅为开发构建工具，位于 `.tools`，不随输入法安装包分发。官方发行地址与校验记录见依赖锁文件。

当前工程供用户本地构建、试用。若对外发布，须附带对应版本源码、修改说明与所有依赖授权；不包含搜狗商标资产或搜狗私有词库。

- **Inno Setup 6.7.4**：仅在 D 盘用于构建安装包，安装包含 Inno Setup 安装/卸载运行时；来源、版本和校验见 `installer.lock.json`。构建工具许可见 `.tools/inno-setup/License.txt`，其版权声明保留于运行时。简体中文安装消息来自该版本官方仓库的社区翻译，作者信息保留于 `scripts/ChineseSimplified.isl`。
