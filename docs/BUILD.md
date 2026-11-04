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
