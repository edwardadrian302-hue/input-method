# 构建

构建工具目录：`D:\cursor\projects\input-method\.tools`。不修改系统 PATH，不在 C 盘安装 C++ 编译工具。

采用 LLVM-MinGW 20261020 UCRT x64 便携工具链；Rime 采用官方 librime 1.17.0 Windows MSVC x64 二进制，通过稳定 C API 动态加载。构建时不使用 MSVC 导入库。当前构建不等于从源码重建所有依赖。

准备源代码：按 `sources.lock.json` 中的 URL 和 commit checkout 到 `vendor/librime`。另运行 `scripts/prepare-dictionary.ps1` 获取词库锁指定版本，再执行 `python scripts/prepare_data.py` 生成方案并复制词库和许可。

准备工具：将官方 `llvm-mingw-20261020-ucrt-x86_64.zip` 解压到 `.tools`；将官方 `rime-33e7814-Windows-msvc-x64.7z` 解压到 `.tools/rime`。

运行 `powershell -ExecutionPolicy Bypass -File scripts/build.ps1`。输出在 `dist`，自动运行真实 Rime 引擎集成测试。测试数据仅创建在工程 `test-output` 的独立运行目录中，不碰真实用户词频。

主要产物：

- `ChaoxiPinyin.dll`：64 位 Windows TSF 输入法。
- `ChaoxiPinyinBroker.exe`：单用户本地引擎进程。
- `ChaoxiPinyinSettings.exe`：原生设置界面。
- `rime.dll`、`data`：运行库和词库。
- `build/tests/EngineTest.exe`、`build/tests/TsfTest.exe`：开发测试，不放入 dist 或用户安装包。

先构建验证，再运行管理员安装脚本。系统注册需要管理员权限，构建不需要。

## 生成带安装向导的 EXE

使用 Inno Setup 6.7.4。工具便携解包到项目 `.tools/inno-setup`，不修改 PATH、不创建打包工具的系统卸载项。来源、SHA-256、官方 GitHub 资产摘要和发布者记录见 `installer.lock.json`；简体中文翻译取自同版本官方源码树，保留文件头及哈希。

```powershell
powershell -ExecutionPolicy Bypass -File scripts/prepare-installer.ps1
powershell -ExecutionPolicy Bypass -File scripts/build-installer.ps1
```

第一条首次下载需要联网，后续使用已校验的本地下载包。第二条先构建并运行引擎、TSF 测试，通过后生成 `build/ChaoxiPinyin-0.6.2-preview-win-x64-Setup.exe`。若当前沙箱不允许管道或 TSF，需在普通 Windows 用户终端运行测试，不能把沙箱失败等同于输入法故障。`-SkipBuild` 仅供已经核实当前构建及测试的开发者重新打包文档使用。

安装脚本是 `scripts/installer.iss`，可修改版本、默认目录和安装内容。安装器包含注册和反注册、目录选择、开始菜单入口以及系统卸载项，安装时没有下载步骤。安装器自带程序清单要求管理员权限。不会更改默认输入法或强制关闭浏览器；被占用 DLL 卸载时可能延迟至重启删除。旧脚本版升级使用原目录；EXE 版变更目录需先卸载。可继续运行 `scripts/package.ps1` 生成 ZIP 备用包。

界面渲染预览：构建生成 `build/CandidatePreview.exe`，运行后得到 96/144/192 DPI 的候选窗位图，使用的排版和绘制代码与 TSF 候选窗相同，但不包含真实桌面阴影与窗口圆角裁剪。它不是浏览器实机验收。

TSF 测试会设置仅对该测试进程和子进程生效的随机数值测试命名空间，独立管道及互斥量可与用户已经安装的潮汐拼音共存；用户输入法及学习数据不会被停止或清空。

Inno Setup 的使用和再分发条件见官方工具附带许可证，商业使用须核对其商业授权要求。项目安装包仍是未签名的开发预览版。

## 词库与界面

