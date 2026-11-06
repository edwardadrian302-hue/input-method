# 潮汐拼音

一款简洁的 Windows 中文拼音输入法。支持手动下载词库更新；无需账号，没有广告、遥测、云同步或后台自动更新。

当前版本：**0.6.6 预览版**。面向 Windows 10/11 x64 的 64 位应用。

设置采用浅绿色分类卡片布局；在“词库与数据 → 查看与修改本地短词”中可管理自动学习的词语和次数。

## 安装

1. 运行 `ChaoxiPinyin-0.6.6-preview-win-x64-Setup.exe`。
2. 按向导选择安装目录，默认 `D:\ChaoxiPinyin`，并允许 Windows 管理员权限请求。
3. 按 **Win + 空格**，选择 **“潮汐拼音”**。

安装不会更改默认输入法。若切换列表中没有出现潮汐拼音，保存文件后注销并重新登录，再试一次。

升级后若仍是旧版行为，请先保存工作，再注销并重新登录，让应用和本地引擎加载新版文件。

## 输入

候选合并后连续分页：每页最多 9 项，仍有后续候选时会补满当前页；只有真正的最后一页可能不足 9 项。

| 功能 | 操作或示例 |
|---|---|
| 全拼 | `nihao` → 你好，`qishangbaxia` → 七上八下 |
| 补全与声母缩写 | `shenm` / `sm` → 什么，`zenm` → 怎么 |
| 英文候选 | 中文状态输入 `build` 可选英文原文；输入 `buil` 可选完整单词 `build`，选择后保持中文状态 |
| 选择候选 | 数字 **1–9**；空格选择首选 |
| 翻页 | **- / =** 或 **PageUp / PageDown** |
| 修改与取消 | **Backspace / Esc** |
| 中英文切换 | 单按 **Shift**，或点击输入法的 **中 / 英** 按钮 |
| 标点 | 中文模式默认使用中文标点；英文模式直接输入英文标点 |

输入拼音后按逗号或分号，会提交首选并追加标点。中文状态输入英文字母后，单按 Shift 会原样提交字母并切换到英文，例如 `hello` → `hello`；空格仍选择中文首选。点击中 / 英按钮切换时，先提交当前首选。

候选窗每页显示九个候选，宽度随内容调整，并适应屏幕缩放。内置雾凇中文词库。

## 设置

从开始菜单打开 **“潮汐拼音设置”**，或右键点击输入法的中 / 英按钮。

- **外观**：字号 10–24 磅，默认 12 磅，支持即时预览。
- **标点**：可在中文模式下使用英文标点。
- **补全**：拼音补全、声母缩写与英文单词补全默认开启。英文从三个字母起提供最多三个本地词库补全，保留词库中的大小写；英文原文候选始终可用。中文首选保持优先，英文候选用数字选择，不写中文学习词频。仅支持纯英文字母，不提供英文语法纠错或整句预测。
- **模糊音**：z/zh、c/ch、s/sh、n/l、en/eng、an/ang、in/ing 可分别开启，默认关闭。
- **误按纠错**：可选开启；支持完整拼音的一次漏字、多字、错按或相邻字母颠倒，在候选中提供建议，不自动替换输入。
- **上屏联想**：可选开启；选词后按本地词库提供续词，空格或数字选择，Esc 关闭。继续输入拼音则开始新一轮输入。
- **学习**：开启或关闭本地词条学习，选择 1–4、1–8 或 1–16 字范围，清除已学习的词条。
- **个人词库**：新增、修改和删除手动词条，填写词语与完整拼音，例如“潮汐拼音 / chao xi pin yin”。默认最多 4096 条，可在设置中改为 1–100000 条，每条 1–32 个汉字；ü 使用 v。支持批量导入，导入词条也计入容量并参与备份。
- **备份**：导出或恢复 `.cxbackup` 文件，包含已保存设置、自动学习词频和手动词条。恢复会替换当前数据，操作前有确认。

**词库容量**：自动学习与个人词条分别配置，默认 8192 / 4096 条，各可设置为 1–100000 条。降低到已有条数以下会拒绝保存，不自动删词；容量随备份一起保存。

修改后点击 **“应用设置”**。调整拼写规则时需要准备词库索引，等待完成后即可继续输入。

## 批量词库与新词更新

打开 **设置 → 管理个人词库**：

