param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$compiler = Join-Path $root '.tools\inno-setup\ISCC.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw '请先运行 scripts/prepare-installer.ps1 准备 D 盘便携打包工具。' }
if (!$SkipBuild) {
  & powershell -ExecutionPolicy Bypass -File scripts/build.ps1
  if ($LASTEXITCODE -ne 0) { throw '构建或测试失败，停止打包。' }
}
& $compiler scripts/installer.iss
if ($LASTEXITCODE -ne 0) { throw 'EXE 安装包编译失败。' }
Get-FileHash build/ChaoxiPinyin-0.6.3-preview-win-x64-Setup.exe -Algorithm SHA256
