param([string]$SevenZip = 'D:\7-Zip\7z.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if ([IO.Path]::GetPathRoot($root) -ne 'D:\') { throw '按本项目约定，编译工具必须放在 D 盘。请将项目放在 D 盘后运行。' }
Set-Location $root
if (!(Test-Path $SevenZip)) { throw '请通过 -SevenZip 指定现有 7z.exe 的完整路径。' }
New-Item -ItemType Directory -Force .tools,.downloads | Out-Null
$assets = Get-Content downloads.lock.json -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($asset in $assets) {
  $name = [IO.Path]::GetFileName(([Uri]$asset.url).AbsolutePath)
  $file = Join-Path $root ('.downloads\' + $name)
  if (!(Test-Path $file)) { Invoke-WebRequest -UseBasicParsing -Uri $asset.url -OutFile $file }
  if ((Get-FileHash $file -Algorithm SHA256).Hash.ToLower() -ne $asset.sha256) { throw "下载校验失败：$name" }
  $output = if ($name -like 'llvm-*') { Join-Path $root '.tools' } else { Join-Path $root '.tools\rime' }
  & $SevenZip x $file ("-o$output") -y -bso0
  if ($LASTEXITCODE -ne 0) { throw "解压失败：$name" }
}
Write-Host '编译工具与引擎已准备在 D 盘项目 .tools；未修改系统 PATH。'
Write-Host 'data 目录已包含词库。可运行 scripts/build.ps1；仅重新生成词库时才需要 vendor 源代码。'
