# 第三方组件与许可

潮汐拼音使用以下开源组件。对应许可文本随软件保存在 `licenses/` 目录中。

| 组件 | 用途 | 许可 |
|---|---|---|
| librime 1.17.0 | 拼音转换引擎，采用官方 Windows MSVC x64 预编译运行库 | BSD-3-Clause，见 `licenses/librime-BSD.txt` |
| iDvel/rime-ice | 中文常用字、基础与扩展词库 | GPL-3.0，见 `licenses/rime-ice-GPL3.txt` |
| Inno Setup 6.7.4 | Windows 安装与卸载运行时 | 见 `licenses/inno-setup.txt` |

雾凇词库版本：`99662ba01e3affa6a0f009174ca530b887bac3b6`，来源：https://github.com/iDvel/rime-ice 。使用 `cn_dicts/8105`、`base`、`ext`，保留词表中的来源与贡献者说明。潮汐拼音提供独立中文词典包装，不包含上游英文方案或 Lua 扩展。安装目录 `data/` 中的 `.dict.yaml` 为所用词表源码。

librime 核心源码来源：https://github.com/rime/librime 。运行库采用官方发布文件，通过 C API 加载。

安装向导的简体中文消息取自 Inno Setup 同版本官方源码仓库中的社区翻译；作者信息保留于源码仓库的 `scripts/ChineseSimplified.isl`。Inno Setup 的版权与网站信息保留于安装/卸载运行时。

LLVM-MinGW 仅用于开发构建，不随软件分发。开发依赖、下载校验与源码版本记录保存在源码仓库的锁文件中。潮汐图标为本项目自有资产。

可选新词下载来源：[iDvel/rime-ice main/cn_dicts/ext.dict.yaml](https://github.com/iDvel/rime-ice/blob/main/cn_dicts/ext.dict.yaml)，沿用该词库 GPL-3.0 许可；内置词库仍固定上述 commit，下载操作不覆盖内置文件。