仓库 `data/cn_dicts` 已包含固定版本的雾凇常用字、基础和扩展词库，原始头部与许可保留。重新生成时先运行 `scripts/prepare-dictionary.ps1` 准备 `vendor/rime-ice`（固定 commit 与 SHA-256），再运行 `python scripts/prepare_data.py`；生成器验证全部词库哈希，包装为只含中文词表的 `chaoxi.dict.yaml`，0.6.0 另复制锁定的英文基础表到 `data/en_dicts/en.dict.yaml`，由本地引擎单独读取用于英文补全；不引入上游英文方案、Lua、自动更新或云服务。词库锁为 `dictionary.lock.json`，不由引擎运行时下载。

`src/settings.cpp` 是原生可视化设置界面，支持即时字号预览，应用动作在后台线程中等待本地引擎。`assets/chaoxi.svg` / `chaoxi.ico` 是项目自有潮汐图标，资源通过同一 D 盘工具链 windres 编译到 DLL 和 EXE。源代码与词库许可证见 THIRD_PARTY.md。

命名空间环境变量为 `CHAOXI_TEST_INSTANCE`。测试不连接已安装输入法的管道，不读取其用户词频。协议使用 v5 命名；只迁移同一 ChaoxiPinyin 根目录中的 0.3 设置与词频，不读取更早 JianPin 目录。

## 发布目录规则

每次构建会检查 `dist` 为项目中的普通目录，然后重新生成，避免混入历史二进制、旧词典或测试程序。运行时文件只含潮汐拼音主程序、引擎、词库、许可、用户说明与脚本安装/卸载工具。测试程序位于 `build/tests`，验证记录和构建依赖锁文件只保存在源码仓库。EXE 采用同一批运行文件，不附带脚本安装工具，也不显示额外的安装前说明页。

Rime 方案、prism、custom 配置统一命名为 `chaoxipinyin`；安装标记为 `chaoxipinyin-install.marker`。TSF CLSID、语言配置 GUID 与安装器 AppId 是稳定的产品身份，保持不变。已注册在原目录的 EXE 安装允许原位升级；其他非空目录必须有当前安装标记。删除过时产物不涉及用户已安装的程序或学习数据。

`build/tests/SettingsPreview.exe` 使用隐藏窗口中的实际控件位置、文字与字号绘制布局示意图，输出 `build/settings-preview.bmp` 和 `build/phrases-preview.bmp`，不显示窗口、不启动 broker。它不代替真实 Windows 控件交互、文件选择对话框或多屏验收。

词库解析测试：`build/tests/DictionaryTest.exe`。显式联网实测：`build/tests/DictionaryTest.exe --online`，只下载公开词库并在 build 写入结果；默认构建测试不联网。设置与该测试链接系统 WinHTTP。

## 0.6.1 同目录数据

运行时数据固定为安装目录的 `UserData`，包含 `profile.tsv`、`engine-v5` 与 `cache-v5`。首次启动 broker 时，在其互斥量内验证并复制旧 `%LOCALAPPDATA%\ChaoxiPinyin\profile.tsv`，新目录已有档案则跳过，不删除旧数据；损坏档案报错，不使用空档案覆盖。更早的 settings.ini / words.tsv 按既有逻辑迁移。缓存在新目录重建。

EXE 与脚本安装只为 UserData 授予本机 Users 修改权限，不放宽二进制目录权限。数据属于该安装目录，本机多账户共用；本版针对个人电脑使用，不提供账户间隔离。卸载保留 UserData 和安装标记，便于原目录重装。不要在程序运行时移动数据；删除整个安装文件夹前先备份。

测试 broker 的 `--data-dir` 跳过真实旧目录迁移。迁移回归只使用 test-output 下显式传入的模拟源与目标。

0.6.2 另生成 chaoxipinyin_t9.schema.yaml；普通与九键方案共用固定词表，派生 prism 分开。Android 0.2.0 的工程、设置与构建说明见 ANDROID.md。升级后的档案使用 v2；应保留旧版备份供回退，不能让旧程序读取新格式。
