$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$lock = Get-Content dictionary.lock.json -Raw -Encoding UTF8 | ConvertFrom-Json
$target = Join-Path $root 'vendor\rime-ice'
foreach ($file in $lock.files) {
  $path = Join-Path $target $file.path
  New-Item -ItemType Directory -Force (Split-Path $path -Parent) | Out-Null
  if (!(Test-Path -LiteralPath $path)) { Invoke-WebRequest -UseBasicParsing -Uri ("https://raw.githubusercontent.com/iDvel/rime-ice/" + $lock.commit + '/' + $file.path) -OutFile $path }
  if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne $file.sha256) { throw ('词库校验失败：' + $file.path) }
}
Write-Host '固定版本词库已准备；运行 python scripts/prepare_data.py 生成离线数据。'
