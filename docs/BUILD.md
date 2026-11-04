# 构建

构建工具目录：`D:\cursor\projects\input-method\.tools`。不修改系统 PATH，不在 C 盘安装 C++ 编译工具。

采用 LLVM-MinGW 20261020 UCRT x64 便携工具链；Rime 采用官方 librime 1.17.0 Windows MSVC x64 二进制，通过稳定 C API 动态加载。构建时不使用 MSVC 导入库。当前构建不等于从源码重建所有依赖。

准备源代码：按 `sources.lock.json` 中的 URL 和 commit checkout 到 `vendor/librime`、`vendor/double-pinyin`、`vendor/pinyin-simp`。执行 `python scripts/prepare_data.py` 生成方案并复制词库和许可。

准备工具：将官方 `llvm-mingw-20261020-ucrt-x86_64.zip` 解压到 `.tools`；将官方 `rime-33e7814-Windows-msvc-x64.7z` 解压到 `.tools/rime`。

运行 `powershell -ExecutionPolicy Bypass -File scripts/build.ps1`。输出在 `dist`，自动运行真实 Rime 引擎集成测试。测试数据仅创建在工程 `test-output` 的独立运行目录中，不碰真实用户词频。

主要产物：

- `JianPin.dll`：64 位 Windows TSF 输入法。
- `JianPinBroker.exe`：单用户本地引擎进程。
- `JianPinSettings.exe`：原生设置界面。
- `rime.dll`、`data`：运行库和离线词库。
- `EngineTest.exe`：开发测试，不安装到用户目录。

先构建验证，再运行管理员安装脚本。系统注册需要管理员权限，构建不需要。

## 生成带安装向导的 EXE

使用 Inno Setup 6.7.4。工具便携解包到项目 `.tools/inno-setup`，不修改 PATH、不创建打包工具的系统卸载项。来源、SHA-256、官方 GitHub 资产摘要和发布者记录见 `installer.lock.json`；简体中文翻译取自同版本官方源码树，保留文件头及哈希。

```powershell
powershell -ExecutionPolicy Bypass -File scripts/prepare-installer.ps1
powershell -ExecutionPolicy Bypass -File scripts/build-installer.ps1
```

第一条首次下载需要联网，后续使用已校验的本地下载包。第二条先构建并运行引擎、TSF 测试，通过后生成 `build/JianPin-0.2.0-preview-win-x64-Setup.exe`。若当前沙箱不允许管道或 TSF，需在普通 Windows 用户终端运行测试，不能把沙箱失败等同于输入法故障。`-SkipBuild` 仅供已经核实当前构建及测试的开发者重新打包文档使用。

安装脚本是 `scripts/installer.iss`，可修改版本、默认目录和安装内容。安装器包含注册和反注册、目录选择、开始菜单入口以及系统卸载项，安装时没有下载步骤。安装器自带程序清单要求管理员权限。不会更改默认输入法或强制关闭浏览器；被占用 DLL 卸载时可能延迟至重启删除。旧脚本版升级使用原目录；EXE 版变更目录需先卸载。可继续运行 `scripts/package.ps1` 生成 ZIP 备用包。

界面渲染预览：构建生成 `build/CandidatePreview.exe`，运行后得到 96/144/192 DPI 的候选窗位图，使用的排版和绘制代码与 TSF 候选窗相同，但不包含真实桌面阴影与窗口圆角裁剪。它不是浏览器实机验收。

TSF 测试会设置仅对该测试进程和子进程生效的随机数值测试命名空间，独立管道及互斥量可与用户已经安装的简拼共存；用户输入法及学习数据不会被停止或清空。

Inno Setup 的使用和再分发条件见官方工具附带许可证，商业使用须核对其商业授权要求。项目安装包仍是未签名的开发预览版。