- **导入本地词库**：选择 UTF-8 的 `.dict.yaml`、TXT 或 TSV，默认列顺序为“词语、拼音、可选权重”，用 Tab 分隔。拼音使用英文字母、空格或撇号，ü 写成 v。不支持二进制 SCEL、自定义 YAML 列映射、声调符号或仅有词语的文件；不会猜测多音字读音，也不会加载 YAML 引用的其他词表或代码。
- **检查新词更新**：经提示后从 [雾凇拼音](https://github.com/iDvel/rime-ice) 的最新扩展词库下载，筛选本机尚未收录的词。这是社区新词增量，不保证覆盖实时热搜。
- 两种方式都会显示新增、重复、无效条数，确认后合并；跳过内置及已存在的相同拼音/词语。超过剩余容量时整批拒绝，可先拆分词库。取消、下载失败和保存失败均保留原有数据。
- 下载仅发生在主动点击时，使用 HTTPS；不上传输入内容、词频或个人词库。联网下载可能受 GitHub 可达性影响，失败可下载文件后本地导入。正常输入不依赖网络。

文本示例（词语与拼音之间为 Tab）：

```text
潮汐拼音	chao xi pin yin
潮汐词库	chao xi ci ku
```

## 本地数据

本地学习默认开启，默认仅保存已提交的 **1–4 个纯汉字词条及次数**，可在设置中扩大到 8 或 16 字，默认最多 8192 条，可在设置中改为 1–100000 条，用于调整同页、同字数候选排序，保留完整短语与局部候选的先后顺序。扩大范围后，相应长度的中文短语也可能被记住。不保存自动输入的原始拼音、英语、数字或按时间排列的输入历史。

手动词条独立于自动学习；关闭或清除自动学习不会删除手动词条。手动词条按完整拼音匹配，不自动推断读音。误按纠错和上屏联想默认关闭，可按需开启；联想是本地词库的前缀续词，不读取应用上下文或剪贴板。

关闭学习后，已有数据保留，但不参与排序、不继续更新；清除学习数据不会影响内置词库或手动词条。词条中也可能包含个人信息，可随时关闭学习并清除。

数据保存在安装目录的 `UserData` 文件夹（默认 `D:\ChaoxiPinyin\UserData`），设置、学习词频和手动词条统一保存在明文 `profile.tsv`，另有词库缓存。首次启动 0.6.2 时，如果新目录没有档案，会校验并复制 `%LOCALAPPDATA%\ChaoxiPinyin\profile.tsv`，旧文件保留；旧档案损坏会报错而非静默丢弃。缓存不复制，将在新目录重建。更早版本会读取旧目录的 `settings.ini` 和 `words.tsv`，原文件保留，不再作为新版的数据源；明确点击“清除学习数据”时也会移除旧词频文件。

`UserData` 允许本机普通用户写入，同一安装目录共用数据，适合个人电脑使用；其他本机账户也可能访问这些数据。卸载保留此目录，手动删除安装文件夹前请先备份。不要在输入法运行时手动移动或修改档案。

0.6.2 起档案及备份使用格式 v2，包含容量设置；新版可读取旧格式 v1，旧版程序不能读取 v2，升级前请导出旧版备份供需要时回退。

备份文件同样是明文，请妥善保存。备份只包含个人数据，不包含内置词库和缓存；恢复时校验格式、条数和长度，文件无效时不覆盖现有数据。

## 卸载

先切换到其他输入法，再通过 Windows **“已安装的应用”** 或开始菜单 **“卸载潮汐拼音”** 卸载。卸载默认保留本地数据；如不需要，请先在设置中清除学习数据。系统提示需要重启时，请按提示操作。

## 从源码构建 Windows 版

### 环境要求

- Windows 10/11 x64，使用普通用户的 Windows PowerShell 5.1 终端。
- Git for Windows，用于拉取源码。
- 可解压 ZIP 和 `.7z` 文件的工具。自动准备脚本调用 7-Zip 的 `7z.exe`，运行该脚本时需提供它的完整路径；使用其他解压工具的方式见下文。
- 工具和下载缓存在仓库内的 `.tools`、`.downloads`。当前 `bootstrap.ps1` 和 `prepare-installer.ps1` 按项目原有的便携工具管理约定检查 D 盘，因此直接使用这两个脚本时，请将仓库放在 D 盘，例如 `D:\projects\input-method`。
- 建议预留至少 5 GB 可用空间，并能访问 GitHub 下载固定版本工具。首次准备需要联网。

脚本自动准备 LLVM-MinGW 20261020 UCRT x64 和 librime 1.17.0，并核对 SHA-256。Windows 构建直接使用仓库内的词库文件和预编译 Rime，无需先准备 Python、Visual Studio、Android SDK 或 `vendor` 源码。重新生成词库的开发流程见 [构建说明](docs/BUILD.md)。

### 拉取、准备和构建

下面是已验证的自动准备流程，使用 D 盘仓库和 7-Zip。在 PowerShell 中执行，按本机实际位置修改 `7z.exe` 路径。

```powershell
git clone https://github.com/edwardadrian302-hue/input-method.git D:\projects\input-method
cd D:\projects\input-method
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap.ps1 -SevenZip "C:\Program Files\7-Zip\7z.exe"
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1
```

`build.ps1` 编译程序并运行引擎、词库解析、隔离 TSF 和设置布局检查。运行文件输出到 `dist`，测试程序输出到 `build/tests`，模拟词库和学习记录输出到 `test-output`。测试使用独立数据目录和随机管道，请在普通 Windows 用户终端执行。工具脚本不修改系统 PATH。

### 使用其他解压工具或盘符

7-Zip 用于解压下载的工具包，编译过程调用的是 LLVM-MinGW。也可以使用其他支持 ZIP 和 `.7z` 格式的工具手动准备依赖。

1. 按 `downloads.lock.json` 中的地址下载两个工具包，并用 `Get-FileHash -Algorithm SHA256` 核对其中记录的摘要。
2. 将 LLVM-MinGW 工具包解压到仓库 `.tools`，确保编译器位于 `.tools/llvm-mingw-20261020-ucrt-x86_64/bin/x86_64-w64-mingw32-clang++.exe`，保留工具包完整目录结构。
3. 将 Rime 工具包解压到 `.tools/rime`，确保存在 `.tools/rime/dist/include/rime_api.h` 和 `.tools/rime/dist/lib/rime.dll`。
4. 在仓库根目录运行 `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1`。

盘符限制来自上述两个准备脚本中的显式检查，Windows 编译脚本按仓库所在位置计算工具路径。使用 C、E 等其他盘符时，需要手动准备依赖，或先调整准备脚本的盘符检查。生成 EXE 还需按 `installer.lock.json` 校验并准备 Inno Setup 6.7.4，使 `.tools/inno-setup/ISCC.exe` 可用。手动准备完成后，跳过下一节的 `prepare-installer.ps1`，直接执行 `build-installer.ps1 -SkipBuild` 和 `package.ps1`。其他盘符和其他解压工具的组合尚未单独实测；目前完整验证的流程是 D 盘与 7-Zip。

### 生成安装包

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prepare-installer.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-installer.ps1 -SkipBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package.ps1
```

这里的 `-SkipBuild` 用于上一节已经完整构建并通过测试的同一份源码。修改源码后须重新运行 `build.ps1`，也可以省略 `-SkipBuild`，由打包脚本重新构建和测试。

产物为 `build/ChaoxiPinyin-0.6.6-preview-win-x64-Setup.exe` 和 `build/ChaoxiPinyin-0.6.6-preview-win-x64.zip`。Inno Setup 6.7.4 会便携准备到 `.tools/inno-setup`，下载时验证摘要和官方签名，其使用条件见 [第三方说明](THIRD_PARTY.md)。

以上命令只准备工具、构建、测试和打包。安装及系统输入法注册由使用者手动执行，需要管理员权限。

### 首次构建验证记录

2026-11-06，在现有 Windows x64 主机上从 GitHub 全新拉取源码，确认项目内没有 `.tools`、`.downloads`、`vendor`、`build` 或 `dist`，重新下载工具后完成 Windows 编译、433 项核心检查、7 项词库解析检查、隔离 TSF 与 100 次 IPC 检查、设置布局检查，以及 EXE 和 ZIP 打包。ZIP 中 20 个文件逐项与该次 `dist` 校验一致。

首次检出还发现，Git 的 `core.autocrlf=true` 会改变 `ChineseSimplified.isl` 的换行，导致安装工具准备时摘要不匹配。已通过 `.gitattributes` 为此文件设置 `-text`，重新检出验证后打包通过。

本次验证使用 Windows PowerShell 5.1.26100.9711、7-Zip 26.01 x64。验证范围为现有主机中的全新项目目录和空项目缓存，系统安装与 Windows 10 独立兼容性按 [验收清单](docs/ACCEPTANCE.md) 另行检查。

## 项目资料

[第三方组件与许可](THIRD_PARTY.md)。构建、架构和兼容性验证记录保存在源码仓库的 `docs/` 目录中，不随用户安装包分发。当前版本为预览版，兼容性验证进展见源码仓库的 `docs/VERIFICATION.md`；32 位应用与 ARM64 暂不支持。
